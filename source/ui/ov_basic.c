// Overlay dispatch plus the general-purpose overlays: notice/confirm, the
// Start menu, the number stepper and the on-screen keyboard.
#include <stdio.h>
#include <string.h>

#include "core/actions.h"
#include "core/rules.h"
#include "core/text.h"
#include "ui.h"

static void ov_notice_build(Ctx *cx, Overlay *o);
static void ov_menu_build(Ctx *cx, Overlay *o);
static void ov_stepper_build(Ctx *cx, Overlay *o);
static void ov_kbd_build(Ctx *cx, Overlay *o);

void ov_build(Ctx *cx, Overlay *o)
{
    cx->key = 0x10000u | o->uid;
    cx->focus = &o->focus;
    switch (o->type) {
    case OV_NOTICE:
        ov_notice_build(cx, o);
        break;
    case OV_MENU:
        ov_menu_build(cx, o);
        break;
    case OV_STEPPER:
        ov_stepper_build(cx, o);
        break;
    case OV_KBD:
        ov_kbd_build(cx, o);
        break;
    case OV_SREST:
        ov_srest_build(cx, o);
        break;
    case OV_SPELL:
        ov_spell_build(cx, o);
        break;
    case OV_SPELLPICK:
        ov_spellpick_build(cx, o);
        break;
    case OV_SLOTSET:
        ov_slotset_build(cx, o);
        break;
    case OV_FEATPICK:
        ov_featpick_build(cx, o);
        break;
    case OV_WDETAIL:
        ov_wdetail_build(cx, o);
        break;
    case OV_WEAPON:
        ov_weapon_build(cx, o);
        break;
    }
}

static void wrap_lines(Surface *s, const char *text, int cols, int x, int *y, int step, uint8_t col, int maxLines)
{
    static TextLine wl[16];
    int n = text_wrap(text, (int)strlen(text), cols, wl, 16);
    for (int i = 0; i < n && i < 16 && i < maxLines; i++) {
        d_textn(s, wl[i].p, wl[i].len, x, *y, col, 1);
        *y += step;
    }
}

// ---------- notice / confirm ----------

void notice_act(int act, const Notice *n)
{
    switch (act) {
    case NA_RETRY_SAVE:
        ov_pop();
        session_save_now();
        break;
    case NA_TRY_MOUNT:
        session_try_mount();
        break;
    case NA_NOSAVE:
        session_nosave_mode();
        break;
    case NA_TEMP_SESSION:
        session_temp_session();
        break;
    case NA_DELETE_CHAR:
        session_delete(n->arg);
        break;
    case NA_LONG_REST:
        do_long_rest();
        break;
    case NA_DELETE_WEAPON: {
        act_delete_weapon(ch(), n->arg);
        A.dirty = true;
        while (A.nov && ov_top()->type != OV_MENU)
            ov_pop();
        toast("WEAPON DELETED", C_GOLD);
        break;
    }
    case NA_REMOVE_SPELL:
        act_remove_spell(ch(), n->name);
        A.dirty = true;
        ov_pop();
        ov_pop();
        toastf(C_DIM, "REMOVED %s", n->name);
        break;
    default:
        ov_pop();
        break;
    }
}

static void notice_btn_act(const Btn *b)
{
    Overlay *o = ov_top();
    Notice copy = o->notice; // the handler may pop the overlay
    notice_act(copy.btns[b->a].act, &copy);
}

static void notice_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Notice *n = &ov_top()->notice;
    d_rect(s, 0, 0, SCREEN_W, 16, n->err ? C_RED : C_PANEL);
    d_text(s, n->title, 6, 5, n->err ? C_INK : C_GOLD, 1);
    int y = 26;
    for (int i = 0; i < n->nlines; i++)
        wrap_lines(s, n->lines[i], 40, 8, &y, 11, C_TEXT, 16);
    if (n->status[0])
        d_text(s, n->status, 8, 122, C_RED, 1);
}

static void notice_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Notice *n = &ov_top()->notice;
    int t = A.toast.t;
    A.toast.t = 0;
    top_base(s); // the sheet (or the select screen) under the wash, without its toast
    A.toast.t = t;
    d_dim(s);
    d_rect(s, 16, 64, 224, 64, C_PANEL);
    d_box(s, 16, 64, 224, 64, n->err ? C_RED : C_GOLD, 1);
    int sc = d_tw(n->title, 2) <= 208 ? 2 : 1;
    d_text_c(s, n->title, 16, 224, sc == 2 ? 78 : 82, n->err ? C_RED : C_GOLD, sc);
    d_text_c(s, n->sub[0] ? n->sub : "SEE THE LOWER SCREEN", 16, 224, 104, C_DIM, 1);
}

