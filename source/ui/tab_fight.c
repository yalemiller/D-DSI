// FIGHT tab: HP, keypad with damage / heal / temp HP, inspiration, death
// saves, and the rest buttons. The rules live in core/actions.c.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/actions.h"
#include "core/text.h"
#include "ui.h"

static int amount(void)
{
    return atoi(A.amt);
}

static void fx_simple(int kind, int t, uint8_t col)
{
    Fx f;
    memset(&f, 0, sizeof f);
    f.kind = (uint8_t)kind;
    f.t = t;
    f.col = col;
    fx_add(&f);
}

static void fx_count(int from, int to)
{
    Fx f;
    memset(&f, 0, sizeof f);
    f.kind = FX_COUNT;
    f.t = 18;
    f.from = from;
    f.to = to;
    fx_add(&f);
}

static void fx_float(const char *text, uint8_t col)
{
    Fx f;
    memset(&f, 0, sizeof f);
    f.kind = FX_FLOAT;
    f.t = 40;
    f.col = col;
    snprintf(f.text, sizeof f.text, "%s", text);
    fx_add(&f);
}

void fx_sparkle(int t, int x, int y, int w, int h, uint8_t col)
{
    Fx f;
    memset(&f, 0, sizeof f);
    f.kind = FX_SPARKLE;
    f.t = t;
    f.col = col;
    f.rect[0] = (int16_t)x;
    f.rect[1] = (int16_t)y;
    f.rect[2] = (int16_t)w;
    f.rect[3] = (int16_t)h;
    fx_add(&f);
}

static void first_word(const char *name, char *out, size_t cap)
{
    size_t i = 0;
    while (name[i] && name[i] != ' ' && i + 1 < cap) {
        out[i] = name[i];
        i++;
    }
    out[i] = 0;
}

static void apply_damage(const Btn *b)
{
    (void)b;
    Character *k = ch();
    int n = amount();
    if (!n) {
        toast("ENTER AN AMOUNT FIRST", C_GOLD);
        return;
    }
    HpChange c = act_damage(k, n);
    fx_simple(FX_SHAKE, 12, C_NONE);
    fx_simple(FX_FLASH, 14, C_RED);
    fx_count(c.before, c.after);
    char t[12];
    snprintf(t, sizeof t, "-%d", n);
    fx_float(t, C_RED);
    if (k->hp == 0) {
        char w[NAME_LEN + 1];
        first_word(k->name, w, sizeof w);
        toastf(C_RED, "%s IS DOWN!", w);
    } else if (c.absorbed) {
        toastf(C_RED, "OUCH! -%d (%d TEMP)  HP %d/%d", n, c.absorbed, k->hp, k->hpMax);
    } else {
        toastf(C_RED, "OUCH! -%d  HP %d/%d", n, k->hp, k->hpMax);
    }
    A.amt[0] = 0;
    A.dirty = true;
}

static void apply_heal(const Btn *b)
{
    (void)b;
    Character *k = ch();
    int n = amount();
    if (!n) {
        toast("ENTER AN AMOUNT FIRST", C_GOLD);
        return;
    }
    HpChange c = act_heal(k, n);
    fx_simple(FX_FLASH, 14, C_GREEN);
    fx_count(c.before, c.after);
    char t[12];
    snprintf(t, sizeof t, "+%d", c.after - c.before);
    fx_float(t, C_GREEN);
    fx_sparkle(36, 4, 54, 114, 38, C_GREEN);
    if (c.before == 0)
        toastf(C_GREEN, "BACK ON YOUR FEET!  HP %d", k->hp);
    else if (k->hp == (int)k->hpMax)
        toastf(C_GREEN, "+%d HP  GOOD AS NEW!", c.after - c.before);
    else
        toastf(C_GREEN, "+%d HP  NOW %d/%d", n, k->hp, k->hpMax);
    A.amt[0] = 0;
    A.dirty = true;
}

