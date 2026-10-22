// The top screen: the always-on sheet (identity, HP, AC, abilities and saves,
// secondary numbers, status), the hover summary card, full-page cards for
// spells / features / weapons, and the frame-counted effects.
#include <stdio.h>
#include <string.h>

#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

#define MAX_LINES 256
static TextLine lines[MAX_LINES]; // shared by the text layouts on this screen

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi : v;
}

uint8_t feat_cat_color(int cat)
{
    static const uint8_t COLS[FCAT_COUNT] = { C_GOLD, C_GOLD, C_GREEN, C_DIM, C_BLUE, C_RED, C_LINE };
    return cat >= 0 && cat < FCAT_COUNT ? COLS[cat] : C_RED;
}

void sparkle(Surface *s, int t, int seed, int x, int y, int w, int h, uint8_t col)
{
    for (int i = 0; i < 7; i++) {
        if (((t + i * 3) >> 2) % 2)
            continue;
        int px = x + (seed * 37 + i * 53) % w, py = y + (seed * 17 + i * 29) % h;
        d_rect(s, px - 1, py, 3, 1, col);
        d_rect(s, px, py - 1, 1, 3, col);
        d_rect(s, px, py, 1, 1, C_TEXT);
    }
}

void top_fx(Surface *s)
{
    for (int i = 0; i < A.nfx; i++) {
        const Fx *f = &A.fx[i];
        if (f->kind == FX_SPARKLE)
            sparkle(s, f->t, f->seed, f->rect[0], f->rect[1], f->rect[2], f->rect[3], f->col);
    }
    const Fx *ov = fx_find(FX_NIGHT);
    if (!ov)
        ov = fx_find(FX_FIRE);
    if (!ov)
        return;
    // globalAlpha = min(1, t / 18, (T - t + 1) / 8)
    int a = 256, a1 = ov->t * 256 / 18, a2 = (ov->T - ov->t + 1) * 256 / 8;
    a = a1 < a ? a1 : a;
    a = a2 < a ? a2 : a;
    s->alpha = a;
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    if (ov->kind == FX_NIGHT) {
        for (int i = 0; i < 14; i++) {
            int px = (ov->seed * 31 + i * 71) % 250 + 3, py = (ov->seed * 13 + i * 37) % 170 + 4;
            if (((ov->t + i * 5) >> 3) % 3) {
                d_rect(s, px, py, 1, 1, C_TEXT);
            } else {
                d_rect(s, px - 1, py, 3, 1, C_GOLD);
                d_rect(s, px, py - 1, 1, 3, C_GOLD);
            }
        }
        d_icon(s, IC_MOON, 128 - 17, 30, C_TEXT, C_GOLD, C_NONE, 3);
        d_text_c(s, "A NIGHT PASSES...", 0, SCREEN_W, 112, C_GOLD, 2);
        d_text_c(s, "HP, SPELL SLOTS AND HIT DICE RESTORED", 0, SCREEN_W, 136, C_DIM, 1);
    } else {
        d_icon(s, (ov->t >> 3) & 1 ? IC_FIRE2 : IC_FIRE, 128 - 17, 28, C_GOLD, C_TEXT, C_DIM, 3);
        sparkle(s, ov->t, ov->seed, 90, 14, 76, 30, C_GOLD);
        d_text_c(s, "YOU REST BY THE FIRE", 0, SCREEN_W, 112, C_GOLD, 2);
        if (ov->sub[0])
            d_text_c(s, ov->sub, 0, SCREEN_W, 136, C_DIM, 1);
    }
    s->alpha = 256;
}

// tx(c, label) then the value 5 px after it; returns where the next pair starts.
static int pair(Surface *s, int x, int y, const char *l, const char *v, uint8_t col)
{
    d_text(s, l, x, y, C_DIM, 1);
    d_text(s, v, x + d_tw(l, 1) + 5, y, col, 1);
    return x + d_tw(l, 1) + 5 + d_tw(v, 1) + 14;
}

static void draw_textn_clip(Surface *s, const char *p, int len, int max, int x, int y, uint8_t col)
{
    d_textn(s, p, len < max ? len : max, x, y, col, 1);
}

