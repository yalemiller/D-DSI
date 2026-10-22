// FEATS tab: tracker chips for features with uses (A uses one, Y gives one
// back), then every feature in a 2-column list; A reads it on the top screen.
#include <stdio.h>
#include <string.h>

#include "core/actions.h"
#include "core/text.h"
#include "ui.h"

static int order[MAX_FEATS], norder;
static int tracked[4], ntracked;
static List featList;

static const Feature *lib_feat(int fi)
{
    return features_find(&A.flib, ch()->feats[fi].name);
}

static void spend_feat(const char *name, int delta)
{
    Character *k = ch();
    FeatUse r = act_feat_use(k, &A.flib, name, delta);
    int max = features_max_uses(k, features_find(&A.flib, name));
    int i = character_find_feat(k, name);
    switch (r) {
    case FEAT_UNTRACKED:
        return;
    case FEAT_NONE_LEFT:
        toastf(C_GOLD, "NO %s LEFT", name);
        return;
    case FEAT_FULL:
        toastf(C_DIM, "%s IS FULL", name);
        return;
    default:
        A.dirty = true;
        toastf(C_GOLD, "%s: %d OF %d LEFT", name, max - k->feats[i].used, max);
    }
}

static void pips(Surface *s, int x, int y, int max, int used, uint8_t col)
{
    if (max > 8) {
        char buf[24];
        snprintf(buf, sizeof buf, "%d/%d", max - used, max);
        d_text_r(s, buf, x, y, col, 1);
        return;
    }
    for (int i = 0; i < max; i++) {
        int px = x - (max - i) * 7 + 2;
        if (i < max - used)
            d_rect(s, px, y, 5, 7, col);
        else
            d_box(s, px, y, 5, 7, C_DIM, 1);
    }
}

static void chip_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    int fi = tracked[b->a];
    const Feature *f = lib_feat(fi);
    char name[FEAT_NAME_LEN + 1];
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    text_copy(name, k->feats[fi].name, 19);
    d_text(s, name, b->x + 5, b->y + 4, C_TEXT, 1);
    d_text(s, f ? RESET_NAMES[f->reset] : "", b->x + 5, b->y + 14, C_DIM, 1);
    pips(s, b->x + b->w - 5, b->y + 14, features_max_uses(k, f), k->feats[fi].used, C_GOLD);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void chip_act(const Btn *b)
{
    spend_feat(ch()->feats[tracked[b->a]].name, 1);
}

static void chip_y(const Btn *b)
{
    spend_feat(ch()->feats[tracked[b->a]].name, -1);
}

static void row_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    int fi = order[b->a];
    const KnownFeat *it = &k->feats[fi];
    const Feature *f = lib_feat(fi);
    int h = b->h - 1, y = b->y + 4, cat = f ? f->cat : -1;
    d_rect(s, b->x, b->y, b->w, h, press ? C_PANEL2 : C_PANEL);
    d_rect(s, b->x, b->y, 2, h, feat_cat_color(cat));
    int max = features_max_uses(k, f);
    char cnt[24] = "", name[FEAT_NAME_LEN + 2];
    if (max)
        snprintf(cnt, sizeof cnt, "%d/%d", max - it->used, max);
    int room = (b->w - 8 - (cnt[0] ? d_tw(cnt, 1) + 4 : 0)) / 6;
    text_copy(name, it->name, FEAT_NAME_LEN);
    if ((int)strlen(name) > room && room > 0) { // "LONG NAM." like the prototype
        name[room - 1] = '.';
        name[room] = 0;
    }
    d_text(s, name, b->x + 5, y, cat < 0 ? C_RED : C_TEXT, 1);
    if (cnt[0])
        d_text_r(s, cnt, b->x + b->w - 3, y, C_GOLD, 1);
    if (A.peek.active && A.peek.kind == PEEK_FEAT && !strcmp(A.peek.name, it->name))
        d_box(s, b->x, b->y, b->w, h, C_BLUE, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, h);
}

static void row_act(const Btn *b)
{
    memset(&A.peek, 0, sizeof A.peek);
    A.peek.active = true;
    A.peek.kind = PEEK_FEAT;
    text_copy(A.peek.name, ch()->feats[order[b->a]].name, FEAT_NAME_LEN);
}

static void add_act(const Btn *b)
{
    (void)b;
    open_feat_picker();
}

static bool feats_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    return list_keys(&featList, in);
}

static void feats_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    char buf[48];
    if (!norder) {
        d_rect(s, 3, 18, 250, 140, C_PANEL);
        d_text_c(s, "NO FEATURES YET", 3, 250, 70, C_TEXT, 1);
        d_text_c(s, "TAP + ADD (OR PRESS X) TO PICK CLASS", 3, 250, 86, C_DIM, 1);
        d_text_c(s, "FEATURES, FEATS, TRAITS AND ITEMS.", 3, 250, 96, C_DIM, 1);
    }
    if (ntracked)
        snprintf(buf, sizeof buf, "%d FEATURES, %d TRACKED", norder, ntracked);
    else
        snprintf(buf, sizeof buf, "%d FEATURES", norder);
    d_text(s, buf, 4, 168, C_DIM, 1);
}

void tab_feats(Ctx *cx, Character *k)
{
    norder = my_feats(k, order);
    ntracked = 0;
    for (int j = 0; j < norder && ntracked < 4; j++)
        if (features_max_uses(k, lib_feat(order[j])) > 0)
            tracked[ntracked++] = order[j];
    for (int j = 0; j < ntracked; j++) {
        Btn *b = ctx_add(cx, BID(B_FTRACK, j), 3 + (j % 2) * 126, 18 + (j / 2) * 26, 124, 24);
        b->draw = chip_draw;
        b->act = chip_act;
        b->onY = chip_y;
        b->a = j;
    }
    int rowsT = (ntracked + 1) / 2;
    int y0 = 18 + rowsT * 26 + (rowsT ? 1 : 0), nrows = (162 - y0) / 16;
    if (nrows < 3)
        nrows = 3;
    featList = (List){ .kind = B_FROW, .focus = cx->focus, .scroll = &A.scroll[LIST_FEATS], .n = norder, .cols = 2,
                       .rows = nrows, .x = 3, .y = y0, .w = 226, .h = 16, .draw = row_draw, .act = row_act };
    list_build(cx, &featList);
    Btn *b = add_plain(cx, BID(B_FADD, 0), 181, 164, 72, 14, "+ ADD", C_PANEL2, C_TEXT, 1);
    b->act = add_act;
    cx->def = ntracked ? BID(B_FTRACK, 0) : norder ? BID(B_FROW, 0) : BID(B_FADD, 0);
    cx->onKeys = feats_keys;
    if (BKIND(*cx->focus) == B_FTRACK)
        ctx_footer(cx, "A", "USE", "Y", "RESTORE", "X", "EXPAND", "L/R", "TABS", (const char *)NULL);
    else
        ctx_footer(cx, "A", "READ", "X", "EXPAND", "L/R", "TABS", "ST", "MENU", (const char *)NULL);
    cx->bot = feats_bot;
}
