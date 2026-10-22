// The character session: the tab bar, the hover summary / full-page logic on
// the top screen, and the keys shared by every tab.
#include <stdlib.h>
#include <string.h>

#include "core/text.h"
#include "ui.h"

// L/R order of the tabs (ids: 0 skills, 1 fight, 2 arms, 3 magic, 4 feats, 5 edit).
static const int TAB_ORDER[TAB_COUNT] = { TAB_SKILLS, TAB_FEATS, TAB_MAGIC, TAB_ARMS, TAB_FIGHT, TAB_EDIT };

int spell_level_of(const char *name)
{
    const Spell *sp = spells_find(&A.lib, name);
    return sp ? sp->level : -1;
}

static const Character *sort_k;

static int cmp_spells(const void *a, const void *b)
{
    const KnownSpell *x = &sort_k->spells[*(const int *)a], *y = &sort_k->spells[*(const int *)b];
    int lx = spell_level_of(x->name), ly = spell_level_of(y->name);
    if (lx < 0)
        lx = 99;
    if (ly < 0)
        ly = 99;
    return lx != ly ? lx - ly : strcmp(x->name, y->name);
}

static int feat_cat_of(const char *name)
{
    const Feature *f = features_find(&A.flib, name);
    return f ? f->cat : 9;
}

static int cmp_feats(const void *a, const void *b)
{
    const KnownFeat *x = &sort_k->feats[*(const int *)a], *y = &sort_k->feats[*(const int *)b];
    int cx = feat_cat_of(x->name), cy = feat_cat_of(y->name);
    return cx != cy ? cx - cy : strcmp(x->name, y->name);
}

int my_spells(const Character *k, int *order)
{
    for (int i = 0; i < k->nSpells; i++)
        order[i] = i;
    sort_k = k;
    qsort(order, k->nSpells, sizeof order[0], cmp_spells);
    return k->nSpells;
}

int my_feats(const Character *k, int *order)
{
    for (int i = 0; i < k->nFeats; i++)
        order[i] = i;
    sort_k = k;
    qsort(order, k->nFeats, sizeof order[0], cmp_feats);
    return k->nFeats;
}

// What the focused button would expand to (PEEK_*), or false.
static bool peek_for(const Character *k, const Btn *b, Peek *out)
{
    if (!b)
        return false;
    int kind = BKIND(b->id), i = BINDEX(b->id);
    memset(out, 0, sizeof *out);
    if (A.tab == TAB_MAGIC && kind == B_SROW) {
        int order[MAX_SPELLS] = { 0 };
        int n = my_spells(k, order);
        if (i >= n)
            return false;
        out->kind = PEEK_SPELL;
        text_copy(out->name, k->spells[order[i]].name, SPELL_NAME_LEN);
        return true;
    }
    if (A.tab == TAB_FEATS && (kind == B_FROW || kind == B_FTRACK)) {
        int order[MAX_FEATS] = { 0 }, tracked[4], nt = 0;
        int n = my_feats(k, order);
        for (int j = 0; j < n && nt < 4; j++)
            if (features_max_uses(k, features_find(&A.flib, k->feats[order[j]].name)) > 0)
                tracked[nt++] = order[j];
        int fi = kind == B_FROW ? (i < n ? order[i] : -1) : (i < nt ? tracked[i] : -1);
        if (fi < 0)
            return false;
        out->kind = PEEK_FEAT;
        text_copy(out->name, k->feats[fi].name, FEAT_NAME_LEN);
        return true;
    }
    if (A.tab == TAB_ARMS && kind == B_WEAPON && i < k->nWeapons) {
        out->kind = PEEK_WEAPON;
        out->idx = i;
        return true;
    }
    return false;
}

static void sheet_top(Surface *s, const Ctx *cx)
{
    const Character *k = ch();
    static Summary sum; // static: ~4 KB
    Peek pk;
    if (A.peek.active) {
        Peek *P = &A.peek;
        if (peek_for(k, ctx_focused(cx), &pk) && pk.kind == P->kind &&
            (strcmp(pk.name, P->name) != 0 || pk.idx != P->idx)) {
            text_copy(P->name, pk.name, sizeof P->name - 1);
            P->idx = pk.idx;
            P->scroll = 0;
        }
        if (!summary_for(k, P->kind, P->name, P->idx, &sum)) {
            P->active = false;
            top_sheet(s, NULL, NULL);
            return;
        }
        top_page(s, &sum, &P->scroll, &P->maxScroll);
        return;
    }
    const Btn *b = ctx_focused(cx);
    if (peek_for(k, b, &pk) && summary_for(k, pk.kind, pk.name, pk.idx, &sum)) {
        strcpy(sum.hint, BKIND(b->id) == B_FTRACK ? "A: USE  X: FULL SCREEN" : "X: FULL SCREEN");
        top_sheet(s, NULL, &sum);
        return;
    }
    top_sheet(s, NULL, NULL);
}

static bool (*tab_keys)(const Ctx *cx, const Input *in); // the current tab's own keys

