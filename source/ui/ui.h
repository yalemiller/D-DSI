// Internal UI model shared by the screen files: session state, buttons and
// contexts (immediate mode: each frame the current screen builds its button
// list), the overlay stack, toasts and effects. Ported from the prototype's
// createSim(); see docs/PLAN.md for the layering.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "core/character.h"
#include "core/features.h"
#include "core/fs.h"
#include "core/portrait.h"
#include "core/save.h"
#include "core/spells.h"
#include "core/storage.h"
#include "app.h"
#include "gfx/draw.h"
#include "input.h"

#if defined(__GNUC__)
#define PRINTF_FMT(a, b) __attribute__((format(printf, a, b)))
#else
#define PRINTF_FMT(a, b)
#endif

// ---------- buttons and contexts ----------

#define MAX_BTNS 64
#define BID(kind, i) ((uint16_t)(((kind) << 10) | (i)))
#define BKIND(id) ((id) >> 10)
#define BINDEX(id) ((id) & 1023)

// Button kinds (ids are kind + index; 0 is "no button").
enum {
    B_NONE, B_SLOT, B_DEL, B_TAB, B_SKILL, B_INSP, B_KEY, B_DMG, B_HEAL, B_TEMP, B_DSS, B_DSF, B_SREST, B_LREST,
    B_WEAPON, B_WADD, B_WDEDIT, B_WDPREV, B_WDNEXT, B_WDBACK, B_WFIELD, B_WDEL, B_WDONE,
    B_SLOTCHIP, B_SETSLOTS, B_SPADD, B_SROW, B_SCRUP, B_SCRDN, B_SLOTROW, B_SSDONE,
    B_FTRACK, B_FROW, B_FADD, B_FPFILTER, B_FPROW,
    B_EDPAGE, B_EDROW, B_EDSCORE, B_EDSAVE, B_EDSKILL,
    B_NOTICE, B_MENU, B_STEP, B_STOK, B_STCANCEL, B_KB, B_KBCANCEL, B_KBSPACE, B_KBDEL, B_KBOK,
    B_SPPREP, B_SPUSE, B_SPUP, B_SPDN, B_SPPREV, B_SPNEXT, B_SPREM, B_SPBACK, B_PFILTER, B_PROW,
    B_SRN, B_SRH, B_SRAVG, B_SRCANCEL, B_SROK,
};

typedef struct Btn Btn;
typedef void (*BtnDraw)(Surface *s, const Btn *b, bool focus, bool press);
typedef void (*BtnFn)(const Btn *b);

struct Btn {
    uint16_t id;
    int16_t x, y, w, h;
    bool nav; // reachable with the D-pad (false: touch only)
    bool disabled;
    BtnDraw draw;
    BtnFn act, onY;
    int a, b; // payload
    // Style for the shared plain / icon button drawers.
    const char *label, *label2;
    uint8_t bg, fg, sc;
    int8_t icon;          // -1 none
    uint8_t ic0, ic1, ic2; // icon colors (C_NONE = default); ic1 == IC_USE_BG takes the button fill
    int (*iconFn)(void);   // animated icon (overrides icon)
    bool arrow;            // rows: a ▶ after the value
};
#define IC_USE_BG 0xFE

typedef struct Ctx Ctx;
typedef void (*CtxDraw)(Surface *s, const Ctx *cx);

typedef struct {
    const char *key, *label;
} Hint;

struct Ctx {
    uint32_t key;    // identity: a press only fires in the context it started in
    uint16_t *focus; // where this context remembers its focus
    uint16_t def;
    Btn btns[MAX_BTNS];
    int n;
    Hint footer[5];
    int nfooter;
    bool edit; // blue footer
    CtxDraw top, bot;
    bool (*onKeys)(const Ctx *cx, const Input *in);
    char arena[2048]; // strings the buttons point at (values formatted while building)
    int arenaUsed;
};

