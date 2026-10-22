// The frame loop: tick toasts and effects, build the current screen's
// buttons, then take at most one action (touch first, then keys).
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "core/text.h"
#include "ui.h"

App A;

static Ctx frameCtx, renderCtx; // static: a Ctx is ~5 KB and the DS stack is small

void app_init(const Storage *storage)
{
    spells_free(&A.lib);
    features_free(&A.flib);
    memset(&A, 0, sizeof A);
    A.storage = storage;
    A.fs = &storage->fs;
    A.theme = THEME_DEFAULT;
    memcpy(A.pal, THEMES[A.theme].c, sizeof A.pal);
    spells_init(&A.lib);
    features_init(&A.flib);
    A.view = VIEW_BOOT;
}

const uint16_t *app_palette(void)
{
    return A.pal;
}

const char *sg(int n)
{
    static char bufs[16][12];
    static int i;
    i = (i + 1) % 16;
    return text_signed(bufs[i], n);
}

void toast(const char *text, uint8_t col)
{
    snprintf(A.toast.text, sizeof A.toast.text, "%s", text);
    A.toast.col = col;
    A.toast.t = 110;
}

void toastf(uint8_t col, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(A.toast.text, sizeof A.toast.text, fmt, ap);
    va_end(ap);
    A.toast.col = col;
    A.toast.t = 110;
}

void fx_add(const Fx *f)
{
    int j = 0;
    for (int i = 0; i < A.nfx; i++)
        if (A.fx[i].kind != f->kind)
            A.fx[j++] = A.fx[i];
    A.nfx = j;
    if (A.nfx >= MAX_FX)
        return;
    Fx *n = &A.fx[A.nfx++];
    *n = *f;
    n->T = n->t;
    n->seed = (int)((A.frame * 7919u) % 997u);
}

const Fx *fx_find(int kind)
{
    for (int i = 0; i < A.nfx; i++)
        if (A.fx[i].kind == kind)
            return &A.fx[i];
    return NULL;
}

Overlay *ov_push(OvType type)
{
    if (A.nov >= MAX_OVERLAYS)
        A.nov = MAX_OVERLAYS - 1; // replace the top rather than overflow
    Overlay *o = &A.ov[A.nov++];
    memset(o, 0, sizeof *o);
    o->type = type;
    o->uid = ++A.uid;
    memset(&A.press, 0, sizeof A.press);
    return o;
}

void ov_pop(void)
{
    if (A.nov > 0)
        A.nov--;
    memset(&A.press, 0, sizeof A.press);
}

Overlay *ov_top(void)
{
    return A.nov ? &A.ov[A.nov - 1] : NULL;
}

void ov_clear(void)
{
    A.nov = 0;
}

Notice *notice_push(void)
{
    Overlay *o = ov_push(OV_NOTICE);
    o->notice.cancel = -1;
    return &o->notice;
}

void ctx_now(Ctx *cx)
{
    memset(cx, 0, sizeof *cx);
    list_reset();
    Overlay *o = ov_top();
    if (o) {
        ov_build(cx, o);
        return;
    }
    if (A.view == VIEW_BOOT)
        view_boot(cx);
    else if (A.view == VIEW_SELECT)
        view_select(cx);
    else
        view_sheet(cx);
}

static void act(const Btn *b)
{
    if (b && !b->disabled && b->act)
        b->act(b);
}

static bool touch_step(const Ctx *cx, const Input *in)
{
    switch (in->touch) {
    case TOUCH_DOWN: {
        const Btn *b = ctx_hit(cx, in->tx, in->ty);
        A.press.down = true;
        A.press.id = b ? b->id : 0;
        A.press.inside = b != NULL;
        A.press.ctx = cx->key;
        if (b && b->nav && cx->focus)
            *cx->focus = b->id;
        break;
    }
    case TOUCH_MOVE:
        if (A.press.down && A.press.id) {
            const Btn *b = ctx_hit(cx, in->tx, in->ty);
            A.press.inside = b && b->id == A.press.id;
        }
        break;
    case TOUCH_UP: {
        Press p = A.press;
        memset(&A.press, 0, sizeof A.press);
        if (p.down && p.id && p.ctx == cx->key) {
            const Btn *b = ctx_hit(cx, in->tx, in->ty);
            if (b && b->id == p.id) {
                act(b);
                return true;
            }
        }
        break;
    }
    case TOUCH_NONE:
        break;
    }
    return false;
}

static void step(const Input *in)
{
    if (A.view == VIEW_BOOT) {
        if (++A.t == 50)
            session_startup();
        return;
    }
    Ctx *cx = &frameCtx;
    ctx_now(cx);
    if (touch_step(cx, in))
        return;
    if (cx->onKeys && cx->onKeys(cx, in))
        return;
    static const int DIRS[4] = { K_UP, K_DOWN, K_LEFT, K_RIGHT };
    for (int d = 0; d < 4; d++) {
        if (in->rep & KBIT(DIRS[d])) {
            uint16_t id = nav_move(cx, ctx_focus(cx), DIRS[d]);
            if (id && cx->focus)
                *cx->focus = id;
            break;
        }
    }
    if (in->down & KBIT(K_A)) {
        act(ctx_focused(cx));
    } else if (in->down & KBIT(K_Y)) {
        const Btn *b = ctx_focused(cx);
        if (b && !b->disabled && b->onY)
            b->onY(b);
    }
}

bool app_frame(const Input *in)
{
    A.frame++;
    if (A.lid)
        return false;
    // Anything animating (including the frame a toast or effect ends) needs a redraw.
    bool animating = A.toast.t > 0 || A.nfx > 0 || A.view == VIEW_BOOT;
    if (A.toast.t > 0)
        A.toast.t--;
    int j = 0;
    for (int i = 0; i < A.nfx; i++)
        if (--A.fx[i].t > 0)
            A.fx[j++] = A.fx[i];
    A.nfx = j;
    step(in);
    bool input = in->down || in->rep || in->touch != TOUCH_NONE;
    // Every 8th frame covers the campfire icon and the keyboard cursor blink.
    return animating || input || A.toast.t > 0 || A.nfx > 0 || A.frame == 1 || (A.frame & 7) == 0;
}

void app_render(Surface *top, Surface *bot)
{
    if (A.lid)
        return;
    top->pal = bot->pal = A.pal;
    Ctx *cx = &renderCtx;
    ctx_now(cx);
    if (cx->top)
        cx->top(top, cx);
    d_rect(bot, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    if (cx->bot)
        cx->bot(bot, cx);
    uint16_t fid = cx->n ? ctx_focus(cx) : 0;
    for (int i = 0; i < cx->n; i++) {
        const Btn *b = &cx->btns[i];
        bool press = A.press.down && A.press.id == b->id && A.press.inside && A.press.ctx == cx->key;
        if (b->draw)
            b->draw(bot, b, b->id == fid && b->nav, press);
    }
    if (cx->nfooter)
        footer_draw(bot, cx->footer, cx->nfooter, cx->edit);
}

void app_lid(bool closed)
{
    session_lid(closed);
}
