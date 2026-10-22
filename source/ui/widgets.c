// Buttons, focus, geometric D-pad navigation, scrolling lists and the shared
// button drawers (the prototype's plain(), iconBtn(), focusBox(), listRows()).
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/text.h"
#include "ui.h"

Btn *ctx_add(Ctx *cx, uint16_t id, int x, int y, int w, int h)
{
    static Btn overflow; // never drawn: keeps callers safe if MAX_BTNS is ever exceeded
    if (cx->n >= MAX_BTNS) {
        memset(&overflow, 0, sizeof overflow);
        return &overflow;
    }
    Btn *b = &cx->btns[cx->n++];
    memset(b, 0, sizeof *b);
    b->id = id;
    b->x = (int16_t)x;
    b->y = (int16_t)y;
    b->w = (int16_t)w;
    b->h = (int16_t)h;
    b->nav = true;
    b->icon = -1;
    b->bg = C_PANEL2;
    b->fg = C_TEXT;
    b->sc = 1;
    b->ic0 = b->ic1 = b->ic2 = C_NONE;
    return b;
}

void ctx_footer(Ctx *cx, const char *k1, const char *l1, ...)
{
    cx->nfooter = 0;
    const char *k = k1, *l = l1;
    va_list ap;
    va_start(ap, l1);
    while (k && cx->nfooter < 5) {
        cx->footer[cx->nfooter++] = (Hint){ k, l };
        k = va_arg(ap, const char *);
        if (!k)
            break;
        l = va_arg(ap, const char *);
    }
    va_end(ap);
}

const char *ctx_str(Ctx *cx, const char *fmt, ...)
{
    int room = (int)sizeof cx->arena - cx->arenaUsed;
    if (room <= 1)
        return "";
    char *p = cx->arena + cx->arenaUsed;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(p, (size_t)room, fmt, ap);
    va_end(ap);
    if (n < 0)
        n = 0;
    if (n >= room)
        n = room - 1;
    cx->arenaUsed += n + 1;
    return p;
}

// ---------- focus and navigation ----------

static bool navigable(const Btn *b)
{
    return b->nav && !b->disabled;
}

const Btn *ctx_find(const Ctx *cx, uint16_t id)
{
    for (int i = 0; i < cx->n; i++)
        if (cx->btns[i].id == id)
            return &cx->btns[i];
    return NULL;
}

uint16_t ctx_focus(const Ctx *cx)
{
    const Btn *first = NULL, *def = NULL;
    uint16_t cur = cx->focus ? *cx->focus : 0;
    for (int i = 0; i < cx->n; i++) {
        const Btn *b = &cx->btns[i];
        if (!navigable(b))
            continue;
        if (b->id == cur)
            return cur;
        if (!first)
            first = b;
        if (b->id == cx->def)
            def = b;
    }
    uint16_t d = def ? def->id : first ? first->id : 0;
    if (cx->focus)
        *cx->focus = d;
    return d;
}

const Btn *ctx_focused(const Ctx *cx)
{
    uint16_t id = ctx_focus(cx);
    return id ? ctx_find(cx, id) : NULL;
}

uint16_t nav_move(const Ctx *cx, uint16_t id, int dir)
{
    const Btn *c = ctx_find(cx, id);
    if (!c || !navigable(c)) {
        for (int i = 0; i < cx->n; i++)
            if (navigable(&cx->btns[i]))
                return cx->btns[i].id;
        return id;
    }
    // Scores are the prototype's p + gap * 3 + off * 0.3, scaled by 20 to stay in integers.
    int cx2 = 2 * c->x + c->w, cy2 = 2 * c->y + c->h;
    const Btn *best = NULL;
    long bestScore = 0;
    for (int i = 0; i < cx->n; i++) {
        const Btn *b = &cx->btns[i];
        if (b == c || !navigable(b))
            continue;
        int bx2 = 2 * b->x + b->w, by2 = 2 * b->y + b->h, p2, gap, off2;
        if (dir == K_LEFT || dir == K_RIGHT) {
            p2 = dir == K_LEFT ? cx2 - bx2 : bx2 - cx2;
            int lo = b->y > c->y ? b->y : c->y;
            int hi = b->y + b->h < c->y + c->h ? b->y + b->h : c->y + c->h;
            gap = lo - hi > 0 ? lo - hi : 0;
            off2 = abs(by2 - cy2);
        } else {
            p2 = dir == K_UP ? cy2 - by2 : by2 - cy2;
            int lo = b->x > c->x ? b->x : c->x;
            int hi = b->x + b->w < c->x + c->w ? b->x + b->w : c->x + c->w;
            gap = lo - hi > 0 ? lo - hi : 0;
            off2 = abs(bx2 - cx2);
        }
        if (p2 <= 2)
            continue;
        long score = (long)p2 * 10 + (long)gap * 60 + (long)off2 * 3;
        if (!best || score < bestScore) {
            best = b;
            bestScore = score;
        }
    }
    return best ? best->id : id;
}