void top_summary(Surface *s, const Summary *sum)
{
    uint8_t col = sum->col;
    d_rect(s, 0, 97, SCREEN_W, 79, C_PANEL);
    d_rect(s, 0, 97, 2, 79, col);
    if (sum->icon >= 0)
        d_icon(s, sum->icon, 6, 101 + floordiv(7 - icon_h(sum->icon), 2), col, C_NONE, C_NONE, 1);
    int nx = sum->icon >= 0 ? 6 + icon_w(sum->icon) + 4 : 6;
    d_text(s, sum->title, nx, 101, C_TEXT, 1);
    if (sum->tag[0])
        d_text_r(s, sum->tag, 251, 101, sum->tagCol, 1);
    draw_textn_clip(s, sum->meta, (int)strlen(sum->meta), 41, 6, 112, C_DIM);
    d_rect(s, 6, 122, 244, 1, C_LINE);
    int n = text_layout(sum->desc ? sum->desc : "", 41, true, lines, MAX_LINES);
    int vis = sum->hint[0] ? 4 : 5;
    bool more = n > vis;
    for (int i = 0; i < vis && i < n && i < MAX_LINES; i++) {
        char buf[48];
        if (i == vis - 1 && more) {
            int len = lines[i].len > 37 ? 37 : lines[i].len;
            snprintf(buf, sizeof buf, "%.*s...", len, lines[i].p);
            d_text(s, buf, 6, 126 + i * 10, C_TEXT, 1);
        } else {
            d_textn(s, lines[i].p, lines[i].len, 6, 126 + i * 10, C_TEXT, 1);
        }
    }
    if (sum->hint[0]) {
        const char *mt = 6 + d_tw("MORE ON THE FULL PAGE", 1) + 8 <= 251 - d_tw(sum->hint, 1) ? "MORE ON THE FULL PAGE"
                                                                                           : "MORE...";
        if (more)
            d_text(s, mt, 6, 167, C_DIM, 1);
        d_text_r(s, sum->hint, 251, 167, C_GOLD, 1);
    }
}

