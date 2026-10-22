// Weapon details (full card on the top screen) and the weapon editor.
#include <stdio.h>
#include <string.h>

#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

void open_weapon_detail(int idx)
{
    Overlay *o = ov_push(OV_WDETAIL);
    o->weapon.idx = idx;
}

void open_weapon_editor(int idx)
{
    Overlay *o = ov_push(OV_WEAPON);
    o->weapon.idx = idx;
}

static Weapon *cur_weapon(const Overlay *o)
{
    Character *k = ch();
    return o->weapon.idx < k->nWeapons ? &k->weapons[o->weapon.idx] : NULL;
}

static void pop_act(const Btn *b)
{
    (void)b;
    ov_pop();
}

static bool scroll_keys(Overlay *o, const Input *in)
{
    if (in->rep & KBIT(K_L)) {
        o->scroll = o->scroll - 2 > 0 ? o->scroll - 2 : 0;
        return true;
    }
    if (in->rep & KBIT(K_R)) {
        o->scroll = o->scroll + 2 < o->maxScroll ? o->scroll + 2 : o->maxScroll;
        return true;
    }
    return false;
}

static void weapon_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    Overlay *o = ov_top();
    top_weapon(s, ch(), cur_weapon(o), &o->scroll, &o->maxScroll);
}

// ---------- details ----------

static void detail_edit_act(const Btn *b)
{
    (void)b;
    open_weapon_editor(ov_top()->weapon.idx);
}

static void detail_step_act(const Btn *b)
{
    Overlay *o = ov_top();
    int n = ch()->nWeapons;
    o->weapon.idx = (o->weapon.idx + (b->b < 0 ? n - 1 : 1)) % n;
    o->scroll = 0;
    A.focusTab[TAB_ARMS][0] = BID(B_WEAPON, o->weapon.idx);
}

static void detail_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    const Character *k = ch();
    const Weapon *w = cur_weapon(o);
    char buf[64], d[24], tag[24];
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, "WEAPON", 6, 5, C_GOLD, 1);
    snprintf(buf, sizeof buf, "%d OF %d", o->weapon.idx + 1, k->nWeapons);
    d_text_r(s, buf, 250, 5, C_DIM, 1);
    d_rect(s, 3, 88, 250, 64, C_PANEL);
    d_text(s, w->name, 9, 94, C_TEXT, 1);
    snprintf(buf, sizeof buf, "%s TO HIT", sg(rules_weapon_to_hit(k, w)));
    d_text(s, buf, 9, 108, C_GOLD, 1);
    snprintf(buf, sizeof buf, "%s %s", rules_weapon_damage(w, d, sizeof d), DTYPE_NAMES[w->type]);
    d_text(s, buf, 9, 120, C_TEXT, 1);
    if (w->mastery) {
        snprintf(buf, sizeof buf, G_DIAMOND " %s", rules_mastery_tag(k, w, tag, sizeof tag));
        d_text(s, buf, 9, 132, C_BLUE, 1);
    } else {
        d_text(s, "NO MASTERY", 9, 132, C_DIM, 1);
    }
    if (w->props[0])
        d_text(s, w->props, 9, 143, C_DIM, 1); // at most 28 chars, inside the 40-char limit
    d_text(s, "DETAILS ON TOP SCREEN", 6, 163, C_DIM, 1);
}

static bool detail_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    Overlay *o = ov_top();
    if (in->down & KBIT(K_B)) {
        ov_pop();
        return true;
    }
    return scroll_keys(o, in);
}