static bool notice_keys(const Ctx *cx, const Input *in)
{
    const Notice *n = &ov_top()->notice;
    if (in->down & KBIT(K_B)) {
        int ci = n->cancel >= 0 ? n->cancel : n->nbtns == 1 ? 0 : -1;
        if (ci >= 0) {
            const Btn *b = ctx_find(cx, BID(B_NOTICE, ci));
            if (b)
                b->act(b);
        }
        return true;
    }
    return false;
}

static void ov_notice_build(Ctx *cx, Overlay *o)
{
    Notice *n = &o->notice;
    for (int j = 0; j < n->nbtns; j++) {
        const NoticeBtn *nb = &n->btns[j];
        uint8_t bg = nb->style == NB_DANGER ? C_RED : nb->style == NB_PRIMARY ? C_GOLD : C_PANEL2;
        uint8_t fg = nb->style == NB_PLAIN ? C_TEXT : C_INK;
        Btn *b = add_plain(cx, BID(B_NOTICE, j), n->nbtns == 1 ? 68 : (j == 0 ? 12 : 134), 136,
                           n->nbtns == 1 ? 120 : 110, 26, nb->label, bg, fg, 1);
        b->act = notice_btn_act;
        b->a = j;
    }
    cx->def = BID(B_NOTICE, n->def);
    int ci = n->cancel >= 0 ? n->cancel : n->nbtns == 1 ? 0 : -1;
    if (ci >= 0)
        ctx_footer(cx, G_TRI_LEFT G_TRI_RIGHT, "CHOOSE", "A", "OK", "B", n->btns[ci].label, (const char *)NULL);
    else
        ctx_footer(cx, G_TRI_LEFT G_TRI_RIGHT, "CHOOSE", "A", "OK", (const char *)NULL);
    cx->bot = notice_bot;
    cx->top = notice_top;
    cx->onKeys = notice_keys;
}

// ---------- Start menu ----------

enum { M_RESUME, M_SAVE, M_SREST, M_LREST, M_EDIT, M_CHARS, M_COUNT };
static const char *const MENU_LABELS[M_COUNT] = { "RESUME", "SAVE NOW", "SHORT REST", "LONG REST", "EDIT SHEET",
                                                  "CHARACTERS" };

void open_menu(void)
{
    ov_push(OV_MENU);
}

static void menu_act(const Btn *b)
{
    switch (b->a) {
    case M_RESUME:
        ov_pop();
        break;
    case M_SAVE:
        ov_pop();
        session_save_now();
        break;
    case M_SREST:
        ov_pop();
        open_short_rest();
        break;
    case M_LREST:
        ask_long_rest();
        break;
    case M_EDIT:
        ov_pop();
        A.prevTab = A.tab == TAB_EDIT ? A.prevTab : A.tab;
        A.tab = TAB_EDIT;
        break;
    default:
        ov_pop();
        session_leave();
        break;
    }
}

static void menu_draw(Surface *s, const Btn *b, bool focus, bool press)
{
    d_rect(s, b->x, b->y, b->w, b->h, press ? C_TEXT : C_PANEL);
    d_text(s, b->label, b->x + 8, b->y + 5, press ? C_INK : C_TEXT, 1);
    if (focus)
        draw_focus(s, b->x, b->y, b->w, b->h);
}

