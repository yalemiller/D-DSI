// Boot splash and the character select screen.
#include <stdio.h>
#include <string.h>

#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

#define APP_VERSION "V0.1"

// ---------- boot ----------

static void boot_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_icon(s, IC_SWORD, 128 - 11, 26, C_TEXT, C_NONE, C_NONE, 2);
    sparkle(s, A.t, 5, 96, 22, 64, 30, C_GOLD);
    d_text_c(s, "POCKET SHEET", 0, SCREEN_W, 64, C_GOLD, 3);
    d_rect(s, 60, 92, 56, 1, C_LINE);
    d_rect(s, 140, 92, 56, 1, C_LINE);
    d_text_c(s, G_DIAMOND, 0, SCREEN_W, 89, C_GOLD, 1);
    d_text_c(s, "CHARACTER REFERENCE", 0, SCREEN_W, 100, C_DIM, 1);
    d_text_c(s, APP_VERSION, 0, SCREEN_W, 114, C_DIM, 1);
    d_text_c(s, "GAME TERMS FROM SRD 5.1, CC-BY-4.0", 0, SCREEN_W, 176, C_DIM, 1);
}

static void boot_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    static const char *const DOTS[3] = { "MOUNTING SD CARD.", "MOUNTING SD CARD..", "MOUNTING SD CARD..." };
    int x = floordiv(SCREEN_W - d_tw(DOTS[2], 1), 2);
    d_text(s, DOTS[(A.t / 12) % 3], x, 92, C_TEXT, 1);
}

void view_boot(Ctx *cx)
{
    cx->key = 1;
    cx->top = boot_top;
    cx->bot = boot_bot;
}

// ---------- character select ----------

void top_select(Surface *s)
{
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_text_c(s, "POCKET SHEET", 0, SCREEN_W, 16, C_GOLD, 2);
    d_text_c(s, "CHARACTER REFERENCE", 0, SCREEN_W, 36, C_DIM, 1);
    d_rect(s, 8, 52, 240, 98, C_PANEL);
    char buf[64];
    if (A.roster.used[A.sel]) {
        const Character *k = &A.roster.slot[A.sel];
        portrait_draw(s, A.sel, 14, 58);
        d_text(s, k->name, 70, 60, C_TEXT, d_tw(k->name, 2) <= 172 ? 2 : 1);
        snprintf(buf, sizeof buf, "%s%s%s", k->cls, k->sub[0] ? " " : "", k->sub);
        d_text(s, buf, 70, 80, C_TEXT, 1);
        snprintf(buf, sizeof buf, "LEVEL %d   HP %d/%d", k->level, k->hp, k->hpMax);
        d_text(s, buf, 70, 92, C_DIM, 1);
        snprintf(buf, sizeof buf, "AC %d   PROF %s", k->ac, sg(rules_pb(k)));
        d_text(s, buf, 70, 102, C_DIM, 1);
        snprintf(buf, sizeof buf, "STR %d   DEX %d   CON %d", k->ab[0], k->ab[1], k->ab[2]);
        d_text(s, buf, 14, 120, C_DIM, 1);
        snprintf(buf, sizeof buf, "INT %d   WIS %d   CHA %d", k->ab[3], k->ab[4], k->ab[5]);
        d_text(s, buf, 14, 132, C_DIM, 1);
    } else {
        d_text_c(s, "EMPTY SLOT", 8, 240, 86, C_DIM, 2);
        d_text_c(s, "PRESS A TO CREATE A CHARACTER", 8, 240, 108, C_DIM, 1);
    }
    if (!A.temp) {
        d_text(s, "SAVING TO", 8, 156, C_DIM, 1);
        d_text(s, "FAT:/DATA/POCKETSHEET/", 8, 166, C_TEXT, 1);
    } else {
        d_text(s, "TEMPORARY SESSION", 8, 156, C_GOLD, 1);
        d_text(s, "NOTHING WILL BE SAVED", 8, 166, C_TEXT, 1);
    }
    snprintf(buf, sizeof buf, "SLOT %d/3", A.sel + 1);
    d_text_r(s, buf, 248, 166, C_DIM, 1);
    if (A.toast.t > 0)
        strip(s, A.toast.text, A.toast.col);
}