void top_sheet(Surface *s, const char *banner, const Summary *sum)
{
    const Character *k = ch();
    int dm = rules_mod(k->ab[AB_DEX]);
    bool down = k->hp == 0;
    char buf[96];
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);

    // 1. identity
    portrait_draw(s, A.cur, 1, 1);
    d_text(s, k->name, 55, 4, C_TEXT, d_tw(k->name, 2) <= 199 ? 2 : 1);
    char lv[16];
    snprintf(lv, sizeof lv, "LEVEL %d", k->level);
    int room = floordiv(252 - d_tw(lv, 1) - 8 - 55, 6);
    snprintf(buf, sizeof buf, "%s%s%s", k->cls, k->sub[0] ? ", " : "", k->sub);
    if ((int)strlen(buf) > room)
        buf[room > 0 ? room : 0] = 0;
    d_text(s, buf, 55, 24, C_TEXT, 1);
    d_text(s, lv, 55 + d_tw(buf, 1) + 8, 24, C_GOLD, 1);
    snprintf(buf, sizeof buf, "%s%s%s", k->species, k->species[0] && k->background[0] ? ", " : "", k->background);
    if (buf[0]) {
        buf[33] = 0;
        d_text(s, buf, 55, 36, C_DIM, 1);
    }

    // 2. survival: HP dominates, AC next, INIT and SPEED support
    const Fx *sh = fx_find(FX_SHAKE), *cnt = fx_find(FX_COUNT), *fl = fx_find(FX_FLASH);
    int hx = sh ? (sh->t % 4 < 2 ? 2 : -2) : 0;
    int shown = k->hp;
    if (cnt) { // Math.round(to + (from - to) * t / T)
        int num = cnt->to * cnt->T + (cnt->from - cnt->to) * cnt->t;
        shown = floordiv(2 * num + cnt->T, 2 * cnt->T);
    }
    d_rect(s, 1 + hx, 52, 120, 42, C_PANEL);
    if (fl && (fl->t >> 1) % 2 == 0)
        d_box(s, 1 + hx, 52, 120, 42, fl->col, 2);
    d_icon(s, down ? IC_MSKULL : IC_MHEART, 5 + hx, 56, C_RED, C_NONE, C_NONE, 1);
    d_text(s, down ? "DOWN" : "HIT POINTS", 15 + hx, 56, down ? C_RED : C_DIM, 1);
    if (k->temp) {
        snprintf(buf, sizeof buf, "+%d", k->temp);
        d_text_r(s, buf, 117 + hx, 56, C_BLUE, 1);
        d_icon(s, IC_MSHIELD, 117 + hx - d_tw(buf, 1) - 10, 56, C_BLUE, C_NONE, C_NONE, 1);
    }
    snprintf(buf, sizeof buf, "%d", shown);
    uint8_t ccol = down ? C_RED : fl && fl->t > 4 ? fl->col : C_TEXT;
    d_text(s, buf, 5 + hx, 68, ccol, 3);
    int curw = d_tw(buf, 3);
    snprintf(buf, sizeof buf, "/%d", k->hpMax);
    d_text(s, buf, 5 + hx + curw + 3, 75, C_DIM, 2);
    d_shield(s, 122, 52, 48, 42, C_DIM);
    d_shield(s, 124, 54, 44, 38, C_PANEL);
    snprintf(buf, sizeof buf, "%d", k->ac);
    d_text_c(s, buf, 122, 48, 56, C_TEXT, 3);
    d_text_c(s, "AC", 122, 48, 79, C_DIM, 1);
    d_rect(s, 172, 52, 83, 20, C_PANEL);
    d_icon(s, IC_BOLT, 176, 57, C_GOLD, C_LINE, C_NONE, 1);
    d_text(s, "INIT", 188, 59, C_DIM, 1);
    d_text_r(s, sg(dm), 250, 55, C_TEXT, 2);
    d_rect(s, 172, 74, 83, 20, C_PANEL);
    d_icon(s, IC_BOOT, 176, 79, C_DIM, C_LINE, C_NONE, 1);
    d_text(s, "SPEED", 188, 81, C_DIM, 1);
    snprintf(buf, sizeof buf, "%d", k->speed);
    d_text_r(s, buf, 250, 77, C_TEXT, 2);

    if (sum) {
        top_summary(s, sum);
    } else {
        // 3. abilities with their saving throw underneath
        for (int i = 0; i < 6; i++) {
            int x = i * 43, y = 97;
            bool p = (k->saveProf >> i) & 1;
            const char *sv = sg(rules_save(k, i));
            d_rect(s, x, y, 41, 45, C_PANEL);
            d_text(s, AB_NAMES[i], x + 4, y + 4, C_DIM, 1);
            snprintf(buf, sizeof buf, "%d", k->ab[i]);
            d_text_r(s, buf, x + 37, y + 4, C_DIM, 1);
            d_text_c(s, sg(rules_mod(k->ab[i])), x, 41, y + 13, C_TEXT, 2);
            d_rect(s, x, y + 30, 41, 15, C_PANEL2);
            bool lbl = d_tw("SAVE", 1) + 3 + d_tw(sv, 1) <= 39;
            int w = lbl ? d_tw("SAVE", 1) + 3 + d_tw(sv, 1) : d_tw(sv, 1);
            int x0 = x + floordiv(41 - w, 2);
            if (lbl)
                d_text(s, "SAVE", x0, y + 34, C_DIM, 1);
            d_text(s, sv, x0 + (lbl ? d_tw("SAVE", 1) + 3 : 0), y + 34, p ? C_GOLD : C_TEXT, 1);
        }
        // 4. secondary numbers
        d_rect(s, 0, 145, SCREEN_W, 31, C_PANEL);
        int x = pair(s, 5, 149, "PROF", sg(rules_pb(k)), C_TEXT);
        d_icon(s, IC_EYE, x, 149, C_DIM, C_GOLD, C_NONE, 1);
        snprintf(buf, sizeof buf, "%d", rules_passive_perception(k));
        pair(s, x + 15, 149, "PASSIVE PERCEPTION", buf, C_TEXT);
        bool has = k->spellAb != SPELL_AB_NONE;
        int y = 162;
        x = 5;
        if (has) {
            d_icon(s, IC_MSPARK, x, y, C_BLUE, C_NONE, C_NONE, 1);
            x += 10;
            snprintf(buf, sizeof buf, "%d", rules_spell_dc(k));
            x = pair(s, x, y, "SPELL DC", buf, C_TEXT);
            x = pair(s, x, y, "ATK", sg(rules_spell_attack(k)), C_TEXT);
        }
        bool slots = false;
        for (int l = 0; l < 9; l++)
            slots = slots || k->slotMax[l] > 0;
        if (slots) {
            if (!has) {
                d_text(s, "SLOTS", x, y, C_DIM, 1);
                x += 36;
            }
            int x0 = x;
            for (int l = 0; l < 9; l++) {
                int m = k->slotMax[l];
                if (!m)
                    continue;
                if (x + 8 + m * 6 > 254) {
                    x = x0;
                    y += 9;
                }
                snprintf(buf, sizeof buf, "%d", l + 1);
                d_text(s, buf, x, y, C_DIM, 1);
                x += 8;
                for (int i = 0; i < m; i++) {
                    if (i < m - k->slotUsed[l])
                        d_rect(s, x, y, 4, 7, C_BLUE);
                    else
                        d_box(s, x, y, 4, 7, C_DIM, 1);
                    x += 6;
                }
                x += 6;
            }
        } else if (!has) {
            d_text(s, "NO SPELLCASTING", x, y, C_DIM, 1);
        }
    }
    for (int i = 0; i < A.nfx; i++) {
        const Fx *f = &A.fx[i];
        if (f->kind != FX_FLOAT)
            continue;
        // Math.round(72 - (1 - t / T) * 16)
        int fy = 56 + floordiv(32 * f->t + f->T, 2 * f->T);
        if (f->t > 6 || (f->t & 1))
            d_text(s, f->text, 84 + hx, fy, f->col, 2);
    }

    // 5. status
    if (banner) {
        strip(s, banner, C_BLUE);
    } else if (A.toast.t > 0) {
        strip(s, A.toast.text, A.toast.col);
    } else {
        d_rect(s, 0, 178, SCREEN_W, 14, C_PANEL);
        if (down) {
            d_icon(s, IC_MSKULL, 4, 182, C_RED, C_NONE, C_NONE, 1);
            d_text(s, "DEATH SAVES", 14, 182, C_RED, 1);
            int px = 84;
            for (int i = 0; i < 3; i++, px += 8)
                d_text(s, i < k->ds ? G_DOT : G_RING, px, 182, i < k->ds ? C_GREEN : C_LINE, 1);
            px += 6;
            for (int i = 0; i < 3; i++, px += 9) {
                if (i < k->df)
                    d_icon(s, IC_MSKULL, px, 182, C_RED, C_NONE, C_NONE, 1);
                else
                    d_text(s, G_RING, px, 182, C_LINE, 1);
            }
        } else {
            d_icon(s, IC_MSTAR, 4, 182, k->insp ? C_GOLD : C_LINE, C_NONE, C_NONE, 1);
            d_text(s, k->insp ? "INSPIRED" : "NO INSPIRATION", 14, 182, k->insp ? C_GOLD : C_DIM, 1);
            snprintf(buf, sizeof buf, "HIT DICE %d/%d", k->level - k->hdUsed, k->level);
            d_text(s, buf, 104, 182, C_DIM, 1);
        }
        if (A.temp)
            d_text_r(s, "TEMP SESSION", 252, 182, C_BLUE, 1);
        else if (A.dirty)
            d_text_r(s, "UNSAVED", 252, 182, C_GOLD, 1);
        else
            d_text_r(s, "SAVED", 252, 182, C_GREEN, 1);
    }
    top_fx(s);
}