Btn *ctx_add(Ctx *cx, uint16_t id, int x, int y, int w, int h);
void ctx_footer(Ctx *cx, const char *k1, const char *l1, ...); // NULL-terminated pairs
const char *ctx_str(Ctx *cx, const char *fmt, ...) PRINTF_FMT(2, 3); // printf into the arena

uint16_t ctx_focus(const Ctx *cx); // current focus (falls back to def, then the first button)
const Btn *ctx_focused(const Ctx *cx);
const Btn *ctx_find(const Ctx *cx, uint16_t id);
uint16_t nav_move(const Ctx *cx, uint16_t id, int dir); // dir: K_UP / K_DOWN / K_LEFT / K_RIGHT
const Btn *ctx_hit(const Ctx *cx, int x, int y);

// Shared button drawers.
void draw_focus(Surface *s, int x, int y, int w, int h);
void btn_plain(Surface *s, const Btn *b, bool focus, bool press);      // label centered
void btn_icon(Surface *s, const Btn *b, bool focus, bool press);       // icon + 1-2 lines
void btn_row(Surface *s, const Btn *b, bool focus, bool press);        // label left, gold value right, optional arrow
Btn *add_plain(Ctx *cx, uint16_t id, int x, int y, int w, int h, const char *label, uint8_t bg, uint8_t fg, int sc);
void footer_draw(Surface *s, const Hint *hints, int n, bool edit);
void strip(Surface *s, const char *text, uint8_t col);

// Scrolling list (one or more columns, filled row by row): only visible cells
// become buttons; the D-pad moves through the whole list.
typedef struct {
    int kind;        // button kind of the cells
    uint16_t *focus; // the owning context's focus slot
    int *scroll;     // first visible row
    int n, cols, rows;
    int x, y, w, h;  // h = row height
    BtnDraw draw;
    BtnFn act, onY;
} List;
void list_build(Ctx *cx, List *L);
bool list_keys(const List *L, const Input *in);
void list_reset(void);

// ---------- overlays ----------

typedef enum {
    OV_NOTICE, OV_MENU, OV_STEPPER, OV_KBD, OV_SREST, OV_SPELL, OV_SPELLPICK, OV_SLOTSET, OV_FEATPICK, OV_WDETAIL,
    OV_WEAPON,
} OvType;

typedef enum {
    NA_CLOSE, NA_RETRY_SAVE, NA_TRY_MOUNT, NA_NOSAVE, NA_TEMP_SESSION, NA_DELETE_CHAR, NA_LONG_REST,
    NA_DELETE_WEAPON, NA_REMOVE_SPELL,
} NoticeAct;

enum { NB_PLAIN, NB_PRIMARY, NB_DANGER };

typedef struct {
    char label[16];
    uint8_t style, act;
} NoticeBtn;

typedef struct {
    char title[40], sub[40];
    bool err;
    char lines[4][128];
    int nlines;
    NoticeBtn btns[2];
    int nbtns;
    int def, cancel; // cancel: -1 = B does nothing (unless there's one button)
    char status[40]; // e.g. "STILL NO SD CARD."
    int arg;
    char name[24];
} Notice;

// Stepper targets.
enum { SF_LEVEL, SF_HPMAX, SF_AC, SF_SPEED, SF_ABILITY, SF_SLOTMAX, SF_WHIT, SF_WDICE, SF_WDMG };
// Keyboard targets.
enum { KT_NEWCHAR, KT_NAME, KT_CLASS, KT_SUB, KT_SPECIES, KT_BACKGROUND, KT_WNAME, KT_WRANGE, KT_WPROPS, KT_ADDWEAPON };

