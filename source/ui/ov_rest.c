// Short rest (spend hit dice, enter the HP rolled or use the average) and
// long rest (asks first). Both save, then play their scene on the top screen.
#include <stdio.h>
#include <string.h>

#include "core/actions.h"
#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

void ask_long_rest(void)
{
    Notice *n = notice_push();
    snprintf(n->title, sizeof n->title, "TAKE A LONG REST?");
    snprintf(n->sub, sizeof n->sub, "RESTORES HP, SLOTS, HIT DICE");
    snprintf(n->lines[0], sizeof n->lines[0], "RESTORES ALL HP, SPELL SLOTS, HIT DICE AND FEATURE USES.");
    snprintf(n->lines[1], sizeof n->lines[1], "CLEARS TEMP HP AND DEATH SAVES.");
    n->nlines = 2;
    n->btns[0] = (NoticeBtn){ "CANCEL", NB_PLAIN, NA_CLOSE };
    n->btns[1] = (NoticeBtn){ "REST", NB_PRIMARY, NA_LONG_REST };
    n->nbtns = 2;
    n->def = 0;
    n->cancel = 0;
}

void do_long_rest(void)
{
    act_long_rest(ch(), &A.flib);
    A.dirty = true;
    ov_clear();
    Fx f;
    memset(&f, 0, sizeof f);
    f.kind = FX_NIGHT;
    f.t = 90;
    fx_add(&f);
    session_rest_save("A NIGHT PASSES. FULLY RESTORED");
}

void open_short_rest(void)
{
    ov_push(OV_SREST);
}

static void apply(void)
{
    Overlay *o = ov_top();
    int n = o->srest.n;
    HpChange c = act_short_rest(ch(), &A.flib, n, o->srest.hp);
    A.dirty = true;
    ov_clear();
    Fx f;
    memset(&f, 0, sizeof f);
    f.kind = FX_FIRE;
    f.t = 80;
    if (n)
        snprintf(f.sub, sizeof f.sub, "HP %d " G_ARROW " %d   %d HIT %s SPENT", c.before, c.after, n,
                 n > 1 ? "DICE" : "DIE");
    else
        snprintf(f.sub, sizeof f.sub, "HP %d " G_ARROW " %d", c.before, c.after);
    fx_add(&f);
    char msg[64];
    snprintf(msg, sizeof msg, "RESTED BY THE FIRE: HP %d " G_ARROW " %d", c.before, c.after);
    session_rest_save(msg);
}

static void btn_act(const Btn *b)
{
    Overlay *o = ov_top();
    const Character *k = ch();
    int left = k->level - k->hdUsed;
    switch (BKIND(b->id)) {
    case B_SRN:
        if (b->b < 0) {
            o->srest.n = o->srest.n > 0 ? o->srest.n - 1 : 0;
        } else if (o->srest.n < left) {
            o->srest.n++;
        } else {
            toast("NO HIT DICE LEFT", C_GOLD);
        }
        break;
    case B_SRH: {
        int v = o->srest.hp + b->b;
        o->srest.hp = v < 0 ? 0 : v > 999 ? 999 : v;
        break;
    }
    case B_SRAVG:
        if (o->srest.n)
            o->srest.hp = act_short_rest_average(k, o->srest.n);
        break;
    case B_SRCANCEL:
        ov_pop();
        break;
    default:
        apply();
        break;
    }
}

static void srest_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    const Character *k = ch();
    int left = k->level - k->hdUsed, con = rules_mod(k->ab[AB_CON]);
    char buf[64];
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, "SHORT REST", 6, 5, C_GOLD, 1);
    snprintf(buf, sizeof buf, "HIT DICE %d/%d  D%d", left, k->level, k->hitDie);
    d_text_r(s, buf, 250, 5, C_DIM, 1);
    snprintf(buf, sizeof buf, "EACH DIE: D%d %s (CON). ROLL, THEN ENTER.", k->hitDie, sg(con));
    d_text(s, buf, 4, 22, C_DIM, 1);
    d_text(s, "DICE TO SPEND", 6, 44, C_TEXT, 1);
    d_rect(s, 158, 36, 56, 22, C_PANEL);
    snprintf(buf, sizeof buf, "%d", o->srest.n);
    d_text_c(s, buf, 158, 56, 40, C_GOLD, 2);
    d_text(s, "HP REGAINED", 6, 70, C_TEXT, 1);
    d_rect(s, 93, 82, 70, 24, C_PANEL);
    snprintf(buf, sizeof buf, "%d", o->srest.hp);
    d_text_c(s, buf, 93, 70, 87, C_GOLD, 2);
}

static void srest_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    const Character *k = ch();
    int after = k->hp + o->srest.hp < (int)k->hpMax ? k->hp + o->srest.hp : (int)k->hpMax;
    char buf[64];
    snprintf(buf, sizeof buf, "SHORT REST: %d HIT DICE   HP %d " G_ARROW " %d", o->srest.n, k->hp, after);
    top_sheet(s, buf, NULL);
}

static bool srest_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    if (in->down & KBIT(K_B)) {
        ov_pop();
        return true;
    }
    if (in->down & KBIT(K_START)) {
        apply();
        return true;
    }
    return false;
}

static Btn *add(Ctx *cx, int kind, int i, int x, int y, int w, int h, const char *label, int delta)
{
    Btn *b = add_plain(cx, BID(kind, i), x, y, w, h, label, C_PANEL2, C_TEXT, 1);
    b->act = btn_act;
    b->b = delta;
    return b;
}

void ov_srest_build(Ctx *cx, Overlay *o)
{
    add(cx, B_SRN, 0, 120, 36, 36, 22, "-", -1)->sc = 2;
    add(cx, B_SRN, 1, 216, 36, 36, 22, "+", 1)->sc = 2;
    add(cx, B_SRH, 0, 3, 82, 46, 24, "-10", -10);
    add(cx, B_SRH, 1, 51, 82, 40, 24, "-1", -1);
    add(cx, B_SRH, 2, 165, 82, 40, 24, "+1", 1);
    add(cx, B_SRH, 3, 207, 82, 46, 24, "+10", 10);
    int avg = act_short_rest_average(ch(), o->srest.n);
    Btn *b = add(cx, B_SRAVG, 0, 3, 112, 250, 20,
                 o->srest.n ? ctx_str(cx, "USE AVERAGE: %d HP", avg) : "USE AVERAGE (CHOOSE DICE FIRST)", 0);
    if (!o->srest.n)
        b->fg = C_DIM;
    add(cx, B_SRCANCEL, 0, 12, 146, 110, 26, "CANCEL", 0);
    b = add(cx, B_SROK, 0, 134, 146, 110, 26, "REST", 0);
    b->bg = C_GOLD;
    b->fg = C_INK;
    cx->def = BID(B_SRN, 1);
    ctx_footer(cx, "A", "PRESS", "ST", "REST", "B", "CANCEL", (const char *)NULL);
    cx->bot = srest_bot;
    cx->top = srest_top;
    cx->onKeys = srest_keys;
}