void top_base(Surface *s)
{
    if (A.view == VIEW_SHEET)
        top_sheet(s, NULL, NULL);
    else
        top_select(s);
}

static void status_toast(Surface *s)
{
    if (A.toast.t > 0)
        strip(s, A.toast.text, A.toast.col);
}

void top_page(Surface *s, const Summary *sum, int *scroll, int *maxScroll)
{
    uint8_t col = sum->col;
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_rect(s, 0, 0, SCREEN_W, 178, C_PANEL);
    d_rect(s, 0, 0, 2, 178, col);
    if (sum->icon >= 0) {
        int iy = 5 + floordiv(7 - icon_h(sum->icon), 2);
        d_icon(s, sum->icon, 6, iy > 1 ? iy : 1, col, C_NONE, C_NONE, 1);
    }
    int nx = sum->icon >= 0 ? 6 + icon_w(sum->icon) + 4 : 6;
    d_text(s, sum->title, nx, 5, C_TEXT, 1);
    if (sum->tag[0])
        d_text_r(s, sum->tag, 251, 5, sum->tagCol, 1);
    draw_textn_clip(s, sum->meta, (int)strlen(sum->meta), 41, 6, 16, C_DIM);
    int ry = 26;
    if (sum->meta2[0]) {
        draw_textn_clip(s, sum->meta2, (int)strlen(sum->meta2), 41, 6, 26, C_DIM);
        ry = 36;
    }
    d_rect(s, 6, ry, 244, 1, C_LINE);
    int n = text_layout(sum->desc ? sum->desc : "", 41, false, lines, MAX_LINES);
    int y0 = ry + 4, vis = (164 - y0) / 10;
    *maxScroll = n - vis > 0 ? n - vis : 0;
    *scroll = clampi(*scroll, 0, *maxScroll);
    for (int i = 0; i < vis && *scroll + i < n && *scroll + i < MAX_LINES; i++)
        d_textn(s, lines[*scroll + i].p, lines[*scroll + i].len, 6, y0 + i * 10, C_TEXT, 1);
    if (*maxScroll) {
        char marks[3] = { *scroll > 0 ? G_TRI_UP[0] : ' ', *scroll < *maxScroll ? G_TRI_DOWN[0] : 0, 0 };
        d_text(s, marks, 6, 167, C_GOLD, 1);
        d_text(s, "L/R SCROLL", 22, 167, C_DIM, 1);
    }
    d_text_r(s, "X: CLOSE", 251, 167, C_GOLD, 1);
    if (A.toast.t > 0) {
        strip(s, A.toast.text, A.toast.col);
    } else {
        d_rect(s, 0, 178, SCREEN_W, 14, C_BG);
        d_text_c(s, ch()->name, 0, SCREEN_W, 182, C_DIM, 1);
    }
}

static void wrapped_note(Surface *s, const char *text)
{
    int n = text_wrap(text, (int)strlen(text), 40, lines, MAX_LINES);
    for (int i = 0; i < n && i < MAX_LINES; i++)
        d_textn(s, lines[i].p, lines[i].len, 6, 44 + i * 10, C_TEXT, 1);
}

