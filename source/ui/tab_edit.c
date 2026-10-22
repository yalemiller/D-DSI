// EDIT tab: INFO (identity and numbers), ABILITIES (scores and save
// proficiencies), SKILLS (none -> proficient -> expertise).
#include <stdio.h>
#include <string.h>

#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

static List infoList;

enum { F_NAME, F_CLASS, F_SUB, F_SPECIES, F_BG, F_LEVEL, F_HPMAX, F_HITDIE, F_AC, F_SPEED, F_SPELLAB, F_COUNT };
static const char *const INFO_LABELS[F_COUNT] = { "NAME",   "CLASS",   "SUBCLASS",    "SPECIES", "BACKGROUND",   "LEVEL",
                                                  "MAX HP", "HIT DIE", "ARMOR CLASS", "SPEED",   "SPELL ABILITY" };
static const char *info_values[F_COUNT];

static void page_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    bool on = A.editPage == b->a;
    d_rect(s, b->x, b->y, b->w, b->h, on ? C_BLUE : press ? C_PANEL2 : C_PANEL);
    d_text_c(s, b->label, b->x, b->w, b->y + 4, on ? C_INK : C_DIM, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void page_act(const Btn *b)
{
    A.editPage = b->a;
}

static void info_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    Btn r = *b;
    r.label = INFO_LABELS[b->a];
    r.label2 = info_values[b->a];
    r.arrow = b->a != F_HITDIE && b->a != F_SPELLAB;
    btn_row(s, &r, focus, press);
}

static void info_act(const Btn *b)
{
    Character *k = ch();
    switch (b->a) {
    case F_NAME:
        open_kbd("NAME", "NAME", k->name, NAME_LEN, false, KT_NAME, 0);
        break;
    case F_CLASS:
        open_kbd("CLASS", "CLASS", k->cls, CLASS_LEN, false, KT_CLASS, 0);
        break;
    case F_SUB:
        open_kbd("SUBCLASS", "SUBCLASS", k->sub, SUBCLASS_LEN, true, KT_SUB, 0);
        break;
    case F_SPECIES:
        open_kbd("SPECIES", "SPECIES", k->species, SPECIES_LEN, true, KT_SPECIES, 0);
        break;
    case F_BG:
        open_kbd("BACKGROUND", "BACKGROUND", k->background, BACKGROUND_LEN, true, KT_BACKGROUND, 0);
        break;
    case F_LEVEL:
        open_stepper("LEVEL", SF_LEVEL, 0, 1, 20, 1, 5);
        break;
    case F_HPMAX:
        open_stepper("MAX HP", SF_HPMAX, 0, 1, 999, 1, 10);
        break;
    case F_HITDIE: {
        int i = 0;
        while (i < 4 && HIT_DIE_SIZES[i] != k->hitDie)
            i++;
        k->hitDie = HIT_DIE_SIZES[(i + 1) % 4];
        A.dirty = true;
        break;
    }
    case F_AC:
        open_stepper("ARMOR CLASS", SF_AC, 0, 0, 40, 1, 5);
        break;
    case F_SPEED:
        open_stepper("SPEED (FEET)", SF_SPEED, 0, 0, 120, 5, 10);
        break;
    default: {
        int sa = k->spellAb;
        k->spellAb = (uint8_t)(sa == SPELL_AB_NONE ? 0 : sa == 5 ? SPELL_AB_NONE : sa + 1);
        A.dirty = true;
        break;
    }
    }
}

static void score_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    int i = b->a;
    char buf[24];
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    d_text(s, AB_FULL[i], b->x + 6, b->y + 7, C_TEXT, 1);
    snprintf(buf, sizeof buf, "%d  (%s)", k->ab[i], sg(rules_mod(k->ab[i])));
    d_text_r(s, buf, b->x + b->w - 6, b->y + 7, C_GOLD, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void score_act(const Btn *b)
{
    char title[32];
    snprintf(title, sizeof title, "%s SCORE", AB_FULL[b->a]);
    open_stepper(title, SF_ABILITY, b->a, 1, 30, 1, 5);
}

static void save_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    bool p = (ch()->saveProf >> b->a) & 1;
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_PANEL2 : C_PANEL);
    d_text(s, "SAVE PROF", b->x + 6, b->y + 7, C_DIM, 1);
    d_text_r(s, p ? G_DOT : G_RING, b->x + b->w - 6, b->y + 7, p ? C_GOLD : C_LINE, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void save_act(const Btn *b)
{
    ch()->saveProf ^= (uint8_t)(1 << b->a);
    A.dirty = true;
}

static void set_skill(int i, int v)
{
    static const char *const STATE[3] = { "NOT PROFICIENT", "PROFICIENT", "EXPERTISE" };
    Character *k = ch();
    k->skill[i] = (uint8_t)v;
    A.dirty = true;
    toastf(v == 2 ? C_BLUE : v ? C_GOLD : C_DIM, "%s: %s (%s)", SKILLS[i].name, STATE[v], sg(rules_skill(k, i)));
}

static void skill_act(const Btn *b)
{
    set_skill(b->a, (ch()->skill[b->a] + 1) % 3);
}

static void skill_y(const Btn *b)
{
    set_skill(b->a, ch()->skill[b->a] == 2 ? 1 : 2);
}

static bool info_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    return list_keys(&infoList, in);
}