const Btn *ctx_hit(const Ctx *cx, int x, int y)
{
    for (int i = cx->n - 1; i >= 0; i--) {
        const Btn *b = &cx->btns[i];
        if (!b->disabled && x >= b->x && x < b->x + b->w && y >= b->y && y < b->y + b->h)
            return b;
    }
    return NULL;
}

// ---------- drawers ----------

void draw_focus(Surface *s, int x, int y, int w, int h)
{
    d_box(s, x, y, w, h, C_GOLD, 2);
    d_box(s, x + 2, y + 2, w - 4, h - 4, C_INK, 1);
}

void btn_plain(Surface *s, const Btn *b, bool focus, bool press)
{
    uint8_t bg = press ? C_TEXT : b->bg, fg = press ? C_INK : b->fg;
    d_rect(s, b->x, b->y, b->w, b->h, bg);
    d_text_c(s, b->label, b->x, b->w, b->y + floordiv(b->h - 7 * b->sc, 2), fg, b->sc);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

void btn_icon(Surface *s, const Btn *b, bool focus, bool press)
{
    uint8_t bg = b->bg, fg = b->fg;
    uint8_t ic0 = b->ic0 == C_NONE ? fg : b->ic0;
    if (press) {
        bg = C_TEXT;
        fg = C_INK;
        ic0 = C_INK;
    }
    uint8_t ic1 = b->ic1 == IC_USE_BG ? bg : press ? C_INK : b->ic1 == C_NONE ? ic0 : b->ic1;
    uint8_t ic2 = press ? C_INK : b->ic2 == C_NONE ? ic0 : b->ic2;
    d_rect(s, b->x, b->y, b->w, b->h, bg);
    int icon = b->iconFn ? b->iconFn() : b->icon;
    int iw = icon_w(icon), ih = icon_h(icon), sc = b->sc, lh = 7 * sc;
    int nl = b->label2 ? 2 : 1;
    int lw = d_tw(b->label, sc);
    if (b->label2 && d_tw(b->label2, sc) > lw)
        lw = d_tw(b->label2, sc);
    int x0 = b->x + floordiv(b->w - iw - 5 - lw, 2);
    d_icon(s, icon, x0, b->y + floordiv(b->h - ih, 2), ic0, ic1, ic2, 1);
    int th = nl * lh + (nl - 1) * 2, ty = b->y + floordiv(b->h - th, 2);
    d_text(s, b->label, x0 + iw + 5, ty, fg, sc);
    if (b->label2)
        d_text(s, b->label2, x0 + iw + 5, ty + lh + 2, fg, sc);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

void btn_row(Surface *s, const Btn *b, bool focus, bool press)
{
    int h = b->h - 1;
    d_rect(s, b->x, b->y, b->w, h, press ? C_PANEL2 : C_PANEL);
    d_text(s, b->label, b->x + 6, b->y + 4, C_DIM, 1);
    d_text_r(s, b->label2 ? b->label2 : "", b->x + b->w - (b->arrow ? 16 : 6), b->y + 4, C_GOLD, 1);
    if (b->arrow)
        d_text(s, G_TRI_RIGHT, b->x + b->w - 10, b->y + 4, C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, h);
}

Btn *add_plain(Ctx *cx, uint16_t id, int x, int y, int w, int h, const char *label, uint8_t bg, uint8_t fg, int sc)
{
    Btn *b = ctx_add(cx, id, x, y, w, h);
    b->draw = btn_plain;
    b->label = label;
    b->bg = bg;
    b->fg = fg;
    b->sc = (uint8_t)sc;
    return b;
}

void footer_draw(Surface *s, const Hint *hints, int n, bool edit)
{
    d_rect(s, 0, 180, SCREEN_W, 12, edit ? C_BLUE : C_PANEL);
    int x = 4;
    for (int i = 0; i < n; i++) {
        int w = d_tw(hints[i].key, 1) + 4;
        d_rect(s, x, 181, w, 9, edit ? C_INK : C_DIM);
        d_text(s, hints[i].key, x + 2, 182, edit ? C_BLUE : C_INK, 1);
        d_text(s, hints[i].label, x + w + 3, 182, edit ? C_INK : C_TEXT, 1);
        x += w + 3 + d_tw(hints[i].label, 1) + 8;
    }
}

void strip(Surface *s, const char *text, uint8_t col)
{
    d_rect(s, 0, 178, SCREEN_W, 14, col);
    d_text_c(s, text, 0, SCREEN_W, 182, C_INK, 1);
}

// ---------- scrolling lists ----------

static void scroll_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    (void)focus;
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_PANEL);
    d_text_c(s, b->label, b->x, b->w, b->y + floordiv(b->h - 7, 2), press ? C_INK : b->a ? C_TEXT : C_LINE, 1);
}