static void menu_bot(Surface *s, const Ctx *cx)
{
    char buf[128];
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, "MENU", 6, 5, C_GOLD, 1);
    snprintf(buf, sizeof buf, "SLOT %d", A.cur + 1);
    d_text_r(s, buf, 250, 5, C_DIM, 1);
    switch (BINDEX(ctx_focus(cx))) {
    case M_RESUME:
        snprintf(buf, sizeof buf, "BACK TO THE SHEET. B OR START ALSO CLOSE THIS MENU.");
        break;
    case M_SAVE:
        snprintf(buf, sizeof buf, "WRITES FAT:/DATA/POCKETSHEET/SHEET.SAV. %s",
                 A.temp ? "TEMPORARY SESSION: SAVING IS OFF." : A.dirty ? "YOU HAVE UNSAVED CHANGES."
                                                                        : "EVERYTHING IS SAVED.");
        break;
    case M_SREST:
        snprintf(buf, sizeof buf, "SPEND HIT DICE TO REGAIN HP. YOU ENTER WHAT YOU ROLLED, OR USE THE AVERAGE.");
        break;
    case M_LREST:
        snprintf(buf, sizeof buf,
                 "RESTORES ALL HP, SPELL SLOTS AND HIT DICE. CLEARS TEMP HP AND DEATH SAVES. ASKS FIRST.");
        break;
    case M_EDIT:
        snprintf(buf, sizeof buf, "NAME, CLASS, SUBCLASS, LEVEL, HP, AC, SPEED, SCORES AND PROFICIENCIES. "
                                  "SHORTCUT: SELECT.");
        break;
    default:
        snprintf(buf, sizeof buf, "BACK TO THE CHARACTER LIST. %s",
                 A.temp ? "TEMPORARY SESSION: NOTHING IS SAVED." : "UNSAVED CHANGES ARE SAVED FIRST.");
        break;
    }
    int y = 142;
    wrap_lines(s, buf, 40, 8, &y, 11, C_DIM, 3);
}

static void menu_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    top_sheet(s, NULL, NULL);
}

static bool menu_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    if (in->down & (KBIT(K_B) | KBIT(K_START))) {
        ov_pop();
        return true;
    }
    return false;
}

static void ov_menu_build(Ctx *cx, Overlay *o)
{
    (void)o;
    cx->key = 3; // the menu remembers its focus across openings
    cx->focus = &A.focusMenu;
    for (int i = 0; i < M_COUNT; i++) {
        Btn *b = ctx_add(cx, BID(B_MENU, i), 8, 20 + i * 19, 240, 17);
        b->draw = menu_draw;
        b->act = menu_act;
        b->a = i;
        b->label = MENU_LABELS[i];
    }
    cx->def = BID(B_MENU, 0);
    ctx_footer(cx, "A", "CHOOSE", "B", "CLOSE", (const char *)NULL);
    cx->bot = menu_bot;
    cx->top = menu_top;
    cx->onKeys = menu_keys;
}

// ---------- stepper ----------

void open_stepper(const char *title, int field, int arg, int min, int max, int step, int big)
{
    Overlay *o = ov_push(OV_STEPPER);
    snprintf(o->step.title, sizeof o->step.title, "%s", title);
    o->step.field = field;
    o->step.arg = arg;
    o->step.min = min;
    o->step.max = max;
    o->step.step = step;
    o->step.big = big;
    Character *k = ch();
    switch (field) {
    case SF_LEVEL:
        o->step.val = k->level;
        break;
    case SF_HPMAX:
        o->step.val = k->hpMax;
        break;
    case SF_AC:
        o->step.val = k->ac;
        break;
    case SF_SPEED:
        o->step.val = k->speed;
        break;
    case SF_ABILITY:
        o->step.val = k->ab[arg];
        break;
    case SF_SLOTMAX:
        o->step.val = k->slotMax[arg];
        break;
    case SF_WHIT:
        o->step.val = k->weapons[arg].bonus;
        break;
    case SF_WDICE:
        o->step.val = k->weapons[arg].n;
        break;
    default:
        o->step.val = k->weapons[arg].dmg;
        break;
    }
}

static int stepper_get(const Overlay *o)
{
    const Character *k = ch();
    int a = o->step.arg;
    switch (o->step.field) {
    case SF_LEVEL:
        return k->level;
    case SF_HPMAX:
        return k->hpMax;
    case SF_AC:
        return k->ac;
    case SF_SPEED:
        return k->speed;
    case SF_ABILITY:
        return k->ab[a];
    case SF_SLOTMAX:
        return k->slotMax[a];
    case SF_WHIT:
        return k->weapons[a].bonus;
    case SF_WDICE:
        return k->weapons[a].n;
    default:
        return k->weapons[a].dmg;
    }
}

static void stepper_set(const Overlay *o, int v)
{
    Character *k = ch();
    int a = o->step.arg;
    switch (o->step.field) {
    case SF_LEVEL:
        act_set_level(k, v);
        break;
    case SF_HPMAX:
        act_set_hp_max(k, v);
        break;
    case SF_AC:
        k->ac = (uint8_t)v;
        break;
    case SF_SPEED:
        k->speed = (uint8_t)v;
        break;
    case SF_ABILITY:
        k->ab[a] = (uint8_t)v;
        break;
    case SF_SLOTMAX:
        act_set_slot_max(k, a, v);
        break;
    case SF_WHIT:
        k->weapons[a].bonus = (int8_t)v;
        break;
    case SF_WDICE:
        k->weapons[a].n = (uint8_t)v;
        break;
    default:
        k->weapons[a].dmg = (int8_t)v;
        break;
    }
}