void top_spell(Surface *s, const char *name, const Spell *sp, int *scroll, int *maxScroll)
{
    char buf[64];
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_rect(s, 0, 0, SCREEN_W, 32, C_PANEL);
    d_text(s, name, 6, 4, C_TEXT, d_tw(name, 2) <= 244 ? 2 : 1);
    if (!sp) {
        d_text(s, "NOT IN YOUR SPELL LIBRARY", 6, 21, C_RED, 1);
        wrapped_note(s, "ADD IT TO /DATA/POCKETSHEET/SPELLS.TXT ON A PC (THE FILE EXPLAINS THE FORMAT), OR "
                        "REMOVE IT FROM THIS CHARACTER.");
    } else {
        if (sp->level == 0)
            snprintf(buf, sizeof buf, "%s CANTRIP", sp->school);
        else
            snprintf(buf, sizeof buf, "%s-LEVEL %s", ORD_NAMES[sp->level - 1], sp->school);
        d_text(s, buf, 6, 21, C_DIM, 1);
        int r = 250;
        if (sp->ritual) {
            d_text_r(s, "RITUAL", r, 21, C_GOLD, 1);
            r -= 42;
        }
        if (sp->conc)
            d_text_r(s, "CONC", r, 21, C_BLUE, 1);
        const char *labels[4] = { "TIME", "RANGE", "COMPONENTS", "DURATION" };
        const char *vals[4] = { sp->time, sp->range, sp->comp, sp->dur };
        for (int i = 0; i < 4; i++) {
            int x = 3 + (i % 2) * 126, y = 35 + (i / 2) * 20;
            d_rect(s, x, y, 124, 18, C_PANEL);
            d_text(s, labels[i], x + 4, y + 2, C_DIM, 1);
            d_text(s, vals[i][0] ? vals[i] : "-", x + 4, y + 10, C_TEXT, 1);
        }
        int n = text_layout(sp->desc, 40, false, lines, MAX_LINES), vis = 11;
        int s0 = 0;
        if (scroll) {
            *maxScroll = n - vis > 0 ? n - vis : 0;
            *scroll = clampi(*scroll, 0, *maxScroll);
            s0 = *scroll;
        }
        for (int i = 0; i < vis && s0 + i < n && s0 + i < MAX_LINES; i++)
            d_textn(s, lines[s0 + i].p, lines[s0 + i].len, 6, 78 + i * 10, C_TEXT, 1);
        if (s0 > 0)
            d_text_r(s, G_TRI_UP, 252, 78, C_GOLD, 1);
        if (s0 + vis < n)
            d_text_r(s, G_TRI_DOWN, 252, 178, C_GOLD, 1);
    }
    status_toast(s);
}

void top_feat(Surface *s, const char *name, const Feature *f, int *scroll, int *maxScroll)
{
    char buf[64];
    const Character *k = ch();
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_rect(s, 0, 0, SCREEN_W, 32, C_PANEL);
    d_text(s, name, 6, 4, C_TEXT, d_tw(name, 2) <= 244 ? 2 : 1);
    if (!f) {
        d_text(s, "NOT IN YOUR FEATURE LIBRARY", 6, 21, C_RED, 1);
        wrapped_note(s, "ADD IT TO /DATA/POCKETSHEET/FEATURES.TXT ON A PC, OR REMOVE IT FROM THIS CHARACTER.");
        status_toast(s);
        return;
    }
    snprintf(buf, sizeof buf, "%s%s", FCAT_NAMES[f->cat], f->fromFile ? "   FROM FEATURES.TXT" : "");
    d_text(s, buf, 6, 21, C_DIM, 1);
    if (f->inspire) {
        d_text_r(s, "INSPIRES", 250, 21, C_GOLD, 1);
        d_icon(s, IC_MSTAR, 250 - d_tw("INSPIRES", 1) - 10, 21, C_GOLD, C_NONE, C_NONE, 1);
    }
    int y0 = 38, max = features_max_uses(k, f);
    if (max) {
        int i = character_find_feat(k, name), used = i >= 0 ? k->feats[i].used : 0;
        d_rect(s, 3, 35, 250, 18, C_PANEL);
        d_text(s, "USES", 8, 41, C_DIM, 1);
        snprintf(buf, sizeof buf, "%d OF %d%s%s%s", max - used, max,
                 f->usesKind == USES_PB ? " (PROF BONUS)" : f->usesKind == USES_ABILITY ? " (" : "",
                 f->usesKind == USES_ABILITY ? AB_NAMES[f->usesVal] : "", f->usesKind == USES_ABILITY ? ")" : "");
        d_text(s, buf, 38, 41, C_GOLD, 1);
        d_text_r(s, RESET_NAMES[f->reset], 248, 41, C_DIM, 1);
        y0 = 58;
    }
    int n = text_layout(f->desc, 40, false, lines, MAX_LINES), vis = (176 - y0) / 10;
    int s0 = 0;
    if (scroll) {
        *maxScroll = n - vis > 0 ? n - vis : 0;
        *scroll = clampi(*scroll, 0, *maxScroll);
        s0 = *scroll;
    }
    for (int i = 0; i < vis && s0 + i < n && s0 + i < MAX_LINES; i++)
        d_textn(s, lines[s0 + i].p, lines[s0 + i].len, 6, y0 + i * 10, C_TEXT, 1);
    if (s0 > 0)
        d_text_r(s, G_TRI_UP, 252, y0, C_GOLD, 1);
    if (s0 + vis < n)
        d_text_r(s, G_TRI_DOWN, 252, 168, C_GOLD, 1);
    status_toast(s);
}

