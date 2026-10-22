// SKILLS tab: all 18 skills at once; the focused skill's math in the strip.
#include <stdio.h>

#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

static const char *const PROF_GLYPH[3] = { G_RING, G_DOT, G_DIAMOND };
static const uint8_t PROF_COL[3] = { C_LINE, C_GOLD, C_BLUE };

// b->a = skill index, b->b = 1 on the EDIT tab (modifiers in gold).
void skill_row_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    const Character *k = ch();
    int i = b->a, p = k->skill[i], h = b->h - 1;
    d_rect(s, b->x, b->y, b->w, h, press ? C_PANEL2 : C_PANEL);
    int ty = b->y + floordiv(h - 7, 2);
    d_text(s, PROF_GLYPH[p], b->x + 3, ty, PROF_COL[p], 1);
    d_text(s, SKILLS[i].name, b->x + 11, ty, C_TEXT, 1);
    d_text_r(s, sg(rules_skill(k, i)), b->x + b->w - 4, ty, p == 2 ? C_BLUE : b->b ? C_GOLD : C_TEXT, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, h);
}

static void skills_bot(Surface *s, const Ctx *cx)
{
    const Character *k = ch();
    uint16_t f = *cx->focus;
    int i = BKIND(f) == B_SKILL ? BINDEX(f) : 0;
    int ab = SKILLS[i].ability, p = k->skill[i], tot = rules_skill(k, i), pb = rules_pb(k);
    char buf[96], part[48];
    d_rect(s, 3, 163, 250, 15, C_PANEL2);
    if (p == 2 && i == SKILL_PERCEPTION)
        snprintf(part, sizeof part, " + EXPERT %s", sg(2 * pb));
    else if (p == 2)
        snprintf(part, sizeof part, " + EXPERTISE %s (2X PROF)", sg(2 * pb));
    else if (p == 1)
        snprintf(part, sizeof part, " + PROF %s", sg(pb));
    else
        snprintf(part, sizeof part, "  NOT PROFICIENT");
    int n = snprintf(buf, sizeof buf, "%s %s%s = %s", AB_NAMES[ab], sg(rules_mod(k->ab[ab])), part, sg(tot));
    if (i == SKILL_PERCEPTION && n > 0 && n < (int)sizeof buf)
        snprintf(buf + n, sizeof buf - (size_t)n, "  PASSIVE %d", 10 + tot);
    d_text(s, buf, 7, 167, C_TEXT, 1);
}

void tab_skills(Ctx *cx, const Character *k)
{
    (void)k;
    for (int i = 0; i < SKILL_COUNT; i++) {
        Btn *b = ctx_add(cx, BID(B_SKILL, i), 3 + (i < 9 ? 0 : 126), 17 + (i % 9) * 16, 124, 16);
        b->draw = skill_row_draw;
        b->a = i;
    }
    cx->def = BID(B_SKILL, 0);
    ctx_footer(cx, G_TRI_UP G_TRI_DOWN, "BROWSE", "L/R", "TABS", "ST", "MENU", "SE", "EDIT", (const char *)NULL);
    cx->bot = skills_bot;
}
