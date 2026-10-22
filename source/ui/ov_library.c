// Spell details, the spell picker, slot maximums and the feature picker.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/actions.h"
#include "core/text.h"
#include "ui.h"

static void wrap_block(Surface *s, const char *const *paras, int n, int dimFrom)
{
    static TextLine wl[16];
    int line = 0;
    for (int p = 0; p < n; p++) {
        int c = text_wrap(paras[p], (int)strlen(paras[p]), 40, wl, 16);
        for (int i = 0; i < c && i < 16; i++, line++)
            d_textn(s, wl[i].p, wl[i].len, 8, 50 + line * 11, line < dimFrom ? C_TEXT : C_DIM, 1);
        line++; // the blank line after each paragraph
    }
}

static void remove_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_PANEL);
    d_text(s, G_CROSS, b->x + 6, b->y + 7, press ? C_INK : C_RED, 1);
    d_text(s, b->label, b->x + 16, b->y + 7, press ? C_INK : C_TEXT, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void pop_act(const Btn *b)
{
    (void)b;
    ov_pop();
}

static bool scroll_keys(Overlay *o, const Input *in)
{
    if (in->rep & KBIT(K_L)) {
        o->scroll = o->scroll - 2 > 0 ? o->scroll - 2 : 0;
        return true;
    }
    if (in->rep & KBIT(K_R)) {
        o->scroll = o->scroll + 2 < o->maxScroll ? o->scroll + 2 : o->maxScroll;
        return true;
    }
    return false;
}

static void filter_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    bool on = b->b == ov_top()->pick.filter;
    d_rect(s, b->x, b->y, b->w, b->h, on ? C_GOLD : press ? C_PANEL2 : C_PANEL);
    d_text_c(s, b->label, b->x, b->w, b->y + 5, on ? C_INK : C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void filter_act(const Btn *b)
{
    ov_top()->pick.filter = b->b;
    A.scroll[BKIND(b->id) == B_PFILTER ? LIST_PICK : LIST_FPICK] = 0;
}

// ---------- spell details ----------

void open_spell(int i)
{
    Overlay *o = ov_push(OV_SPELL);
    int order[MAX_SPELLS] = { 0 };
    const Character *k = ch();
    o->spell.count = my_spells(k, order);
    for (int j = 0; j < o->spell.count; j++)
        text_copy(o->spell.names[j], k->spells[order[j]].name, SPELL_NAME_LEN);
    o->spell.i = i;
}

static const char *spell_name(const Overlay *o)
{
    return o->spell.names[o->spell.i];
}

static void prep_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Overlay *o = ov_top();
    const Character *k = ch();
    int si = character_find_spell(k, spell_name(o)), lv = spell_level_of(spell_name(o));
    bool prep = si >= 0 && k->spells[si].prep;
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    if (lv == 0) {
        d_text(s, "CANTRIP: ALWAYS READY", b->x + 8, b->y + 9, C_DIM, 1);
    } else {
        d_text(s, "PREPARED", b->x + 8, b->y + 9, C_TEXT, 1);
        d_text_r(s, prep ? "YES " G_DOT : "NO " G_RING, b->x + b->w - 8, b->y + 9, prep ? C_GOLD : C_DIM, 1);
    }
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void prep_act(const Btn *b)
{
    (void)b;
    toggle_prepared(spell_name(ov_top()));
}

static int slots_left(const Character *k, int lv)
{
    return lv > 0 ? k->slotMax[lv - 1] - k->slotUsed[lv - 1] : 0;
}

static void use_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    int lv = spell_level_of(spell_name(ov_top())), left = slots_left(k, lv);
    bool ok = lv > 0 && left > 0;
    char t[48];
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : ok ? C_BLUE : C_PANEL);
    if (lv <= 0)
        snprintf(t, sizeof t, "NO SLOT NEEDED");
    else if (!k->slotMax[lv - 1])
        snprintf(t, sizeof t, "NO %s LEVEL SLOTS SET", ORD_NAMES[lv - 1]);
    else if (left)
        snprintf(t, sizeof t, "USE A %s LEVEL SLOT (%d LEFT)", ORD_NAMES[lv - 1], left);
    else
        snprintf(t, sizeof t, "NO %s LEVEL SLOTS LEFT", ORD_NAMES[lv - 1]);
    d_text_c(s, t, b->x, b->w, b->y + 9, press || ok ? C_INK : C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void use_act(const Btn *b)
{
    (void)b;
    Character *k = ch();
    const char *name = spell_name(ov_top());
    int lv = spell_level_of(name), left = slots_left(k, lv);
    if (lv > 0 && left > 0) {
        k->slotUsed[lv - 1]++;
        A.dirty = true;
        toastf(C_BLUE, "CAST %s: %d %s LEFT", name, left - 1, ORD_NAMES[lv - 1]);
    } else if (lv > 0) {
        toastf(C_GOLD, "NO %s LEVEL SLOTS LEFT", ORD_NAMES[lv - 1]);
    }
}

static void spell_nav_act(const Btn *b)
{
    Overlay *o = ov_top();
    int n = o->spell.count;
    switch (BKIND(b->id)) {
    case B_SPUP:
        o->scroll = o->scroll - 5 > 0 ? o->scroll - 5 : 0;
        break;
    case B_SPDN:
        o->scroll = o->scroll + 5 < o->maxScroll ? o->scroll + 5 : o->maxScroll;
        break;
    case B_SPPREV:
        o->spell.i = (o->spell.i + n - 1) % n;
        o->scroll = 0;
        break;
    default:
        o->spell.i = (o->spell.i + 1) % n;
        o->scroll = 0;
        break;
    }
}

static void spell_remove_act(const Btn *b)
{
    (void)b;
    const char *name = spell_name(ov_top());
    Notice *n = notice_push();
    snprintf(n->title, sizeof n->title, "REMOVE %s?", name);
    n->err = true;
    snprintf(n->sub, sizeof n->sub, "FROM THIS CHARACTER");
    snprintf(n->lines[0], sizeof n->lines[0], "REMOVES THE SPELL FROM THIS CHARACTER.");
    snprintf(n->lines[1], sizeof n->lines[1], "IT STAYS IN YOUR LIBRARY, SO YOU CAN ADD IT BACK.");
    n->nlines = 2;
    n->btns[0] = (NoticeBtn){ "KEEP", NB_PLAIN, NA_CLOSE };
    n->btns[1] = (NoticeBtn){ "REMOVE", NB_DANGER, NA_REMOVE_SPELL };
    n->nbtns = 2;
    n->def = 0;
    n->cancel = 0;
    text_copy(n->name, name, sizeof n->name - 1);
}

static void spell_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    char buf[48];
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, "SPELL", 6, 5, C_GOLD, 1);
    snprintf(buf, sizeof buf, "%d OF %d", o->spell.i + 1, o->spell.count);
    d_text_r(s, buf, 250, 5, C_DIM, 1);
    d_text_c(s, "READ THE FULL SPELL ON THE TOP SCREEN", 0, SCREEN_W, 141, C_DIM, 1);
}