static void select_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    top_select(s);
}

static void select_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    d_text(s, "CHOOSE A CHARACTER", 8, 7, C_DIM, 1);
}

static void slot_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    int i = b->a;
    char buf[48];
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    d_rect(s, b->x, b->y, 20, b->h, C_PANEL2);
    snprintf(buf, sizeof buf, "%d", i + 1);
    d_text_c(s, buf, b->x, 20, b->y + 16, C_DIM, 1);
    if (A.roster.used[i]) {
        const Character *k = &A.roster.slot[i];
        d_text(s, k->name, b->x + 28, b->y + 9, C_TEXT, 1);
        snprintf(buf, sizeof buf, "%s  LV %d", k->cls, k->level);
        d_text(s, buf, b->x + 28, b->y + 24, C_DIM, 1);
        int rx = b->x + b->w - 8;
        snprintf(buf, sizeof buf, "%d/%d", k->hp, k->hpMax);
        d_text_r(s, buf, rx, b->y + 9, C_TEXT, 1);
        d_icon(s, IC_MHEART, rx - d_tw(buf, 1) - 10, b->y + 9, C_RED, C_NONE, C_NONE, 1);
        snprintf(buf, sizeof buf, "%d", k->ac);
        d_text_r(s, buf, rx, b->y + 24, C_DIM, 1);
        d_icon(s, IC_MSHIELD, rx - d_tw(buf, 1) - 10, b->y + 24, C_DIM, C_NONE, C_NONE, 1);
    } else {
        d_text(s, "+ NEW CHARACTER", b->x + 28, b->y + 16, C_DIM, 1);
    }
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void slot_act(const Btn *b)
{
    if (A.roster.used[b->a])
        session_open_slot(b->a);
    else
        open_kbd("NEW CHARACTER NAME", "NAME", "", NAME_LEN, false, KT_NEWCHAR, b->a);
}

static void del_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    bool dis = b->disabled;
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_PANEL);
    d_text(s, G_CROSS, b->x + 6, b->y + 6, dis ? C_LINE : press ? C_INK : C_RED, 1);
    d_text(s, "DELETE...", b->x + 16, b->y + 6, dis ? C_LINE : press ? C_INK : C_TEXT, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void del_act(const Btn *b)
{
    (void)b;
    session_ask_delete(A.sel);
}

static bool select_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    if (in->down & KBIT(K_X)) {
        if (A.roster.used[A.sel])
            session_ask_delete(A.sel);
        return true;
    }
    return false;
}

void view_select(Ctx *cx)
{
    cx->key = 2;
    cx->focus = &A.focusSelect;
    for (int i = 0; i < NUM_SLOTS; i++) {
        Btn *b = ctx_add(cx, BID(B_SLOT, i), 8, 20 + i * 46, 240, 40);
        b->draw = slot_draw;
        b->act = slot_act;
        b->a = i;
    }
    if (BKIND(A.focusSelect) == B_SLOT)
        A.sel = BINDEX(A.focusSelect);
    Btn *del = ctx_add(cx, BID(B_DEL, 0), 8, 158, 100, 18);
    del->disabled = !A.roster.used[A.sel];
    del->draw = del_draw;
    del->act = del_act;
    cx->def = BID(B_SLOT, A.sel);
    ctx_footer(cx, "A", A.roster.used[A.sel] ? "OPEN" : "CREATE", "X", "DELETE", G_TRI_UP G_TRI_DOWN, "CHOOSE",
               (const char *)NULL);
    cx->top = select_top;
    cx->bot = select_bot;
    cx->onKeys = select_keys;
}