static bool sheet_keys(const Ctx *cx, const Input *in)
{
    Character *k = ch();
    if (A.peek.active) {
        if (in->down & (KBIT(K_B) | KBIT(K_X))) {
            A.peek.active = false;
            return true;
        }
        if (in->rep & KBIT(K_L)) {
            A.peek.scroll = A.peek.scroll - 2 > 0 ? A.peek.scroll - 2 : 0;
            return true;
        }
        if (in->rep & KBIT(K_R)) {
            A.peek.scroll = A.peek.scroll + 2 < A.peek.maxScroll ? A.peek.scroll + 2 : A.peek.maxScroll;
            return true;
        }
    } else if (in->down & KBIT(K_X)) {
        Peek pk;
        if (peek_for(k, ctx_focused(cx), &pk)) {
            A.peek = pk;
            A.peek.active = true;
            A.peek.scroll = 0;
            return true;
        }
    }
    if (tab_keys && tab_keys(cx, in))
        return true;
    if (in->down & (KBIT(K_L) | KBIT(K_R))) {
        int p = 0;
        while (p < TAB_COUNT && TAB_ORDER[p] != A.tab)
            p++;
        A.tab = TAB_ORDER[(p + ((in->down & KBIT(K_L)) ? TAB_COUNT - 1 : 1)) % TAB_COUNT];
        A.peek.active = false;
        return true;
    }
    if (in->down & KBIT(K_START)) {
        open_menu();
        return true;
    }
    if (in->down & KBIT(K_SELECT)) {
        if (A.tab == TAB_EDIT) {
            A.tab = A.prevTab;
        } else {
            A.prevTab = A.tab;
            A.tab = TAB_EDIT;
        }
        return true;
    }
    if (in->down & KBIT(K_X)) {
        if (A.tab == TAB_EDIT)
            A.editPage = (A.editPage + 1) % 3;
        else if (A.tab == TAB_FEATS)
            open_feat_picker();
        else if (A.tab == TAB_MAGIC)
            open_spell_picker();
        else if (A.tab == TAB_ARMS)
            add_weapon_flow();
        return true;
    }
    if (in->down & KBIT(K_B)) {
        if (A.tab == TAB_FIGHT && A.amt[0])
            A.amt[0] = 0;
        return true;
    }
    return false;
}

static void tab_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    (void)focus;
    int i = b->a;
    bool on = A.tab == i;
    uint8_t col = on ? C_INK : C_DIM;
    d_rect(s, b->x, 0, b->w - 1, 16, on ? (i == TAB_EDIT ? C_BLUE : C_GOLD) : press ? C_PANEL2 : C_PANEL);
    const char *n = b->label;
    int x0 = b->x + floordiv(b->w - 1 - (n[0] ? 10 + d_tw(n, 1) : 7), 2);
    uint8_t ic = on ? C_INK : i == TAB_FIGHT ? C_RED : i == TAB_MAGIC ? C_BLUE : i == TAB_FEATS ? C_GOLD : C_DIM;
    d_icon(s, b->icon, x0, 4, ic, C_NONE, C_NONE, 1);
    d_text(s, n, x0 + 10, 5, col, 1);
}

static void tab_act(const Btn *b)
{
    A.tab = b->a;
    A.peek.active = false;
}

void view_sheet(Ctx *cx)
{
    static const struct {
        const char *label;
        int icon, tab;
    } TABS[TAB_COUNT] = {
        { "SKILLS", IC_MPAGE, TAB_SKILLS }, { "FEATS", IC_MBOOK, TAB_FEATS }, { "MAGIC", IC_MSPARK, TAB_MAGIC },
        { "ARMS", IC_MSWORD, TAB_ARMS },    { "FIGHT", IC_MHEART, TAB_FIGHT }, { "", IC_MQUILL, TAB_EDIT },
    };
    int ws[TAB_COUNT], total = 0;
    for (int p = 0; p < TAB_COUNT; p++) {
        ws[p] = TABS[p].label[0] ? 10 + d_tw(TABS[p].label, 1) + 9 : 21;
        total += ws[p];
    }
    int extra = SCREEN_W - total; // negative: the labels need 261 px
    ws[0] += extra - 4 * floordiv(extra, 5);
    for (int j = 1; j < 5; j++)
        ws[j] += floordiv(extra, 5);
    int x = 0;
    for (int p = 0; p < TAB_COUNT; p++) {
        Btn *b = ctx_add(cx, BID(B_TAB, TABS[p].tab), x, 0, ws[p], 16);
        b->nav = false;
        b->draw = tab_draw;
        b->act = tab_act;
        b->a = TABS[p].tab;
        b->label = TABS[p].label;
        b->icon = (int8_t)TABS[p].icon;
        x += ws[p];
    }
    Character *k = ch();
    int page = A.tab == TAB_EDIT ? A.editPage : 0;
    cx->key = 0x100u | (uint32_t)(A.tab << 4) | (uint32_t)page;
    cx->focus = &A.focusTab[A.tab][page];
    switch (A.tab) {
    case TAB_SKILLS:
        tab_skills(cx, k);
        break;
    case TAB_FIGHT:
        tab_fight(cx, k);
        break;
    case TAB_ARMS:
        tab_arms(cx, k);
        break;
    case TAB_MAGIC:
        tab_magic(cx, k);
        break;
    case TAB_FEATS:
        tab_feats(cx, k);
        break;
    default:
        tab_edit(cx, k);
        break;
    }
    tab_keys = cx->onKeys;
    cx->onKeys = sheet_keys;
    cx->top = sheet_top;
    if (A.peek.active)
        ctx_footer(cx, G_TRI_UP G_TRI_DOWN, "BROWSE", "L/R", "SCROLL", "X/B", "CLOSE", (const char *)NULL);
}