static void spell_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    Overlay *o = ov_top();
    top_spell(s, spell_name(o), spells_find(&A.lib, spell_name(o)), &o->scroll, &o->maxScroll);
}

static bool spell_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    Overlay *o = ov_top();
    if (in->down & KBIT(K_B)) {
        ov_pop();
        return true;
    }
    if (scroll_keys(o, in))
        return true;
    if (in->down & KBIT(K_Y)) {
        toggle_prepared(spell_name(o));
        return true;
    }
    return false;
}

void ov_spell_build(Ctx *cx, Overlay *o)
{
    if (o->spell.count == 0 || character_find_spell(ch(), spell_name(o)) < 0) {
        ov_pop(); // the spell was removed
        ctx_now(cx);
        return;
    }
    Btn *b = ctx_add(cx, BID(B_SPPREP, 0), 3, 20, 250, 24);
    b->draw = prep_draw;
    b->act = prep_act;
    b = ctx_add(cx, BID(B_SPUSE, 0), 3, 48, 250, 24);
    b->draw = use_draw;
    b->act = use_act;
    add_plain(cx, BID(B_SPUP, 0), 3, 76, 123, 30, G_TRI_UP " SCROLL", C_PANEL2, C_TEXT, 1)->act = spell_nav_act;
    add_plain(cx, BID(B_SPDN, 0), 130, 76, 123, 30, "SCROLL " G_TRI_DOWN, C_PANEL2, C_TEXT, 1)->act = spell_nav_act;
    if (o->spell.count > 1) {
        add_plain(cx, BID(B_SPPREV, 0), 3, 110, 123, 24, G_TRI_LEFT " PREVIOUS", C_PANEL2, C_TEXT, 1)->act =
            spell_nav_act;
        add_plain(cx, BID(B_SPNEXT, 0), 130, 110, 123, 24, "NEXT " G_TRI_RIGHT, C_PANEL2, C_TEXT, 1)->act =
            spell_nav_act;
    }
    b = ctx_add(cx, BID(B_SPREM, 0), 3, 156, 100, 20);
    b->draw = remove_draw;
    b->label = "REMOVE...";
    b->act = spell_remove_act;
    add_plain(cx, BID(B_SPBACK, 0), 153, 156, 100, 20, "BACK", C_GOLD, C_INK, 1)->act = pop_act;
    cx->def = BID(B_SPPREP, 0);
    ctx_footer(cx, "A", "PRESS", "L/R", "SCROLL", "Y", "PREPARE", "B", "BACK", (const char *)NULL);
    cx->bot = spell_bot;
    cx->top = spell_top;
    cx->onKeys = spell_keys;
}