static void skills_legend(Surface *s, const Ctx *cx)
{
    (void)cx;
    static const char *const GLYPH[3] = { G_RING, G_DOT, G_DIAMOND };
    static const uint8_t COL[3] = { C_LINE, C_GOLD, C_BLUE };
    int pb = rules_pb(ch());
    char l[3][24];
    snprintf(l[0], sizeof l[0], "NONE");
    snprintf(l[1], sizeof l[1], "PROF %s", sg(pb));
    snprintf(l[2], sizeof l[2], "EXPERTISE %s", sg(pb * 2));
    int x = 5;
    for (int p = 0; p < 3; p++) {
        d_text(s, GLYPH[p], x, 170, COL[p], 1);
        d_text(s, l[p], x + 8, 170, p ? COL[p] : C_DIM, 1);
        x += 8 + d_tw(l[p], 1) + 14;
    }
}

void tab_edit(Ctx *cx, Character *k)
{
    static const char *const PAGES[3] = { "INFO", "ABILITIES", "SKILLS" };
    for (int i = 0; i < 3; i++) {
        Btn *b = ctx_add(cx, BID(B_EDPAGE, i), 3 + i * 84, 17, 82, 14);
        b->draw = page_draw;
        b->act = page_act;
        b->a = i;
        b->label = PAGES[i];
    }
    int pg = A.editPage;
    if (pg == 0) {
        info_values[F_NAME] = k->name;
        info_values[F_CLASS] = k->cls;
        info_values[F_SUB] = k->sub[0] ? k->sub : "-";
        info_values[F_SPECIES] = k->species[0] ? k->species : "-";
        info_values[F_BG] = k->background[0] ? k->background : "-";
        info_values[F_LEVEL] = ctx_str(cx, "%d", k->level);
        info_values[F_HPMAX] = ctx_str(cx, "%d", k->hpMax);
        info_values[F_HITDIE] = ctx_str(cx, "D%d", k->hitDie);
        info_values[F_AC] = ctx_str(cx, "%d", k->ac);
        info_values[F_SPEED] = ctx_str(cx, "%d", k->speed);
        info_values[F_SPELLAB] = k->spellAb == SPELL_AB_NONE ? "NONE" : AB_NAMES[k->spellAb];
        infoList = (List){ .kind = B_EDROW, .focus = cx->focus, .scroll = &A.scroll[LIST_EDIT], .n = F_COUNT,
                           .cols = 1, .rows = 9, .x = 3, .y = 33, .w = 226, .h = 16, .draw = info_draw,
                           .act = info_act };
        list_build(cx, &infoList);
        cx->onKeys = info_keys;
    } else if (pg == 1) {
        for (int i = 0; i < 6; i++) {
            int y = 34 + i * 24;
            Btn *b = ctx_add(cx, BID(B_EDSCORE, i), 3, y, 168, 21);
            b->draw = score_draw;
            b->act = score_act;
            b->a = i;
            b = ctx_add(cx, BID(B_EDSAVE, i), 174, y, 79, 21);
            b->draw = save_draw;
            b->act = save_act;
            b->a = i;
        }
    } else {
        for (int i = 0; i < SKILL_COUNT; i++) {
            Btn *b = ctx_add(cx, BID(B_EDSKILL, i), 3 + (i < 9 ? 0 : 126), 32 + (i % 9) * 15, 124, 15);
            b->draw = skill_row_draw;
            b->act = skill_act;
            b->onY = skill_y;
            b->a = i;
            b->b = 1;
        }
        cx->bot = skills_legend;
    }
    static const uint16_t DEF[3] = { BID(B_EDROW, 0), BID(B_EDSCORE, 0), BID(B_EDSKILL, 0) };
    cx->def = DEF[pg];
    cx->edit = true;
    if (pg == 2)
        ctx_footer(cx, "A", "CYCLE", "Y", "EXPERTISE", "X", "PAGE", "SE", "DONE", (const char *)NULL);
    else
        ctx_footer(cx, "A", "CHANGE", "X", "PAGE", "L/R", "TABS", "SE", "DONE", (const char *)NULL);
}
