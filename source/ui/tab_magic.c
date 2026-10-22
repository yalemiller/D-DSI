// MAGIC tab: the slot strip (A spends, Y restores, SET opens the maximums)
// and the known-spell list (A details, Y prepares, X expands).
#include <stdio.h>
#include <string.h>

#include "core/actions.h"
#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

static int order[MAX_SPELLS], norder;
static int levels[9], nlevels;
static List spellList;

// One spell row, shared with the picker. b->b = 1 on the character's own list.
// The row's name comes from spell_row_name (set by whoever built the list).
const char *(*spell_row_name)(int i);
bool (*spell_row_known)(int i);

void spell_row_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const char *name = spell_row_name(b->a);
    const Spell *sp = spells_find(&A.lib, name);
    bool mine = b->b;
    int lv = sp ? sp->level : -1, h = b->h - 1, y = b->y + 4;
    char badge[12];
    d_rect(s, b->x, b->y, b->w, h, press ? C_PANEL2 : C_PANEL);
    d_rect(s, b->x + 2, b->y + 2, 11, 11, lv <= 0 ? C_LINE : C_PANEL2);
    snprintf(badge, sizeof badge, "%s", lv < 0 ? "?" : lv == 0 ? "C" : "");
    if (lv > 0)
        snprintf(badge, sizeof badge, "%d", lv);
    d_text_c(s, badge, b->x + 2, 11, y, lv < 0 ? C_RED : C_TEXT, 1);
    int x = b->x + 17;
    bool prep = false;
    if (mine) {
        int si = character_find_spell(ch(), name);
        prep = si >= 0 && ch()->spells[si].prep;
        if (lv > 0)
            d_text(s, prep ? G_DOT : G_RING, x, y, prep ? C_GOLD : C_LINE, 1);
        x += 9;
    }
    d_text(s, name, x, y, lv < 0 ? C_RED : mine && lv > 0 && !prep ? C_DIM : C_TEXT, 1);
    int r = b->x + b->w - 4;
    if (!mine && spell_row_known && spell_row_known(b->a)) {
        d_text_r(s, G_CHECK, r, y, C_GREEN, 1);
        r -= 10;
    }
    if (sp && sp->ritual) {
        d_text_r(s, "R", r, y, C_GOLD, 1);
        r -= 9;
    }
    if (sp && sp->conc)
        d_text_r(s, G_DIAMOND, r, y, C_BLUE, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, h);
}

static const char *mine_name(int i)
{
    return ch()->spells[order[i]].name;
}

static void slot_chip_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    int l = b->a, m = k->slotMax[l], av = m - k->slotUsed[l];
    char buf[12];
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    snprintf(buf, sizeof buf, "%d", l + 1);
    d_text_c(s, buf, b->x, b->w, b->y + 4, av ? C_TEXT : C_LINE, 1);
    int gw = m * 5 - 1, gx = b->x + floordiv(b->w - gw, 2);
    for (int i = 0; i < m; i++) {
        if (i < av)
            d_rect(s, gx + i * 5, b->y + 15, 4, 8, C_BLUE);
        else
            d_box(s, gx + i * 5, b->y + 15, 4, 8, C_DIM, 1);
    }
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void slot_chip_act(const Btn *b)
{
    Character *k = ch();
    int l = b->a;
    if (act_slot(k, l, 1)) {
        A.dirty = true;
        toastf(C_BLUE, "%s LEVEL: %d OF %d LEFT", ORD_NAMES[l], k->slotMax[l] - k->slotUsed[l], k->slotMax[l]);
    } else {
        toastf(C_GOLD, "NO %s LEVEL SLOTS LEFT. Y RESTORES ONE", ORD_NAMES[l]);
    }
}

static void slot_chip_y(const Btn *b)
{
    Character *k = ch();
    int l = b->a;
    if (act_slot(k, l, -1)) {
        A.dirty = true;
        toastf(C_BLUE, "%s LEVEL: RESTORED (%d LEFT)", ORD_NAMES[l], k->slotMax[l] - k->slotUsed[l]);
    }
}

static void set_act(const Btn *b)
{
    (void)b;
    open_slotset();
}

static void add_act(const Btn *b)
{
    (void)b;
    open_spell_picker();
}

static void row_act(const Btn *b)
{
    open_spell(b->a);
}

void toggle_prepared(const char *name)
{
    PrepResult r = act_toggle_prepared(ch(), &A.lib, name);
    if (r == PREP_CANTRIP) {
        toast("CANTRIPS ARE ALWAYS READY", C_BLUE);
    } else if (r != PREP_UNKNOWN) {
        A.dirty = true;
        toastf(r == PREP_ON ? C_GOLD : C_DIM, "%s %s", name, r == PREP_ON ? "PREPARED" : "UNPREPARED");
    }
}