// ---------- weapons ----------

typedef struct {
    const char *p;
    int len;
    uint8_t col;
    int head; // leading chars drawn in gold ("REACH:")
} RichLine;

static char rich_text[8192];
static int rich_used;
static RichLine rich[MAX_LINES];
static int nrich;

static void rich_add(const char *p, int len, uint8_t col, int head)
{
    if (nrich < MAX_LINES)
        rich[nrich++] = (RichLine){ p, len, col, head };
}

static const char *rich_store(const char *s)
{
    int n = (int)strlen(s);
    if (rich_used + n + 1 > (int)sizeof rich_text)
        return "";
    char *p = rich_text + rich_used;
    memcpy(p, s, (size_t)n + 1);
    rich_used += n + 1;
    return p;
}

// Wraps one paragraph at 41 columns; the first line can have a gold head.
static void rich_para(const char *text, uint8_t col, int head)
{
    const char *p = rich_store(text);
    static TextLine wl[64];
    int n = text_wrap(p, (int)strlen(p), 41, wl, 64);
    for (int i = 0; i < n && i < 64; i++)
        rich_add(wl[i].p, wl[i].len, col, i == 0 ? head : 0);
}

// Library keys of the comma-separated weapon properties ("BARREL 6" -> "BARREL").
static int split_props(const char *props, char keys[][WEAPON_PROPS_LEN + 1], int max)
{
    int n = 0;
    const char *p = props;
    while (*p && n < max) {
        const char *e = strchr(p, ',');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        while (len && *p == ' ') {
            p++;
            len--;
        }
        while (len && p[len - 1] == ' ')
            len--;
        if (len)
            features_property_key(p, len, keys[n++]);
        if (!e)
            break;
        p = e + 1;
    }
    return n;
}

static void weapon_lines(const Character *k, const Weapon *w)
{
    static char buf[1400], para[600]; // static: the DS stack is small
    rich_used = nrich = 0;
    if (w->mastery) {
        snprintf(buf, sizeof buf, G_DIAMOND " MASTERY: %s", MASTERY_NAMES[w->mastery]);
        rich_add(rich_store(buf), (int)strlen(buf), C_BLUE, 0);
        rules_mastery_desc(k, w, buf, sizeof buf);
        const char *p = buf;
        for (int i = 0;; i++) {
            const char *nl = strchr(p, '\n');
            snprintf(para, sizeof para, "%.*s", nl ? (int)(nl - p) : (int)strlen(p), p);
            if (i)
                rich_add("", 0, C_TEXT, 0);
            rich_para(para, C_TEXT, 0);
            if (!nl)
                break;
            p = nl + 1;
        }
    } else {
        rich_add("NO MASTERY SET", 14, C_DIM, 0);
    }
    if (w->props[0]) {
        rich_add("", 0, C_TEXT, 0);
        snprintf(buf, sizeof buf, "PROPERTIES: %s", w->props);
        rich_para(buf, C_GOLD, 0);
        char keys[8][WEAPON_PROPS_LEN + 1];
        int nk = split_props(w->props, keys, 8);
        for (int j = 0; j < nk; j++) {
            const Feature *f = features_find(&A.flib, keys[j]);
            if (!f)
                continue;
            rich_add("", 0, C_TEXT, 0);
            const char *p = f->desc;
            for (int i = 0;; i++) {
                const char *nl = strchr(p, '\n');
                int len = nl ? (int)(nl - p) : (int)strlen(p);
                if (i == 0)
                    snprintf(buf, sizeof buf, "%s: %.*s", keys[j], len, p);
                else
                    snprintf(buf, sizeof buf, "%.*s", len, p);
                rich_para(buf, C_TEXT, i == 0 ? (int)strlen(keys[j]) + 1 : 0);
                if (!nl)
                    break;
                p = nl + 1;
            }
        }
    }
}