// ---------- spell picker ----------

static int pickItems[SPELL_LIB_MAX], npick;
static List pickList;

static int cmp_lib_spells(const void *a, const void *b)
{
    const Spell *x = &A.lib.items[*(const int *)a], *y = &A.lib.items[*(const int *)b];
    return x->level != y->level ? x->level - y->level : strcmp(x->name, y->name);
}

static const char *pick_name(int i)
{
    return A.lib.items[pickItems[i]].name;
}

static bool pick_known(int i)
{
    return character_find_spell(ch(), pick_name(i)) >= 0;
}

void open_spell_picker(void)
{
    A.scroll[LIST_PICK] = 0;
    Overlay *o = ov_push(OV_SPELLPICK);
    o->pick.filter = -1;
}

static void pick_act(const Btn *b)
{
    const char *name = pick_name(b->a);
    PickResult r = act_toggle_spell(ch(), name);
    if (r == PICK_FULL) {
        toastf(C_GOLD, "SPELL LIST FULL (%d)", MAX_SPELLS);
        return;
    }
    A.dirty = true;
    toastf(r == PICK_ADDED ? C_BLUE : C_DIM, "%s %s", r == PICK_ADDED ? "ADDED" : "REMOVED", name);
}

static void pick_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    char buf[48];
    if (!npick) {
        d_rect(s, 3, 37, 250, 128, C_PANEL);
        d_text_c(s, "NO SPELLS AT THIS LEVEL", 3, 250, 90, C_TEXT, 1);
        d_text_c(s, "IN YOUR LIBRARY", 3, 250, 102, C_DIM, 1);
    }
    snprintf(buf, sizeof buf, "%d/%d KNOWN", ch()->nSpells, MAX_SPELLS);
    d_text(s, buf, 4, 169, C_DIM, 1);
    const LibInfo *li = &A.libInfo;
    if (li->state && li->skipped)
        snprintf(buf, sizeof buf, "SPELLS.TXT +%d (%d SKIPPED)", li->added, li->skipped);
    else if (li->state)
        snprintf(buf, sizeof buf, "SPELLS.TXT +%d", li->added);
    else
        snprintf(buf, sizeof buf, "BUILT-IN ONLY");
    d_text_r(s, buf, 252, 169, li->skipped ? C_GOLD : C_DIM, 1);
}