typedef struct {
    OvType type;
    uint16_t uid;
    uint16_t focus;
    int scroll, maxScroll; // text scrolling on the top screen
    union {
        Notice notice;
        struct {
            char title[40];
            int val, min, max, step, big, field, arg;
        } step;
        struct {
            char title[40], field[16], val[32], err[40];
            int max, target, arg;
            bool allowEmpty;
        } kbd;
        struct {
            int n, hp;
        } srest;
        struct {
            char names[MAX_SPELLS][SPELL_NAME_LEN + 1];
            int count, i;
        } spell;
        struct {
            int filter; // -1 = all
            char lastF[24];
            int listScroll;
        } pick;
        struct {
            int idx;
        } weapon;
    };
} Overlay;

#define MAX_OVERLAYS 8

// ---------- effects ----------

enum { FX_SHAKE, FX_FLASH, FX_COUNT, FX_FLOAT, FX_SPARKLE, FX_NIGHT, FX_FIRE };

typedef struct {
    uint8_t kind, col;
    int t, T, seed;
    int from, to;
    char text[12];
    int16_t rect[4];
    char sub[64];
} Fx;

#define MAX_FX 8

typedef struct {
    char text[64];
    uint8_t col;
    int t;
} Toast;

// ---------- session ----------

enum { VIEW_BOOT, VIEW_SELECT, VIEW_SHEET };
enum { TAB_SKILLS, TAB_FIGHT, TAB_ARMS, TAB_MAGIC, TAB_FEATS, TAB_EDIT, TAB_COUNT };
enum { LIST_SPELLS, LIST_FEATS, LIST_EDIT, LIST_WEAPON, LIST_PICK, LIST_FPICK, LIST_COUNT };

typedef enum { PS_UNKNOWN, PS_NONE, PS_BAD, PS_OK } PortraitState;

typedef struct {
    PortraitState state;
    BmpStatus reason;
    uint16_t px[PORTRAIT_SIZE * PORTRAIT_SIZE];
} PortraitCache;

typedef struct {
    bool active;
    int kind; // PEEK_*
    char name[24];
    int idx;
    int scroll, maxScroll;
} Peek;
enum { PEEK_SPELL, PEEK_FEAT, PEEK_WEAPON };

typedef struct {
    int state; // 0 built-in only, 1 file read
    int added, skipped;
} LibInfo;

typedef struct {
    bool down, inside;
    uint16_t id;
    uint32_t ctx;
} Press;

typedef struct {
    const Storage *storage;
    const Fs *fs;
    bool mounted;
    int theme;
    uint16_t pal[C_COUNT];

    int view;
    int t; // boot animation frames
    bool temp; // temporary (no-save) session
    Roster roster;
    int cur, sel;
    int tab, prevTab, editPage;
    bool dirty;
    char amt[4];

    SpellLib lib;
    LibInfo libInfo;
    FeatureLib flib;
    LibInfo flibInfo;

    uint16_t focusSelect, focusMenu;
    uint16_t focusTab[TAB_COUNT][3];
    int scroll[LIST_COUNT];

    Overlay ov[MAX_OVERLAYS];
    int nov;
    uint16_t uid;

    Fx fx[MAX_FX];
    int nfx;
    Toast toast;
    Peek peek;
    Press press;
    uint32_t frame;

    bool lid;
    bool lidSaved; // a save was attempted when the lid closed
    SaveResult lidSave;

    PortraitCache portraits[NUM_SLOTS];
} App;

extern App A;

static inline Character *ch(void)
{
    return &A.roster.slot[A.cur];
}

// ---------- app.c ----------
void toast(const char *text, uint8_t col);
void toastf(uint8_t col, const char *fmt, ...) PRINTF_FMT(2, 3);
void fx_add(const Fx *f);
const Fx *fx_find(int kind);
Overlay *ov_push(OvType type);
void ov_pop(void);
Overlay *ov_top(void);
void ov_clear(void);
Notice *notice_push(void);
void ctx_now(Ctx *cx);
const char *sg(int n); // "+3" in a rotating static buffer