void ov_wdetail_build(Ctx *cx, Overlay *o)
{
    if (!cur_weapon(o)) {
        ov_pop();
        ctx_now(cx);
        return;
    }
    int n = ch()->nWeapons;
    add_plain(cx, BID(B_WDEDIT, 0), 3, 22, 250, 28, "EDIT WEAPON", C_GOLD, C_INK, 1)->act = detail_edit_act;
    if (n > 1) {
        Btn *b = add_plain(cx, BID(B_WDPREV, 0), 3, 56, 123, 26, G_TRI_LEFT " PREVIOUS", C_PANEL2, C_TEXT, 1);
        b->act = detail_step_act;
        b->b = -1;
        b = add_plain(cx, BID(B_WDNEXT, 0), 130, 56, 123, 26, "NEXT " G_TRI_RIGHT, C_PANEL2, C_TEXT, 1);
        b->act = detail_step_act;
        b->b = 1;
    }
    add_plain(cx, BID(B_WDBACK, 0), 153, 156, 100, 20, "BACK", C_PANEL2, C_TEXT, 1)->act = pop_act;
    cx->def = BID(B_WDEDIT, 0);
    ctx_footer(cx, "A", "PRESS", "L/R", "SCROLL", "B", "BACK", (const char *)NULL);
    cx->bot = detail_bot;
    cx->top = weapon_top;
    cx->onKeys = detail_keys;
}

// ---------- editor ----------

enum { WF_NAME, WF_ABILITY, WF_PROF, WF_HIT, WF_DICE, WF_DIE, WF_DMG, WF_TYPE, WF_MASTERY, WF_RANGE, WF_PROPS, WF_COUNT };
static const char *const FIELD_LABELS[WF_COUNT] = { "NAME",         "ABILITY",     "PROFICIENT", "HIT BONUS",
                                                    "DAMAGE DICE",  "DIE SIZE",    "DAMAGE BONUS", "DAMAGE TYPE",
                                                    "MASTERY",      "RANGE",       "PROPERTIES" };
static const char *field_values[WF_COUNT];
static List fieldList;

static void field_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    Btn r = *b;
    r.label = FIELD_LABELS[b->a];
    r.label2 = field_values[b->a];
    r.arrow = b->a == WF_NAME || b->a == WF_HIT || b->a == WF_DICE || b->a == WF_DMG || b->a == WF_RANGE ||
              b->a == WF_PROPS;
    btn_row(s, &r, focus, press);
}

static void field_act(const Btn *b)
{
    Overlay *o = ov_top();
    int idx = o->weapon.idx;
    Weapon *w = cur_weapon(o);
    switch (b->a) {
    case WF_NAME:
        open_kbd("WEAPON NAME", "NAME", w->name, WEAPON_NAME_LEN, false, KT_WNAME, idx);
        return;
    case WF_ABILITY:
        w->ab = (uint8_t)((w->ab + 1) % 4);
        break;
    case WF_PROF:
        w->prof ^= 1;
        break;
    case WF_HIT:
        open_stepper("TO-HIT BONUS (MAGIC, ARCHERY)", SF_WHIT, idx, -5, 10, 1, 1);
        return;
    case WF_DICE:
        open_stepper("NUMBER OF DICE", SF_WDICE, idx, 1, 9, 1, 1);
        return;
    case WF_DIE: {
        int i = 0;
        while (i < 5 && DIE_SIZES[i] != w->die)
            i++;
        w->die = DIE_SIZES[(i + 1) % 5];
        break;
    }
    case WF_DMG:
        open_stepper("DAMAGE BONUS (ADDED TO DICE)", SF_WDMG, idx, -10, 30, 1, 5);
        return;
    case WF_TYPE:
        w->type = (uint8_t)((w->type + 1) % DTYPE_COUNT);
        break;
    case WF_MASTERY:
        w->mastery = (uint8_t)((w->mastery + 1) % MASTERY_COUNT);
        break;
    case WF_RANGE:
        open_kbd("RANGE (E.G. 5 FT, 80/320)", "RANGE", w->range, WEAPON_RANGE_LEN, true, KT_WRANGE, idx);
        return;
    default:
        open_kbd("PROPERTIES, COMMA SEPARATED", "PROPERTIES", w->props, WEAPON_PROPS_LEN, true, KT_WPROPS, idx);
        return;
    }
    A.dirty = true;
}