static void pick_top(Surface *s, const Ctx *cx)
{
    Overlay *o = ov_top();
    uint16_t f = *cx->focus;
    if (BKIND(f) == B_PROW && BINDEX(f) < npick) {
        const Spell *sp = &A.lib.items[pickItems[BINDEX(f)]];
        if (strcmp(o->pick.lastF, sp->name) != 0) {
            text_copy(o->pick.lastF, sp->name, sizeof o->pick.lastF - 1);
            o->scroll = 0;
        }
        top_spell(s, sp->name, sp, &o->scroll, &o->maxScroll);
        return;
    }
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_text_c(s, "ADD SPELLS", 0, SCREEN_W, 20, C_GOLD, 2);
    char p2[96];
    if (A.libInfo.state)
        snprintf(p2, sizeof p2, "LIBRARY: %d SPELLS (%d BUILT IN, %d FROM SPELLS.TXT).", A.lib.count,
                 SPELL_BUILTIN_COUNT, A.libInfo.added);
    else
        snprintf(p2, sizeof p2, "LIBRARY: %d SPELLS (%d BUILT IN).", A.lib.count, SPELL_BUILTIN_COUNT);
    const char *paras[3] = { "PICK A LEVEL, THEN PRESS A ON A SPELL TO ADD OR REMOVE IT. THE FULL TEXT SHOWS HERE.",
                             p2, "TO ADD MORE, EDIT /DATA/POCKETSHEET/SPELLS.TXT ON A PC. THE FILE EXPLAINS THE FORMAT." };
    wrap_block(s, paras, 3, 3);
    if (A.toast.t > 0)
        strip(s, A.toast.text, A.toast.col);
}

static bool pick_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    Overlay *o = ov_top();
    if (in->down & KBIT(K_B)) {
        ov_pop();
        return true;
    }
    if (scroll_keys(o, in))
        return true;
    return list_keys(&pickList, in);
}

void ov_spellpick_build(Ctx *cx, Overlay *o)
{
    static const char *const LABELS[11] = { "ALL", "C", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    for (int j = 0; j < 11; j++) {
        Btn *b = ctx_add(cx, BID(B_PFILTER, j), 3 + j * 23, 18, 21, 16);
        b->draw = filter_draw;
        b->act = filter_act;
        b->label = LABELS[j];
        b->b = j - 1;
    }
    npick = 0;
    for (int i = 0; i < A.lib.count; i++)
        if (o->pick.filter < 0 || A.lib.items[i].level == o->pick.filter)
            pickItems[npick++] = i;
    qsort(pickItems, (size_t)npick, sizeof pickItems[0], cmp_lib_spells);
    spell_row_name = pick_name;
    spell_row_known = pick_known;
    pickList = (List){ .kind = B_PROW, .focus = cx->focus, .scroll = &A.scroll[LIST_PICK], .n = npick, .cols = 1,
                       .rows = 8, .x = 3, .y = 37, .w = 226, .h = 16, .draw = spell_row_draw, .act = pick_act };
    list_build(cx, &pickList);
    cx->def = npick ? BID(B_PROW, 0) : BID(B_PFILTER, 0);
    ctx_footer(cx, "A", "ADD/REMOVE", "L/R", "READ", "B", "DONE", (const char *)NULL);
    cx->bot = pick_bot;
    cx->top = pick_top;
    cx->onKeys = pick_keys;
}

// ---------- slot maximums ----------

void open_slotset(void)
{
    ov_push(OV_SLOTSET);
}

static void slotrow_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    int l = b->a, m = ch()->slotMax[l];
    char buf[16];
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    d_text(s, ORD_NAMES[l], b->x + 5, b->y + 11, C_DIM, 1);
    snprintf(buf, sizeof buf, "MAX %d", m);
    d_text(s, buf, b->x + 32, b->y + 11, m ? C_GOLD : C_LINE, 1);
    d_text(s, G_TRI_RIGHT, b->x + 114, b->y + 11, C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void slotrow_act(const Btn *b)
{
    char title[32];
    snprintf(title, sizeof title, "%s LEVEL SLOTS", ORD_NAMES[b->a]);
    open_stepper(title, SF_SLOTMAX, b->a, 0, 4, 1, 5);
}

static void slotset_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, "SPELL SLOTS: MAXIMUMS", 6, 5, C_GOLD, 1);
}