static void stepper_clamp(Overlay *o, int v)
{
    o->step.val = v < o->step.min ? o->step.min : v > o->step.max ? o->step.max : v;
}

static void step_btn_act(const Btn *b)
{
    Overlay *o = ov_top();
    stepper_clamp(o, o->step.val + b->b);
}

static void step_ok(const Btn *b)
{
    (void)b;
    Overlay *o = ov_top();
    bool changed = o->step.val != stepper_get(o);
    stepper_set(o, o->step.val);
    if (changed)
        A.dirty = true;
    ov_pop();
}

static void step_cancel(const Btn *b)
{
    (void)b;
    ov_pop();
}

static void stepper_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    char buf[32];
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, o->step.title, 6, 5, C_GOLD, 1);
    snprintf(buf, sizeof buf, "%d", o->step.val);
    d_text_c(s, buf, 0, SCREEN_W, 36, C_GOLD, 4);
    snprintf(buf, sizeof buf, "RANGE %d-%d", o->step.min, o->step.max);
    d_text_c(s, buf, 0, SCREEN_W, 74, C_DIM, 1);
}

static void stepper_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    const Character *k = ch();
    char buf[64];
    int v = o->step.val;
    if (o->step.field == SF_LEVEL)
        snprintf(buf, sizeof buf, "LEVEL %d " G_ARROW " %d   PROF %s " G_ARROW " %s", k->level, v, sg(rules_pb(k)),
                 sg(2 + (v - 1) / 4));
    else if (o->step.field == SF_ABILITY)
        snprintf(buf, sizeof buf, "%s %d " G_ARROW " %d   MOD %s " G_ARROW " %s", AB_NAMES[o->step.arg],
                 k->ab[o->step.arg], v, sg(rules_mod(k->ab[o->step.arg])), sg(rules_mod(v)));
    else
        snprintf(buf, sizeof buf, "%s %d " G_ARROW " %d", o->step.title, stepper_get(o), v);
    top_sheet(s, buf, NULL);
}

static bool stepper_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    Overlay *o = ov_top();
    if (in->rep & KBIT(K_LEFT))
        stepper_clamp(o, o->step.val - o->step.step);
    else if (in->rep & KBIT(K_RIGHT))
        stepper_clamp(o, o->step.val + o->step.step);
    else if (in->rep & KBIT(K_UP))
        stepper_clamp(o, o->step.val + o->step.big);
    else if (in->rep & KBIT(K_DOWN))
        stepper_clamp(o, o->step.val - o->step.big);
    if (in->down & KBIT(K_A))
        step_ok(NULL);
    else if (in->down & KBIT(K_B))
        ov_pop();
    return true;
}

static void ov_stepper_build(Ctx *cx, Overlay *o)
{
    int d[4] = { -o->step.big, -o->step.step, o->step.step, o->step.big };
    for (int j = 0; j < 4; j++) {
        const char *label = ctx_str(cx, "%c%d", d[j] > 0 ? '+' : '-', d[j] > 0 ? d[j] : -d[j]);
        Btn *b = add_plain(cx, BID(B_STEP, j), 4 + j * 62, 96, 58, 26, label, C_PANEL2, C_TEXT, 2);
        b->nav = false;
        b->act = step_btn_act;
        b->b = d[j];
    }
    Btn *b = add_plain(cx, BID(B_STCANCEL, 0), 4, 140, 120, 26, "CANCEL", C_PANEL2, C_TEXT, 1);
    b->nav = false;
    b->act = step_cancel;
    b = add_plain(cx, BID(B_STOK, 0), 132, 140, 120, 26, "OK", C_GOLD, C_INK, 1);
    b->nav = false;
    b->act = step_ok;
    const char *k1 = ctx_str(cx, "+/-%d", o->step.step), *k2 = ctx_str(cx, "+/-%d", o->step.big);
    ctx_footer(cx, G_TRI_LEFT G_TRI_RIGHT, k1, G_TRI_UP G_TRI_DOWN, k2, "A", "OK", "B", "CANCEL", (const char *)NULL);
    cx->bot = stepper_bot;
    cx->top = stepper_top;
    cx->onKeys = stepper_keys;
}

