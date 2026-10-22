// ARMS tab: up to 4 weapon cards with the two numbers you need (to-hit and
// damage); A opens the details, X expands the card onto the top screen.
#include <stdio.h>

#include "core/actions.h"
#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

static void weapon_card_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    const Weapon *w = &k->weapons[b->a];
    int x = b->x, y = b->y, ai = rules_weapon_ability(k, w);
    char d[24];
    d_rect(s, x, y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    // row 1: identity
    d_icon(s, rules_weapon_is_ranged(w) ? IC_ARROW : IC_SWORD, x + 5, y + 4, C_DIM, C_NONE, C_NONE, 1);
    d_text(s, w->name, x + 20, y + 5, C_TEXT, 1);
    if (w->mastery) {
        int mx = x + 20 + d_tw(w->name, 1) + 8;
        d_text(s, G_DIAMOND, mx, y + 5, C_BLUE, 1);
        d_text(s, MASTERY_NAMES[w->mastery], mx + 8, y + 5, C_BLUE, 1);
    }
    if (w->range[0])
        d_text_r(s, w->range, x + b->w - 6, y + 5, C_DIM, 1);
    // row 2: to-hit and damage
    const char *a = sg(rules_weapon_to_hit(k, w));
    d_text(s, a, x + 6, y + 17, ai < 0 ? C_RED : C_GOLD, 2);
    d_text(s, "HIT", x + 6 + d_tw(a, 2) + 4, y + 24, C_DIM, 1);
    rules_weapon_damage(w, d, sizeof d);
    int dx = x + 76;
    d_text(s, d, dx, y + 17, C_TEXT, 2);
    d_text(s, DTYPE_SHORT[w->type], dx + d_tw(d, 2) + 5, y + 24, C_DIM, 1);
    if (!w->prof)
        d_text_r(s, "NOT PROF", x + b->w - 6, y + 24, C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void weapon_card_act(const Btn *b)
{
    open_weapon_detail(b->a);
}

static void add_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_BG);
    d_box(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_LINE, 1);
    d_text_c(s, "+ ADD WEAPON", b->x, b->w, b->y + floordiv(b->h - 7, 2), press ? C_INK : C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

void add_weapon_flow(void)
{
    if (ch()->nWeapons >= MAX_WEAPONS) {
        toastf(C_GOLD, "%d WEAPONS MAX. EDIT OR DELETE ONE.", MAX_WEAPONS);
        return;
    }
    open_kbd("WEAPON NAME", "NAME", "", WEAPON_NAME_LEN, false, KT_ADDWEAPON, 0);
}

static void add_act(const Btn *b)
{
    (void)b;
    add_weapon_flow();
}

static void arms_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    if (ch()->nWeapons)
        return;
    d_text_c(s, "NO WEAPONS YET", 0, SCREEN_W, 80, C_TEXT, 1);
    d_text_c(s, "ADD UP TO 4. TO-HIT AND DAMAGE ARE", 0, SCREEN_W, 96, C_DIM, 1);
    d_text_c(s, "WORKED OUT FROM YOUR SCORES.", 0, SCREEN_W, 106, C_DIM, 1);
}

void tab_arms(Ctx *cx, Character *k)
{
    for (int i = 0; i < k->nWeapons; i++) {
        Btn *b = ctx_add(cx, BID(B_WEAPON, i), 3, 18 + i * 38, 250, 36);
        b->draw = weapon_card_draw;
        b->act = weapon_card_act;
        b->a = i;
    }
    if (k->nWeapons < MAX_WEAPONS) {
        int i = k->nWeapons;
        Btn *b = ctx_add(cx, BID(B_WADD, 0), 3, 18 + i * 38, 250, i ? 22 : 36);
        b->draw = add_draw;
        b->act = add_act;
    }
    cx->def = k->nWeapons ? BID(B_WEAPON, 0) : BID(B_WADD, 0);
    ctx_footer(cx, "A", k->nWeapons ? "DETAILS" : "ADD", "X", "EXPAND", "L/R", "TABS", "ST", "MENU",
               (const char *)NULL);
    cx->bot = arms_bot;
}