static void apply_temp(const Btn *b)
{
    (void)b;
    Character *k = ch();
    int n = amount(), old;
    if (!n) {
        toast("ENTER AN AMOUNT FIRST", C_GOLD);
        return;
    }
    if (act_temp_hp(k, n, &old)) {
        fx_simple(FX_FLASH, 14, C_BLUE);
        char t[12];
        snprintf(t, sizeof t, "+%d", k->temp - old);
        fx_float(t, C_BLUE);
        toastf(C_BLUE, "A WARD OF %d TEMP HP SHIMMERS", k->temp);
    } else {
        toastf(C_BLUE, "KEPT %d: TEMP HP DOESN'T STACK", old);
    }
    A.amt[0] = 0;
    A.dirty = true;
}

static void insp_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    d_icon(s, IC_STAR, b->x + 4, b->y + 5, k->insp ? C_GOLD : C_LINE, C_NONE, C_NONE, 1);
    d_text(s, k->insp ? "INSPIRED" : "UNINSPIRED", b->x + 19, b->y + 8, k->insp ? C_GOLD : C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void insp_act(const Btn *b)
{
    (void)b;
    Character *k = ch();
    k->insp ^= 1;
    A.dirty = true;
    if (k->insp) {
        toast("INSPIRED!", C_GOLD);
        fx_sparkle(40, 2, 178, 90, 14, C_GOLD);
    } else {
        toast("INSPIRATION SPENT", C_DIM);
    }
}

static void key_act(const Btn *b)
{
    const char *l = b->label;
    size_t n = strlen(A.amt);
    if (!strcmp(l, "CLR")) {
        A.amt[0] = 0;
    } else if (!strcmp(l, "DEL")) {
        if (n)
            A.amt[n - 1] = 0;
    } else if (n < 3) {
        if (!strcmp(A.amt, "0"))
            n = 0;
        A.amt[n] = l[0];
        A.amt[n + 1] = 0;
    }
}

static void ds_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    bool fails = BKIND(b->id) == B_DSF;
    int v = fails ? k->df : k->ds;
    uint8_t col = fails ? C_RED : C_GREEN;
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    d_text(s, b->label, b->x + 5, b->y + 6, C_DIM, 1);
    for (int p = 0; p < 3; p++) {
        int x = b->x + 44 + p * 9;
        if (p < v && fails)
            d_icon(s, IC_MSKULL, x, b->y + 6, col, C_NONE, C_NONE, 1);
        else
            d_text(s, p < v ? G_DOT : G_RING, x, b->y + 6, p < v ? col : C_LINE, 1);
    }
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void ds_act(const Btn *b)
{
    Character *k = ch();
    uint8_t *v = BKIND(b->id) == B_DSF ? &k->df : &k->ds;
    *v = (uint8_t)((*v + 1) % 4);
    A.dirty = true;
}

static int campfire(void)
{
    return (A.frame >> 3) & 1 ? IC_FIRE2 : IC_FIRE;
}

static void srest_act(const Btn *b)
{
    (void)b;
    open_short_rest();
}

static void lrest_act(const Btn *b)
{
    (void)b;
    ask_long_rest();
}

static void fight_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Character *k = ch();
    char buf[32];
    d_rect(s, 3, 19, 168, 22, C_PANEL);
    d_icon(s, k->hp == 0 ? IC_SKULL : IC_HEART, 7, 25, C_RED, C_PANEL, C_NONE, 1);
    snprintf(buf, sizeof buf, "%d/%d", k->hp, k->hpMax);
    d_text(s, buf, 24, 23, k->hp == 0 ? C_RED : C_TEXT, 2);
    if (k->temp) {
        snprintf(buf, sizeof buf, "+%d", k->temp);
        d_text_r(s, buf, 166, 27, C_BLUE, 1);
        d_icon(s, IC_MSHIELD, 166 - d_tw(buf, 1) - 10, 27, C_BLUE, C_NONE, C_NONE, 1);
    }
    d_rect(s, 3, 44, 91, 20, C_PANEL);
    d_text(s, "AMT", 8, 51, C_DIM, 1);
    d_text_r(s, A.amt[0] ? A.amt : "0", 89, 47, A.amt[0] ? C_TEXT : C_LINE, 2);
    d_icon(s, IC_MSKULL, 100, 118, k->hp == 0 ? C_RED : C_DIM, C_NONE, C_NONE, 1);
    d_text(s, "DEATH SAVES", 110, 118, k->hp == 0 ? C_RED : C_DIM, 1);
    snprintf(buf, sizeof buf, "HIT DICE %d/%d D%d", k->level - k->hdUsed, k->level, k->hitDie);
    d_text(s, buf, 4, 167, C_DIM, 1);
}