// ---------- keyboard ----------

static const char *const KB_ROWS[5] = { "1234567890", "QWERTYUIOP", "ASDFGHJKL'", "ZXCVBNM.,/", "-&()+!?:" };

void open_kbd(const char *title, const char *field, const char *val, int max, bool allowEmpty, int target, int arg)
{
    Overlay *o = ov_push(OV_KBD);
    snprintf(o->kbd.title, sizeof o->kbd.title, "%s", title);
    snprintf(o->kbd.field, sizeof o->kbd.field, "%s", field);
    snprintf(o->kbd.val, sizeof o->kbd.val, "%s", val);
    o->kbd.max = max;
    o->kbd.allowEmpty = allowEmpty;
    o->kbd.target = target;
    o->kbd.arg = arg;
}

static void kbd_type(Overlay *o, char c)
{
    size_t n = strlen(o->kbd.val);
    o->kbd.err[0] = 0;
    if ((int)n >= o->kbd.max) {
        snprintf(o->kbd.err, sizeof o->kbd.err, "%d CHARACTERS MAX", o->kbd.max);
        return;
    }
    if (c == ' ' && (!n || o->kbd.val[n - 1] == ' '))
        return;
    o->kbd.val[n] = c;
    o->kbd.val[n + 1] = 0;
}

static void kbd_del(Overlay *o)
{
    size_t n = strlen(o->kbd.val);
    o->kbd.err[0] = 0;
    if (n)
        o->kbd.val[n - 1] = 0;
}

static void kbd_ok(Overlay *o)
{
    char v[32];
    const char *s = o->kbd.val;
    while (*s == ' ')
        s++;
    snprintf(v, sizeof v, "%s", s);
    size_t n = strlen(v);
    while (n && v[n - 1] == ' ')
        v[--n] = 0;
    if (!n && !o->kbd.allowEmpty) {
        snprintf(o->kbd.err, sizeof o->kbd.err, "%s CAN'T BE EMPTY", o->kbd.field[0] ? o->kbd.field : "TEXT");
        return;
    }
    // Names are stored with the save format's name charset; only weapon
    // properties may use , + ! ? : (the prototype let these through and the
    // save would then fail its range check on the next load).
    if (o->kbd.target != KT_WPROPS && !text_name_ok(v)) {
        snprintf(o->kbd.err, sizeof o->kbd.err, "NO , + ! ? : IN %s", o->kbd.field);
        return;
    }
    kbd_done(o, v);
}

void kbd_done(Overlay *o, const char *v)
{
    Character *k = ch();
    int arg = o->kbd.arg, target = o->kbd.target; // o is popped below
    char title[40];
    snprintf(title, sizeof title, "%s", o->kbd.title);
    switch (target) {
    case KT_NEWCHAR:
        ov_pop();
        session_new_char(arg, v);
        return;
    case KT_ADDWEAPON: {
        ov_pop();
        int idx = act_add_weapon(k, v);
        if (idx < 0)
            return;
        A.dirty = true;
        A.focusTab[TAB_ARMS][0] = BID(B_WEAPON, idx);
        open_weapon_detail(idx);
        open_weapon_editor(idx);
        return;
    }
    case KT_WNAME:
        text_copy(k->weapons[arg].name, v, WEAPON_NAME_LEN);
        break;
    case KT_WRANGE:
        text_copy(k->weapons[arg].range, v, WEAPON_RANGE_LEN);
        break;
    case KT_WPROPS:
        text_copy(k->weapons[arg].props, v, WEAPON_PROPS_LEN);
        break;
    case KT_NAME:
        text_copy(k->name, v, NAME_LEN);
        break;
    case KT_CLASS:
        text_copy(k->cls, v, CLASS_LEN);
        break;
    case KT_SUB:
        text_copy(k->sub, v, SUBCLASS_LEN);
        break;
    case KT_SPECIES:
        text_copy(k->species, v, SPECIES_LEN);
        break;
    default:
        text_copy(k->background, v, BACKGROUND_LEN);
        break;
    }
    A.dirty = true;
    ov_pop();
    if (target < KT_WNAME)
        toastf(C_BLUE, "%s UPDATED", title);
}

static void kb_key_act(const Btn *b)
{
    kbd_type(ov_top(), (char)b->a);
}