static void del_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_PANEL);
    d_text(s, G_CROSS, b->x + 6, b->y + 5, press ? C_INK : C_RED, 1);
    d_text(s, "DELETE...", b->x + 16, b->y + 5, press ? C_INK : C_TEXT, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void del_act(const Btn *b)
{
    (void)b;
    const Overlay *o = ov_top();
    const Weapon *w = cur_weapon(o);
    int idx = o->weapon.idx;
    Notice *n = notice_push();
    snprintf(n->title, sizeof n->title, "DELETE %s?", w->name);
    n->err = true;
    snprintf(n->sub, sizeof n->sub, "REMOVES THIS WEAPON");
    snprintf(n->lines[0], sizeof n->lines[0], "THIS REMOVES THE WEAPON FROM THIS CHARACTER.");
    n->nlines = 1;
    n->btns[0] = (NoticeBtn){ "KEEP", NB_PLAIN, NA_CLOSE };
    n->btns[1] = (NoticeBtn){ "DELETE", NB_DANGER, NA_DELETE_WEAPON };
    n->nbtns = 2;
    n->def = 0;
    n->cancel = 0;
    n->arg = idx;
}

static void editor_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    char buf[16];
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, "EDIT WEAPON", 6, 5, C_GOLD, 1);
    snprintf(buf, sizeof buf, "%d OF %d", ov_top()->weapon.idx + 1, ch()->nWeapons);
    d_text_r(s, buf, 250, 5, C_DIM, 1);
}

static bool editor_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    if (in->down & (KBIT(K_B) | KBIT(K_START))) {
        ov_pop();
        return true;
    }
    return list_keys(&fieldList, in);
}

void ov_weapon_build(Ctx *cx, Overlay *o)
{
    const Character *k = ch();
    const Weapon *w = cur_weapon(o);
    if (!w) {
        ov_pop();
        ctx_now(cx);
        return;
    }
    int ai = rules_weapon_ability(k, w);
    field_values[WF_NAME] = w->name;
    if (w->ab == WAB_FINESSE)
        field_values[WF_ABILITY] = ctx_str(cx, "FINESSE (%s)", AB_NAMES[ai < 0 ? 0 : ai]);
    else if (w->ab == WAB_SPELL)
        field_values[WF_ABILITY] =
            ctx_str(cx, "SPELL (%s)", k->spellAb == SPELL_AB_NONE ? "NONE" : AB_NAMES[k->spellAb]);
    else
        field_values[WF_ABILITY] = WAB_NAMES[w->ab];
    field_values[WF_PROF] = w->prof ? "YES" : "NO";
    field_values[WF_HIT] = w->bonus ? ctx_str(cx, "%s", sg(w->bonus)) : "NONE";
    field_values[WF_DICE] = ctx_str(cx, "%d DICE", w->n);
    field_values[WF_DIE] = ctx_str(cx, "D%d", w->die);
    field_values[WF_DMG] = w->dmg ? ctx_str(cx, "%s", sg(w->dmg)) : "NONE";
    field_values[WF_TYPE] = DTYPE_NAMES[w->type];
    field_values[WF_MASTERY] = MASTERY_NAMES[w->mastery];
    field_values[WF_RANGE] = w->range[0] ? w->range : "-";
    if (!w->props[0])
        field_values[WF_PROPS] = "-";
    else if (strlen(w->props) > 18)
        field_values[WF_PROPS] = ctx_str(cx, "%.18s..", w->props);
    else
        field_values[WF_PROPS] = w->props;
    fieldList = (List){ .kind = B_WFIELD, .focus = cx->focus, .scroll = &A.scroll[LIST_WEAPON], .n = WF_COUNT,
                        .cols = 1, .rows = 9, .x = 3, .y = 18, .w = 226, .h = 16, .draw = field_draw,
                        .act = field_act };
    list_build(cx, &fieldList);
    Btn *b = ctx_add(cx, BID(B_WDEL, 0), 3, 163, 100, 16);
    b->draw = del_draw;
    b->act = del_act;
    add_plain(cx, BID(B_WDONE, 0), 153, 163, 100, 16, "DONE", C_GOLD, C_INK, 1)->act = pop_act;
    cx->def = BID(B_WFIELD, 0);
    cx->edit = true;
    ctx_footer(cx, "A", "CHANGE", "B", "DONE", G_TRI_UP G_TRI_DOWN, "FIELD", (const char *)NULL);
    cx->bot = editor_bot;
    cx->top = weapon_top;
    cx->onKeys = editor_keys;
}