void tab_fight(Ctx *cx, Character *k)
{
    (void)k;
    int n = amount();
    Btn *b = ctx_add(cx, BID(B_INSP, 0), 174, 19, 79, 22);
    b->draw = insp_draw;
    b->act = insp_act;
    static const char *const KEYS[12] = { "7", "8", "9", "4", "5", "6", "1", "2", "3", "CLR", "0", "DEL" };
    for (int j = 0; j < 12; j++) {
        b = add_plain(cx, BID(B_KEY, j), 3 + (j % 3) * 31, 67 + (j / 3) * 24, 29, 22, KEYS[j], C_PANEL2, C_TEXT,
                      KEYS[j][1] ? 1 : 2);
        b->act = key_act;
    }
    static const struct {
        int kind;
        const char *label;
        int y, h;
        uint8_t col;
        BtnFn fn;
        int icon;
    } BIG[3] = {
        { B_DMG, "DAMAGE", 44, 24, C_RED, apply_damage, IC_SWORD },
        { B_HEAL, "HEAL", 70, 24, C_GREEN, apply_heal, IC_HEART },
        { B_TEMP, "SET TEMP HP", 96, 18, C_BLUE, apply_temp, IC_SHIELD },
    };
    for (int j = 0; j < 3; j++) {
        b = ctx_add(cx, BID(BIG[j].kind, 0), 100, BIG[j].y, 153, BIG[j].h);
        b->draw = btn_icon;
        b->act = BIG[j].fn;
        b->label = BIG[j].label;
        b->icon = (int8_t)BIG[j].icon;
        b->sc = BIG[j].h >= 24 ? 2 : 1;
        b->ic1 = IC_USE_BG;
        if (n) {
            b->bg = BIG[j].col;
            b->fg = C_INK;
        } else {
            b->bg = C_PANEL2;
            b->fg = C_DIM;
            b->ic0 = BIG[j].col;
        }
    }
    b = ctx_add(cx, BID(B_DSS, 0), 100, 127, 75, 18);
    b->draw = ds_draw;
    b->act = ds_act;
    b->label = "SAVES";
    b = ctx_add(cx, BID(B_DSF, 0), 178, 127, 75, 18);
    b->draw = ds_draw;
    b->act = ds_act;
    b->label = "FAILS";
    b = ctx_add(cx, BID(B_SREST, 0), 100, 150, 75, 24);
    b->draw = btn_icon;
    b->iconFn = campfire;
    b->label = "SHORT";
    b->label2 = "REST";
    b->ic0 = C_GOLD;
    b->ic1 = C_TEXT;
    b->ic2 = C_DIM;
    b->act = srest_act;
    b = ctx_add(cx, BID(B_LREST, 0), 178, 150, 75, 24);
    b->draw = btn_icon;
    b->icon = IC_MOON;
    b->label = "LONG";
    b->label2 = "REST";
    b->ic0 = C_TEXT;
    b->ic1 = C_GOLD;
    b->act = lrest_act;
    cx->def = BID(B_KEY, 0);
    ctx_footer(cx, "A", "PRESS", "B", "CLEAR", "L/R", "TABS", "ST", "MENU", (const char *)NULL);
    cx->bot = fight_bot;
}