// Lists with page buttons in the context being built (reset by ctx_now()).
static List page_lists[4];
static int page_list_count;

void list_reset(void)
{
    page_list_count = 0;
}

static void scroll_act(const Btn *b)
{
    const List *L = &page_lists[b->b];
    int nr = (L->n + L->cols - 1) / L->cols;
    int maxTop = nr - L->rows > 0 ? nr - L->rows : 0;
    int nt = *L->scroll + (BKIND(b->id) == B_SCRUP ? -L->rows : L->rows);
    nt = nt < 0 ? 0 : nt > maxTop ? maxTop : nt;
    *L->scroll = nt;
    *L->focus = BID(L->kind, nt * L->cols);
}

void list_build(Ctx *cx, List *L)
{
    int cols = L->cols ? L->cols : 1;
    L->cols = cols;
    int n = L->n, nr = (n + cols - 1) / cols, gap = cols > 1 ? 2 : 0;
    int cw = (L->w - gap * (cols - 1)) / cols;
    int top = *L->scroll;
    uint16_t f = *L->focus;
    if (f && BKIND(f) == L->kind) {
        int fi = BINDEX(f);
        if (fi >= n) {
            *L->focus = n ? BID(L->kind, n - 1) : 0;
        } else {
            int r = fi / cols;
            if (r < top)
                top = r;
            if (r >= top + L->rows)
                top = r - L->rows + 1;
        }
    }
    int maxTop = nr - L->rows > 0 ? nr - L->rows : 0;
    top = top < 0 ? 0 : top > maxTop ? maxTop : top;
    *L->scroll = top;
    int end = (top + L->rows) * cols < n ? (top + L->rows) * cols : n;
    for (int i = top * cols; i < end; i++) {
        int r = i / cols - top, cc = i % cols;
        Btn *b = ctx_add(cx, BID(L->kind, i), L->x + cc * (cw + gap), L->y + r * L->h, cw, L->h);
        b->draw = L->draw;
        b->act = L->act;
        b->onY = L->onY;
        b->a = i;
    }
    if (nr > L->rows && page_list_count < 4) {
        int slot = page_list_count++;
        page_lists[slot] = *L;
        int hh = (L->rows * L->h) / 2 - 1;
        Btn *u = ctx_add(cx, BID(B_SCRUP, slot), L->x + L->w + 2, L->y, 22, hh);
        Btn *d = ctx_add(cx, BID(B_SCRDN, slot), L->x + L->w + 2, L->y + hh + 2, 22, hh);
        u->nav = d->nav = false;
        u->draw = d->draw = scroll_draw;
        u->act = d->act = scroll_act;
        u->label = G_TRI_UP;
        d->label = G_TRI_DOWN;
        u->a = top > 0;
        d->a = top + L->rows < nr;
        u->b = d->b = slot;
    }
}

bool list_keys(const List *L, const Input *in)
{
    uint16_t f = *L->focus;
    if (!f || BKIND(f) != L->kind)
        return false;
    int j = BINDEX(f), cols = L->cols ? L->cols : 1, n = L->n, nr = (n + cols - 1) / cols;
    if (in->rep & KBIT(K_DOWN)) {
        if (j + cols < n)
            *L->focus = BID(L->kind, j + cols);
        else if (j / cols < nr - 1)
            *L->focus = BID(L->kind, n - 1);
        return true;
    }
    if ((in->rep & KBIT(K_UP)) && j - cols >= 0) {
        *L->focus = BID(L->kind, j - cols);
        return true;
    }
    if (cols > 1 && (in->rep & KBIT(K_RIGHT)) && j % cols < cols - 1 && j + 1 < n) {
        *L->focus = BID(L->kind, j + 1);
        return true;
    }
    if (cols > 1 && (in->rep & KBIT(K_LEFT)) && j % cols > 0) {
        *L->focus = BID(L->kind, j - 1);
        return true;
    }
    return false;
}