static void kb_special_act(const Btn *b)
{
    Overlay *o = ov_top();
    switch (BKIND(b->id)) {
    case B_KBCANCEL:
        ov_pop();
        break;
    case B_KBSPACE:
        kbd_type(o, ' ');
        break;
    case B_KBDEL:
        kbd_del(o);
        break;
    default:
        kbd_ok(o);
        break;
    }
}

static bool blink(void)
{
    return (A.frame >> 5) & 1;
}

static void kbd_bot(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    d_rect(s, 0, 0, SCREEN_W, 16, C_PANEL);
    d_text(s, o->kbd.title, 6, 5, C_GOLD, 1);
    d_rect(s, 3, 164, 250, 14, C_PANEL);
    d_text(s, o->kbd.val, 8, 167, C_TEXT, 1);
    if (blink())
        d_rect(s, 8 + (int)strlen(o->kbd.val) * 6, 174, 5, 1, C_GOLD);
}

static void kbd_top(Surface *s, const Ctx *cx)
{
    (void)cx;
    const Overlay *o = ov_top();
    char buf[16];
    d_rect(s, 0, 0, SCREEN_W, SCREEN_H, C_BG);
    d_text(s, o->kbd.title, 8, 24, C_GOLD, 1);
    d_rect(s, 8, 52, 240, 30, C_PANEL);
    d_box(s, 8, 52, 240, 30, C_LINE, 1);
    d_text(s, o->kbd.val, 14, 60, C_TEXT, 2);
    if (blink())
        d_rect(s, 14 + (int)strlen(o->kbd.val) * 12, 75, 10, 2, C_GOLD);
    snprintf(buf, sizeof buf, "%d/%d", (int)strlen(o->kbd.val), o->kbd.max);
    d_text_r(s, buf, 248, 88, C_DIM, 1);
    if (o->kbd.err[0])
        d_text(s, o->kbd.err, 8, 104, C_RED, 1);
    if (o->kbd.allowEmpty)
        d_text(s, "OPTIONAL: OK WITH NOTHING TYPED CLEARS IT.", 8, 120, C_DIM, 1);
    d_text(s, "TOUCH A KEY, OR MOVE WITH THE D-PAD", 8, 150, C_DIM, 1);
    d_text(s, "AND PRESS A. B DELETES. START IS DONE.", 8, 162, C_DIM, 1);
}

static bool kbd_keys(const Ctx *cx, const Input *in)
{
    (void)cx;
    Overlay *o = ov_top();
    if (in->down & KBIT(K_B)) {
        if (o->kbd.val[0])
            kbd_del(o);
        else
            ov_pop();
        return true;
    }
    if (in->down & KBIT(K_START)) {
        kbd_ok(o);
        return true;
    }
    return false;
}

static void ov_kbd_build(Ctx *cx, Overlay *o)
{
    (void)o;
    static char keyLabels[5][10][2];
    for (int r = 0; r < 5; r++) {
        for (int c = 0; KB_ROWS[r][c]; c++) {
            keyLabels[r][c][0] = KB_ROWS[r][c];
            keyLabels[r][c][1] = 0;
            Btn *b = add_plain(cx, BID(B_KB, r * 10 + c), 3 + c * 25, 18 + r * 24, 23, 22, keyLabels[r][c], C_PANEL2,
                               C_TEXT, 1);
            b->act = kb_key_act;
            b->a = KB_ROWS[r][c];
        }
    }
    Btn *b = add_plain(cx, BID(B_KBCANCEL, 0), 3, 139, 58, 22, "CANCEL", C_PANEL2, C_TEXT, 1);
    b->act = kb_special_act;
    b = add_plain(cx, BID(B_KBSPACE, 0), 63, 139, 96, 22, "SPACE", C_PANEL2, C_TEXT, 1);
    b->act = kb_special_act;
    b = add_plain(cx, BID(B_KBDEL, 0), 161, 139, 42, 22, "DEL", C_PANEL2, C_TEXT, 1);
    b->act = kb_special_act;
    b = add_plain(cx, BID(B_KBOK, 0), 205, 139, 48, 22, "OK", C_GOLD, C_INK, 1);
    b->act = kb_special_act;
    cx->def = BID(B_KB, 10); // Q
    ctx_footer(cx, "A", "TYPE", "B", "DELETE", "ST", "DONE", (const char *)NULL);
    cx->bot = kbd_bot;
    cx->top = kbd_top;
    cx->onKeys = kbd_keys;
}