static void slotset_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    top_sheet(s, "SET HOW MANY SLOTS YOU HAVE AT EACH LEVEL", NULL);
}

static bool slotset_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    if (in->down & (KBIT(K_B) | KBIT(K_START))) {
        ov_pop();
        return true;
    }
    return false;
}

void ov_slotset_build(Ctx *cx, Overlay *o)
{
    (void)o;
    for (int l = 0; l < 9; l++) {
        Btn *b = ctx_add(cx, BID(B_SLOTROW, l), 3 + (l < 5 ? 0 : 126), 20 + (l < 5 ? l : l - 5) * 30, 124, 28);
        b->draw = slotrow_draw;
        b->act = slotrow_act;
        b->a = l;
    }
    add_plain(cx, BID(B_SSDONE, 0), 129, 140, 124, 28, "DONE", C_GOLD, C_INK, 1)->act = pop_act;
    cx->def = BID(B_SLOTROW, 0);
    cx->edit = true;
    ctx_footer(cx, "A", "SET MAX", "B", "DONE", (const char *)NULL);
    cx->bot = slotset_bot;
    cx->top = slotset_top;
    cx->onKeys = slotset_keys;
}

// ---------- feature picker ----------

static int fpickItems[FEAT_LIB_MAX], nfpick;
static List fpickList;

static int cmp_lib_feats(const void *a, const void *b)
{
    const Feature *x = &A.flib.items[*(const int *)a], *y = &A.flib.items[*(const int *)b];
    return x->cat != y->cat ? x->cat - y->cat : strcmp(x->name, y->name);
}

void open_feat_picker(void)
{
    A.scroll[LIST_FPICK] = 0;
    Overlay *o = ov_push(OV_FEATPICK);
    o->pick.filter = -1;
}

static void fpick_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Feature *f = &A.flib.items[fpickItems[b->a]];
    int h = b->h - 1, y = b->y + 4;
    d_rect(s, b->x, b->y, b->w, h, press ? C_PANEL2 : C_PANEL);
    d_rect(s, b->x + 2, b->y + 2, 19, 11, C_PANEL2);
    d_text_c(s, FCAT_SHORT[f->cat], b->x + 2, 19, y, C_DIM, 1);
    d_text(s, f->name, b->x + 25, y, C_TEXT, 1);
    if (character_find_feat(ch(), f->name) >= 0)
        d_text_r(s, G_CHECK, b->x + b->w - 4, y, C_GREEN, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, h);
}

static void fpick_act(const Btn *b)
{
    const char *name = A.flib.items[fpickItems[b->a]].name;
    PickResult r = act_toggle_feat(ch(), name);
    if (r == PICK_REMOVED)
        toastf(C_DIM, "REMOVED %s", name);
    else if (r == PICK_FULL)
        toastf(C_GOLD, "FEATURE LIST FULL (%d)", MAX_FEATS);
    else
        toastf(C_BLUE, "ADDED %s", name);
    A.dirty = true;
}