// ---------- session.c ----------
void session_startup(void);
SaveResult session_save_all(void);
void session_save_now(void);
void session_rest_save(const char *msg);
void session_leave(void);
void session_open_slot(int i);
void session_new_char(int slot, const char *name);
void session_ask_delete(int i);
void session_delete(int i);
void session_try_mount(void);
void session_nosave_mode(void);
void session_temp_session(void);
void notice_save_failed(SaveResult r);
void session_lid(bool closed);
void portrait_draw(Surface *s, int slot, int x, int y);

// ---------- screens ----------
void skill_row_draw(Surface *s, const Btn *b, bool focus, bool press); // b->a skill, b->b edit colors
// Spell rows (MAGIC list and the picker): b->a item, b->b 1 = own list.
// Whoever builds a list points these at its items.
void spell_row_draw(Surface *s, const Btn *b, bool focus, bool press);
extern const char *(*spell_row_name)(int i);
extern bool (*spell_row_known)(int i);
void toggle_prepared(const char *name);
void fx_sparkle(int t, int x, int y, int w, int h, uint8_t col);
void view_boot(Ctx *cx);
void view_select(Ctx *cx);
void view_sheet(Ctx *cx);
void tab_skills(Ctx *cx, const Character *k);
void tab_fight(Ctx *cx, Character *k);
void tab_arms(Ctx *cx, Character *k);
void tab_magic(Ctx *cx, Character *k);
void tab_feats(Ctx *cx, Character *k);
void tab_edit(Ctx *cx, Character *k);
void ov_build(Ctx *cx, Overlay *o);
void ov_spell_build(Ctx *cx, Overlay *o);
void ov_spellpick_build(Ctx *cx, Overlay *o);
void ov_slotset_build(Ctx *cx, Overlay *o);
void ov_featpick_build(Ctx *cx, Overlay *o);
void ov_srest_build(Ctx *cx, Overlay *o);
void ov_wdetail_build(Ctx *cx, Overlay *o);
void ov_weapon_build(Ctx *cx, Overlay *o);

// Overlay openers.
void open_menu(void);
void open_stepper(const char *title, int field, int arg, int min, int max, int step, int big);
void open_kbd(const char *title, const char *field, const char *val, int max, bool allowEmpty, int target, int arg);
void open_short_rest(void);
void ask_long_rest(void);
void do_long_rest(void);
void open_spell(int i);
void open_spell_picker(void);
void open_slotset(void);
void open_feat_picker(void);
void open_weapon_detail(int idx);
void open_weapon_editor(int idx);
void add_weapon_flow(void);
void kbd_done(Overlay *o, const char *value);
void notice_act(int act, const Notice *n);

// ---------- top screen (top.c) ----------
typedef struct {
    char title[24];
    int icon;
    uint8_t col;
    char tag[24];
    uint8_t tagCol;
    char meta[96], meta2[96];
    const char *desc;
    char descBuf[4096]; // generated text (weapons: mastery + property entries)
    char hint[32];
} Summary;

void top_sheet(Surface *s, const char *banner, const Summary *sum);
void top_summary(Surface *s, const Summary *sum);
void top_page(Surface *s, const Summary *sum, int *scroll, int *maxScroll);
void top_spell(Surface *s, const char *name, const Spell *sp, int *scroll, int *maxScroll);
void top_feat(Surface *s, const char *name, const Feature *f, int *scroll, int *maxScroll);
void top_weapon(Surface *s, const Character *k, const Weapon *w, int *scroll, int *maxScroll);
void top_select(Surface *s);
void top_base(Surface *s); // what's under a dialog: the sheet or the current view
void top_fx(Surface *s);
void sparkle(Surface *s, int t, int seed, int x, int y, int w, int h, uint8_t col);
bool summary_for(const Character *k, int kind, const char *name, int idx, Summary *out);

// The 2 px stripe color of a feature category.
uint8_t feat_cat_color(int cat);
// Sorted views of a character's spells / features.
int my_spells(const Character *k, int *order);
int my_feats(const Character *k, int *order);
int spell_level_of(const char *name); // -1 when not in the library