void top_weapon(Surface *s, const Character *k, const Weapon *w, int *scroll, int *maxScroll)
{
    char buf[96], d[24];
    int ai = rules_weapon_ability(k, w);
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_rect(s, 0, 0, SCREEN_W, 32, C_PANEL);
    d_text(s, w->name, 6, 4, C_TEXT, 2);
    d_icon(s, rules_weapon_is_ranged(w) ? IC_ARROW : IC_SWORD, 230, 5, C_GOLD, C_NONE, C_NONE, 2);
    char ab[24];
    if (ai < 0)
        snprintf(ab, sizeof ab, "NO SPELL ABILITY SET");
    else if (w->ab == WAB_FINESSE)
        snprintf(ab, sizeof ab, "FINESSE (%s)", AB_NAMES[ai]);
    else if (w->ab == WAB_SPELL)
        snprintf(ab, sizeof ab, "SPELL (%s)", AB_NAMES[ai]);
    else
        snprintf(ab, sizeof ab, "%s", AB_NAMES[ai]);
    char bonus[24] = "";
    if (w->bonus)
        snprintf(bonus, sizeof bonus, "   %s HIT BONUS", sg(w->bonus));
    snprintf(buf, sizeof buf, "%s   %s%s", ab, w->prof ? "PROFICIENT" : "NOT PROFICIENT", bonus);
    d_text(s, buf, 6, 21, ai < 0 ? C_RED : C_DIM, 1);
    d_rect(s, 1, 35, 82, 40, C_PANEL);
    d_text(s, "TO HIT", 6, 39, C_DIM, 1);
    d_text(s, sg(rules_weapon_to_hit(k, w)), 6, 50, C_GOLD, 3);
    rules_weapon_damage(w, d, sizeof d);
    int big = d_tw(d, 3) <= 160 ? 3 : 2;
    d_rect(s, 85, 35, 170, 40, C_PANEL);
    d_text(s, "DAMAGE", 90, 39, C_DIM, 1);
    d_text_r(s, DTYPE_NAMES[w->type], 250, 39, C_DIM, 1);
    d_text(s, d, 90, big == 3 ? 50 : 54, C_TEXT, big);
    const char *labels[3] = { "ABILITY", "PROF", "RANGE" };
    const char *vals[3] = { ai < 0 ? "-" : sg(rules_weapon_mod(k, w)), w->prof ? sg(rules_pb(k)) : "-",
                            w->range[0] ? w->range : "-" };
    for (int i = 0; i < 3; i++) {
        int x = 1 + i * 85;
        d_rect(s, x, 77, 83, 16, C_PANEL);
        d_text(s, labels[i], x + 5, 82, C_DIM, 1);
        d_text_r(s, vals[i], x + 78, 82, C_TEXT, 1);
    }
    weapon_lines(k, w);
    int vis = 8, s0 = 0;
    if (scroll) {
        *maxScroll = nrich - vis > 0 ? nrich - vis : 0;
        *scroll = clampi(*scroll, 0, *maxScroll);
        s0 = *scroll;
    }
    for (int i = 0; i < vis && s0 + i < nrich; i++) {
        const RichLine *l = &rich[s0 + i];
        int y = 98 + i * 10;
        if (l->head) {
            d_textn(s, l->p, l->head, 6, y, C_GOLD, 1);
            d_textn(s, l->p + l->head, l->len - l->head, 6 + l->head * 6, y, C_TEXT, 1);
        } else {
            d_textn(s, l->p, l->len, 6, y, l->col, 1);
        }
    }
    if (s0 > 0)
        d_text_r(s, G_TRI_UP, 252, 98, C_GOLD, 1);
    if (s0 + vis < nrich)
        d_text_r(s, G_TRI_DOWN, 252, 168, C_GOLD, 1);
    status_toast(s);
}

// ---------- summaries (one builder for the hover card and the full page) ----------

static void join(char *out, size_t cap, const char *sep, const char *const *parts, int n)
{
    out[0] = 0;
    for (int i = 0; i < n; i++) {
        if (!parts[i] || !parts[i][0])
            continue;
        if (out[0])
            strncat(out, sep, cap - strlen(out) - 1);
        strncat(out, parts[i], cap - strlen(out) - 1);
    }
}