static void row_y(const Btn *b)
{
    toggle_prepared(mine_name(b->a));
}

static bool magic_keys(const Ctx *cx, const Input *in)
{
    uint16_t f = *cx->focus;
    if (BKIND(f) != B_SROW)
        return false;
    int j = BINDEX(f);
    if (in->rep & KBIT(K_DOWN)) {
        if (j < norder - 1)
            *cx->focus = BID(B_SROW, j + 1);
        return true;
    }
    if (in->rep & KBIT(K_UP)) {
        if (j > 0)
            *cx->focus = BID(B_SROW, j - 1);
        else
            *cx->focus = nlevels ? BID(B_SLOTCHIP, levels[0]) : BID(B_SETSLOTS, 0);
        return true;
    }
    return false;
}

static void magic_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Character *k = ch();
    char buf[48];
    if (!norder) {
        d_rect(s, 3, 49, 250, 112, C_PANEL);
        d_text_c(s, "NO SPELLS YET", 3, 250, 86, C_TEXT, 1);
        d_text_c(s, "TAP + ADD (OR PRESS X) TO PICK", 3, 250, 102, C_DIM, 1);
        snprintf(buf, sizeof buf, "FROM YOUR LIBRARY OF %d SPELLS.", A.lib.count);
        d_text_c(s, buf, 3, 250, 112, C_DIM, 1);
    }
    int np = 0;
    for (int i = 0; i < k->nSpells; i++)
        if (k->spells[i].prep && spell_level_of(k->spells[i].name) > 0)
            np++;
    snprintf(buf, sizeof buf, "%d PREPARED", np);
    const char *g[3] = { G_DOT, G_DIAMOND, "R" }, *l[3] = { buf, "CONC", "RITUAL" };
    const uint8_t col[3] = { C_GOLD, C_BLUE, C_GOLD };
    int x = 4;
    for (int i = 0; i < 3; i++) {
        d_text(s, g[i], x, 169, col[i], 1);
        d_text(s, l[i], x + 8, 169, C_DIM, 1);
        x += 8 + d_tw(l[i], 1) + 12;
    }
}

void tab_magic(Ctx *cx, Character *k)
{
    norder = my_spells(k, order);
    spell_row_name = mine_name;
    nlevels = 0;
    for (int l = 0; l < 9; l++)
        if (k->slotMax[l] > 0)
            levels[nlevels++] = l;
    const int cw = 25, gap = 2;
    for (int j = 0; j < nlevels; j++) {
        Btn *b = ctx_add(cx, BID(B_SLOTCHIP, levels[j]), 3 + j * (cw + gap), 18, cw, 28);
        b->draw = slot_chip_draw;
        b->act = slot_chip_act;
        b->onY = slot_chip_y;
        b->a = levels[j];
    }
    Btn *b = add_plain(cx, BID(B_SETSLOTS, 0), 3 + nlevels * (cw + gap), 18, nlevels ? 34 : 120, 28,
                       nlevels ? "SET" : "SET SLOTS", C_PANEL2, C_TEXT, 1);
    b->act = set_act;
    b = add_plain(cx, BID(B_SPADD, 0), 205, 18, 48, 28, "+ ADD", C_PANEL2, C_TEXT, 1);
    b->act = add_act;
    spellList = (List){ .kind = B_SROW, .focus = cx->focus, .scroll = &A.scroll[LIST_SPELLS], .n = norder,
                        .cols = 1, .rows = 7, .x = 3, .y = 49, .w = 226, .h = 16, .draw = spell_row_draw,
                        .act = row_act, .onY = row_y };
    int before = cx->n;
    list_build(cx, &spellList);
    for (int i = before; i < cx->n; i++)
        if (BKIND(cx->btns[i].id) == B_SROW)
            cx->btns[i].b = 1; // own list: prepared marks
    cx->def = norder ? BID(B_SROW, 0) : nlevels ? BID(B_SLOTCHIP, levels[0]) : BID(B_SPADD, 0);
    cx->onKeys = magic_keys;
    if (BKIND(*cx->focus) == B_SLOTCHIP)
        ctx_footer(cx, "A", "SPEND", "Y", "RESTORE", "X", "ADD SPELL", "L/R", "TABS", (const char *)NULL);
    else if (norder)
        ctx_footer(cx, "A", "DETAILS", "Y", "PREPARE", "X", "EXPAND", "L/R", "TABS", (const char *)NULL);
    else
        ctx_footer(cx, "A", "PRESS", "X", "ADD SPELL", "L/R", "TABS", (const char *)NULL);
    cx->bot = magic_bot;
}