static void fpick_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    char buf[48];
    if (!nfpick) {
        d_rect(s, 3, 37, 250, 128, C_PANEL);
        d_text_c(s, "NOTHING IN THIS CATEGORY", 3, 250, 96, C_DIM, 1);
    }
    snprintf(buf, sizeof buf, "%d/%d ON SHEET", ch()->nFeats, MAX_FEATS);
    d_text(s, buf, 4, 169, C_DIM, 1);
    if (A.flibInfo.state)
        snprintf(buf, sizeof buf, "FEATURES.TXT +%d", A.flibInfo.added);
    else
        snprintf(buf, sizeof buf, "BUILT-IN ONLY");
    d_text_r(s, buf, 252, 169, C_DIM, 1);
}

static void fpick_top(Surface *s, const Ctx *cx)
{
    Overlay *o = ov_top();
    uint16_t f = *cx->focus;
    if (BKIND(f) == B_FPROW && BINDEX(f) < nfpick) {
        const Feature *fe = &A.flib.items[fpickItems[BINDEX(f)]];
        if (strcmp(o->pick.lastF, fe->name) != 0) {
            text_copy(o->pick.lastF, fe->name, sizeof o->pick.lastF - 1);
            o->scroll = 0;
        }
        top_feat(s, fe->name, fe, &o->scroll, &o->maxScroll);
        return;
    }
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_text_c(s, "ADD FEATURES", 0, SCREEN_W, 20, C_GOLD, 2);
    char p2[64];
    snprintf(p2, sizeof p2, "LIBRARY: %d ENTRIES (%d BUILT IN).", A.flib.count, FEATURE_BUILTIN_COUNT);
    const char *paras[3] = { "PICK A CATEGORY, THEN PRESS A TO ADD OR REMOVE. THE FULL TEXT SHOWS HERE.", p2,
                             "ADD YOUR OWN CLASS FEATURES, FEATS AND ITEM PROPERTIES IN "
                             "/DATA/POCKETSHEET/FEATURES.TXT ON A PC." };
    wrap_block(s, paras, 3, 1000);
    if (A.toast.t > 0)
        strip(s, A.toast.text, A.toast.col);
}

static bool fpick_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    Overlay *o = ov_top();
    if (in->down & KBIT(K_B)) {
        ov_pop();
        return true;
    }
    if (scroll_keys(o, in))
        return true;
    return list_keys(&fpickList, in);
}

void ov_featpick_build(Ctx *cx, Overlay *o)
{
    static const char *const LABELS[FCAT_COUNT + 1] = { "ALL", "CLS", "SUB", "SPC", "BKG", "FT", "ITM", "OTH" };
    for (int j = 0; j <= FCAT_COUNT; j++) {
        Btn *b = ctx_add(cx, BID(B_FPFILTER, j), 3 + j * 31, 18, 29, 16);
        b->draw = filter_draw;
        b->act = filter_act;
        b->label = LABELS[j];
        b->b = j - 1;
    }
    nfpick = 0;
    for (int i = 0; i < A.flib.count; i++)
        if (o->pick.filter < 0 || A.flib.items[i].cat == o->pick.filter)
            fpickItems[nfpick++] = i;
    qsort(fpickItems, (size_t)nfpick, sizeof fpickItems[0], cmp_lib_feats);
    fpickList = (List){ .kind = B_FPROW, .focus = cx->focus, .scroll = &A.scroll[LIST_FPICK], .n = nfpick,
                        .cols = 1, .rows = 8, .x = 3, .y = 37, .w = 226, .h = 16, .draw = fpick_draw,
                        .act = fpick_act };
    list_build(cx, &fpickList);
    cx->def = nfpick ? BID(B_FPROW, 0) : BID(B_FPFILTER, 0);
    ctx_footer(cx, "A", "ADD/REMOVE", "L/R", "READ", "B", "DONE", (const char *)NULL);
    cx->bot = fpick_bot;
    cx->top = fpick_top;
    cx->onKeys = fpick_keys;
}