bool summary_for(const Character *k, int kind, const char *name, int idx, Summary *out)
{
    memset(out, 0, sizeof *out);
    out->icon = -1;
    out->col = C_GOLD;
    out->tagCol = C_GOLD;
    if (kind == PEEK_SPELL) {
        int si = character_find_spell(k, name);
        const Spell *sp = spells_find(&A.lib, name);
        bool prep = si >= 0 && k->spells[si].prep;
        snprintf(out->title, sizeof out->title, "%s", name);
        out->icon = IC_MSPARK;
        if (!sp) {
            out->col = C_RED;
            snprintf(out->meta, sizeof out->meta, "NOT IN YOUR SPELL LIBRARY");
            out->desc = "ADD IT TO /DATA/POCKETSHEET/SPELLS.TXT ON A PC (THE FILE EXPLAINS THE FORMAT), OR REMOVE "
                        "IT FROM THIS CHARACTER.";
            return true;
        }
        out->col = C_BLUE;
        snprintf(out->tag, sizeof out->tag, "%s",
                 sp->level == 0 ? "CANTRIP" : prep ? G_DOT " PREPARED" : "NOT PREPARED");
        out->tagCol = sp->level == 0 || prep ? C_GOLD : C_DIM;
        char lvl[16] = "", comp[32] = "", dur[32] = "";
        if (sp->level)
            snprintf(lvl, sizeof lvl, "%s LEVEL", ORD_NAMES[sp->level - 1]);
        const char *m1[5] = { sp->level ? lvl : sp->school, sp->time, sp->range, sp->conc ? "CONC" : "",
                              sp->ritual ? "RITUAL" : "" };
        join(out->meta, sizeof out->meta, "  ", m1, 5);
        if (sp->comp[0])
            snprintf(comp, sizeof comp, "COMP %s", sp->comp);
        if (sp->dur[0])
            snprintf(dur, sizeof dur, "DUR %s", sp->dur);
        const char *m2[3] = { sp->level ? sp->school : "", comp, dur };
        join(out->meta2, sizeof out->meta2, "  ", m2, 3);
        out->desc = sp->desc;
        return true;
    }
    if (kind == PEEK_WEAPON) {
        if (idx < 0 || idx >= k->nWeapons)
            return false;
        const Weapon *w = &k->weapons[idx];
        int ai = rules_weapon_ability(k, w);
        static char buf[1400]; // static: the DS stack is small
        char d[24];
        snprintf(out->title, sizeof out->title, "%s", w->name);
        out->icon = rules_weapon_is_ranged(w) ? IC_ARROW : IC_SWORD;
        out->col = C_GOLD;
        snprintf(out->tag, sizeof out->tag, "%s TO HIT", sg(rules_weapon_to_hit(k, w)));
        out->tagCol = ai < 0 ? C_RED : C_GOLD;
        char dmg[48], range[24] = "";
        snprintf(dmg, sizeof dmg, "%s %s", rules_weapon_damage(w, d, sizeof d), DTYPE_NAMES[w->type]);
        if (w->range[0])
            snprintf(range, sizeof range, "RANGE %s", w->range);
        const char *m1[2] = { dmg, range };
        join(out->meta, sizeof out->meta, "   ", m1, 2);
        char ab[32], prof[16], bonus[24] = "";
        if (ai < 0)
            snprintf(ab, sizeof ab, "NO SPELL ABILITY");
        else
            snprintf(ab, sizeof ab, "%s%s %s", w->ab == WAB_FINESSE ? "FINESSE " : w->ab == WAB_SPELL ? "SPELL " : "",
                     AB_NAMES[ai], sg(rules_weapon_mod(k, w)));
        if (w->prof)
            snprintf(prof, sizeof prof, "PROF %s", sg(rules_pb(k)));
        else
            snprintf(prof, sizeof prof, "NOT PROFICIENT");
        if (w->bonus)
            snprintf(bonus, sizeof bonus, "HIT BONUS %s", sg(w->bonus));
        const char *m2[3] = { ab, prof, bonus };
        join(out->meta2, sizeof out->meta2, "   ", m2, 3);
        char *o = out->descBuf;
        size_t cap = sizeof out->descBuf;
        o[0] = 0;
        if (w->mastery) {
            rules_mastery_desc(k, w, buf, sizeof buf);
            snprintf(o, cap, "MASTERY: %s. %s", MASTERY_NAMES[w->mastery], buf);
        }
        if (w->props[0]) {
            size_t l = strlen(o);
            snprintf(o + l, cap - l, "%sPROPERTIES: %s.", l ? "\n" : "", w->props);
            char keys[8][WEAPON_PROPS_LEN + 1];
            int nk = split_props(w->props, keys, 8);
            for (int j = 0; j < nk; j++) {
                const Feature *f = features_find(&A.flib, keys[j]);
                if (!f)
                    continue;
                l = strlen(o);
                snprintf(o + l, cap - l, "\n%s: %s", keys[j], f->desc);
            }
        }
        out->desc = o[0] ? o : "NO MASTERY OR PROPERTIES SET.";
        return true;
    }
    // feature
    int fi = character_find_feat(k, name);
    const Feature *f = features_find(&A.flib, name);
    int max = features_max_uses(k, f);
    snprintf(out->title, sizeof out->title, "%s", name);
    out->icon = IC_MBOOK;
    if (!f) {
        out->col = C_RED;
        snprintf(out->meta, sizeof out->meta, "NOT IN YOUR FEATURE LIBRARY");
        out->desc = "ADD IT TO /DATA/POCKETSHEET/FEATURES.TXT ON A PC, OR REMOVE IT FROM THIS CHARACTER.";
        return true;
    }
    out->col = feat_cat_color(f->cat);
    if (max)
        snprintf(out->tag, sizeof out->tag, "%d OF %d LEFT", max - (fi >= 0 ? k->feats[fi].used : 0), max);
    snprintf(out->meta, sizeof out->meta, "%s%s%s%s", FCAT_NAMES[f->cat], max ? "   RESETS: " : "",
             max ? RESET_NAMES[f->reset] : "", f->inspire ? "   INSPIRES" : "");
    if (max && f->usesKind == USES_PB)
        snprintf(out->meta2, sizeof out->meta2, "USES = PROFICIENCY BONUS");
    else if (max && f->usesKind == USES_ABILITY)
        snprintf(out->meta2, sizeof out->meta2, "USES = %s MODIFIER", AB_NAMES[f->usesVal]);
    out->desc = f->desc;
    return true;
}
