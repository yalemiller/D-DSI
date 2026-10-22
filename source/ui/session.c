// Session flows around storage: startup and the load cascade, library files,
// saving with honest error reporting, opening/leaving/creating/deleting
// characters, portraits, and the lid. All file access goes through core/storage.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/demo.h"
#include "core/text.h"
#include "ui.h"

#define SAVE_TEMP 100 // pseudo result: saving is off in a temporary session

static const char *portrait_file(int slot, char *buf, size_t cap)
{
    snprintf(buf, cap, "portrait%d.bmp", slot + 1);
    return buf;
}

static void portraits_forget(void)
{
    for (int i = 0; i < NUM_SLOTS; i++)
        A.portraits[i].state = PS_UNKNOWN;
}

static PortraitCache *portrait_get(int slot)
{
    PortraitCache *p = &A.portraits[slot];
    if (p->state != PS_UNKNOWN)
        return p;
    p->state = PS_NONE;
    if (!A.mounted)
        return p;
    char name[32];
    size_t len;
    uint8_t *data = storage_read(A.fs, portrait_file(slot, name, sizeof name), &len);
    if (!data)
        return p;
    p->reason = portrait_decode(data, len, p->px);
    p->state = p->reason == BMP_OK ? PS_OK : PS_BAD;
    free(data);
    return p;
}

void portrait_draw(Surface *s, int slot, int x, int y)
{
    const PortraitCache *p = portrait_get(slot);
    if (p->state == PS_OK) {
        d_image(s, p->px, PORTRAIT_SIZE, PORTRAIT_SIZE, x, y);
        d_box(s, x - 1, y - 1, PORTRAIT_SIZE + 2, PORTRAIT_SIZE + 2, C_LINE, 1);
        return;
    }
    d_rect(s, x, y, PORTRAIT_SIZE, PORTRAIT_SIZE, C_PANEL2);
    d_box(s, x - 1, y - 1, PORTRAIT_SIZE + 2, PORTRAIT_SIZE + 2, p->state == PS_BAD ? C_RED : C_LINE, 1);
    if (p->state == PS_BAD) {
        d_text_c(s, "BAD", x, PORTRAIT_SIZE, y + 14, C_RED, 1);
        d_text_c(s, "FILE", x, PORTRAIT_SIZE, y + 26, C_RED, 1);
        return;
    }
    // Initials of the first two words, or '?' for an empty slot.
    char ini[3] = { 0 };
    int n = 0;
    if (A.roster.used[slot]) {
        const char *nm = A.roster.slot[slot].name;
        for (int i = 0; nm[i] && n < 2; i++)
            if (nm[i] != ' ' && (i == 0 || nm[i - 1] == ' '))
                ini[n++] = nm[i];
    } else {
        ini[n++] = '?';
    }
    d_text_c(s, ini, x, PORTRAIT_SIZE, y + 17, C_DIM, 2);
}

// ---------- libraries ----------

static void write_template(const char *name, const char *a, const char *b)
{
    size_t la = strlen(a), lb = strlen(b);
    char *buf = malloc(la + lb);
    if (!buf)
        return;
    memcpy(buf, a, la);
    memcpy(buf + la, b, lb);
    A.fs->write(A.fs->ctx, name, (const uint8_t *)buf, la + lb);
    free(buf);
}

static uint8_t *read_or_create(const char *name, const char *tmpl, const char *demo, size_t *len)
{
    uint8_t *data = storage_read(A.fs, name, len);
    if (!data) {
        write_template(name, tmpl, demo);
        data = storage_read(A.fs, name, len);
    }
    return data;
}

static void load_library(void)
{
    spells_init(&A.lib);
    features_init(&A.flib);
    memset(&A.libInfo, 0, sizeof A.libInfo);
    memset(&A.flibInfo, 0, sizeof A.flibInfo);
    if (!A.mounted) {
        // No card: Jedediah's homebrew still loads from the built-in copy.
        spells_parse(&A.lib, DEMO_SPELLS_JED, strlen(DEMO_SPELLS_JED));
        features_parse(&A.flib, DEMO_FEATURES_JED, strlen(DEMO_FEATURES_JED));
        return;
    }
    size_t len;
    int skipped = 0;
    uint8_t *data = read_or_create(FILE_SPELLS, DEMO_SPELLS_TEMPLATE, DEMO_SPELLS_JED, &len);
    if (data) {
        LibParseResult r = spells_parse(&A.lib, (const char *)data, len);
        A.libInfo = (LibInfo){ 1, r.added, r.skipped };
        skipped += r.skipped;
        free(data);
    }
    data = read_or_create(FILE_FEATURES, DEMO_FEATURES_TEMPLATE, DEMO_FEATURES_JED, &len);
    if (data) {
        LibParseResult r = features_parse(&A.flib, (const char *)data, len);
        A.flibInfo = (LibInfo){ 1, r.added, r.skipped };
        skipped += r.skipped;
        free(data);
    }
    if (skipped)
        toastf(C_GOLD, "LIBRARY FILES: %d ENTR%s SKIPPED", skipped, skipped > 1 ? "IES" : "Y");
}

// ---------- saving ----------

SaveResult session_save_all(void)
{
    if (A.temp)
        return (SaveResult)SAVE_TEMP;
    SaveResult r = storage_save(A.fs, &A.roster, (uint8_t)A.cur);
    if (r == SAVE_DONE)
        A.dirty = false;
    return r;
}

static void fill_save_failed(Notice *n, SaveResult r)
{
    if ((int)r == SAVE_TEMP) {
        snprintf(n->title, sizeof n->title, "SAVING IS OFF");
        snprintf(n->sub, sizeof n->sub, "TEMPORARY SESSION");
        snprintf(n->lines[0], sizeof n->lines[0], "THIS IS A TEMPORARY SESSION. CHANGES LAST UNTIL YOU POWER OFF.");
        snprintf(n->lines[1], sizeof n->lines[1], "RESTART WITH A WORKING SD CARD TO SAVE.");
        n->nlines = 2;
        n->btns[0] = (NoticeBtn){ "OK", NB_PLAIN, NA_CLOSE };
        n->nbtns = 1;
        return;
    }
    snprintf(n->title, sizeof n->title, "SAVE FAILED");
    n->err = true;
    snprintf(n->sub, sizeof n->sub, "NOTHING WAS OVERWRITTEN");
    snprintf(n->lines[0], sizeof n->lines[0], "THE SD CARD REFUSED THE WRITE (%s).", storage_save_code(r));
    snprintf(n->lines[1], sizeof n->lines[1], "YOUR LAST GOOD SAVE WAS NOT CHANGED.");
    snprintf(n->lines[2], sizeof n->lines[2],
             "YOUR EDITS ARE STILL HERE IN MEMORY. YOU CAN KEEP GOING AND RETRY LATER.");
    n->nlines = 3;
    n->btns[0] = (NoticeBtn){ "RETRY", NB_PRIMARY, NA_RETRY_SAVE };
    n->btns[1] = (NoticeBtn){ "OK", NB_PLAIN, NA_CLOSE };
    n->nbtns = 2;
    n->cancel = 1;
}

void notice_save_failed(SaveResult r)
{
    fill_save_failed(notice_push(), r);
}

void session_save_now(void)
{
    SaveResult r = session_save_all();
    if (r == SAVE_DONE)
        toast("SAVED TO SD CARD " G_CHECK, C_GREEN);
    else
        notice_save_failed(r);
}

void session_rest_save(const char *msg)
{
    if (A.temp) {
        toast(msg, C_GOLD);
        return;
    }
    SaveResult r = session_save_all();
    if (r == SAVE_DONE)
        toastf(C_GREEN, "%s " G_CHECK, msg);
    else
        notice_save_failed(r);
}

// ---------- loading ----------

static void load_all(void)
{
    LoadResult lr = storage_load(A.fs, &A.roster);
    A.cur = lr.last;
    if (lr.kind == LOAD_FIRST_RUN) {
        demo_roster(&A.roster);
        A.cur = 0;
        A.dirty = true;
        SaveResult r = session_save_all();
        if (r == SAVE_DONE) {
            if (!A.fs->exists(A.fs->ctx, "portrait1.bmp")) {
                A.fs->write(A.fs->ctx, "portrait1.bmp", DEMO_PORTRAIT1_BMP, sizeof DEMO_PORTRAIT1_BMP);
                portraits_forget();
            }
            toast("CREATED /DATA/POCKETSHEET/", C_GREEN);
        } else {
            notice_save_failed(r);
        }
    } else if (lr.kind == LOAD_MIGRATED) {
        A.dirty = true;
        toast("OLDER SAVE UPGRADED. SAVE TO KEEP IT.", C_BLUE);
    } else if (lr.kind == LOAD_BACKUP_RESTORED) {
        A.dirty = true;
        Notice *n = notice_push();
        snprintf(n->title, sizeof n->title, "BACKUP RESTORED");
        snprintf(n->sub, sizeof n->sub, "SAVE FILE WAS DAMAGED");
        if (lr.savReason == PARSE_MISSING)
            snprintf(n->lines[n->nlines++], sizeof n->lines[0], "SHEET.SAV WAS MISSING.");
        else
            snprintf(n->lines[n->nlines++], sizeof n->lines[0], "SHEET.SAV FAILED ITS CHECK (%s).",
                     save_reason(lr.savReason));
        snprintf(n->lines[n->nlines++], sizeof n->lines[0],
                 "LOADED SHEET.BAK, THE SAVE BEFORE IT. YOUR MOST RECENT CHANGES MAY BE MISSING.");
        if (lr.savExisted)
            snprintf(n->lines[n->nlines++], sizeof n->lines[0], "THE DAMAGED FILE WAS KEPT AS SHEET.BAD.");
        n->btns[0] = (NoticeBtn){ "OK", NB_PRIMARY, NA_CLOSE };
        n->nbtns = 1;
    } else if (lr.kind == LOAD_BOTH_BAD) {
        A.cur = 0;
        Notice *n = notice_push();
        snprintf(n->title, sizeof n->title, "CAN'T READ SAVE");
        n->err = true;
        snprintf(n->sub, sizeof n->sub, "SAVE AND BACKUP DAMAGED");
        snprintf(n->lines[n->nlines++], sizeof n->lines[0], "SHEET.SAV: %s. SHEET.BAK: %s.",
                 save_reason(lr.savReason), save_reason(lr.bakReason));
        if (lr.savExisted)
            snprintf(n->lines[n->nlines++], sizeof n->lines[0], "THE DAMAGED FILE WAS KEPT AS SHEET.BAD.");
        snprintf(n->lines[n->nlines++], sizeof n->lines[0],
                 "START FRESH WRITES A NEW SAVE. TEMP SESSION SAVES NOTHING.");
        n->btns[0] = (NoticeBtn){ "START FRESH", NB_PRIMARY, NA_CLOSE };
        n->btns[1] = (NoticeBtn){ "TEMP SESSION", NB_PLAIN, NA_TEMP_SESSION };
        n->nbtns = 2;
    }
}

void session_startup(void)
{
    A.view = VIEW_SELECT;
    A.mounted = A.storage->mount(A.fs->ctx);
    if (!A.mounted) {
        load_library();
        Notice *n = notice_push();
        snprintf(n->title, sizeof n->title, "NO SD CARD FOUND");
        n->err = true;
        snprintf(n->sub, sizeof n->sub, "FAT/DLDI STARTUP FAILED");
        snprintf(n->lines[0], sizeof n->lines[0],
                 "THE APP CAN'T OPEN THE FLASHCART'S MICROSD, SO IT CAN'T LOAD OR SAVE CHARACTERS.");
        snprintf(n->lines[1], sizeof n->lines[1],
                 "CHECK THE CARD IS SEATED AND THAT YOU STARTED THE APP FROM THE CART MENU.");
        n->nlines = 2;
        n->btns[0] = (NoticeBtn){ "TRY AGAIN", NB_PRIMARY, NA_TRY_MOUNT };
        n->btns[1] = (NoticeBtn){ "NO-SAVE MODE", NB_PLAIN, NA_NOSAVE };
        n->nbtns = 2;
        return;
    }
    load_library();
    load_all();
    A.focusSelect = BID(B_SLOT, A.cur);
    A.sel = A.cur;
}

void session_try_mount(void)
{
    Overlay *o = ov_top();
    A.mounted = A.storage->mount(A.fs->ctx);
    if (!A.mounted) {
        if (o && o->type == OV_NOTICE)
            snprintf(o->notice.status, sizeof o->notice.status, "STILL NO SD CARD.");
        return;
    }
    ov_pop();
    portraits_forget();
    load_library();
    load_all();
    A.focusSelect = BID(B_SLOT, A.cur);
    A.sel = A.cur;
}

void session_nosave_mode(void)
{
    A.temp = true;
    demo_roster(&A.roster);
    ov_pop();
    toast("TEMPORARY SESSION", C_GOLD);
}

void session_temp_session(void)
{
    A.temp = true;
    demo_roster(&A.roster);
    ov_pop();
}

// ---------- characters ----------

void session_open_slot(int i)
{
    A.cur = A.sel = i;
    A.tab = A.prevTab = TAB_SKILLS;
    A.editPage = 0;
    A.nfx = 0;
    A.peek.active = false;
    memset(A.scroll, 0, sizeof A.scroll);
    A.amt[0] = 0;
    A.view = VIEW_SHEET;
    memset(A.focusTab, 0, sizeof A.focusTab);
    A.focusMenu = 0;
    A.focusSelect = BID(B_SLOT, i);
}

void session_leave(void)
{
    bool tried = false;
    SaveResult r = SAVE_DONE;
    if (A.dirty && !A.temp) {
        r = session_save_all();
        tried = true;
    }
    A.view = VIEW_SELECT;
    A.focusSelect = BID(B_SLOT, A.cur);
    A.sel = A.cur;
    if (tried) {
        if (r == SAVE_DONE)
            toast("SAVED TO SD CARD " G_CHECK, C_GREEN);
        else
            notice_save_failed(r);
    }
}

void session_new_char(int slot, const char *name)
{
    character_init(&A.roster.slot[slot], name);
    A.roster.used[slot] = true;
    session_open_slot(slot);
    A.tab = TAB_EDIT;
    A.dirty = true;
    toast("NEW CHARACTER: FILL IN YOUR SHEET", C_BLUE);
}

void session_ask_delete(int i)
{
    if (!A.roster.used[i])
        return;
    Notice *n = notice_push();
    snprintf(n->title, sizeof n->title, "DELETE %s?", A.roster.slot[i].name);
    n->err = true;
    snprintf(n->sub, sizeof n->sub, "THIS CAN'T BE UNDONE");
    snprintf(n->lines[0], sizeof n->lines[0],
             "THIS REMOVES THE CHARACTER FROM THE SAVE FILE. ITS PORTRAIT FILE STAYS ON THE SD CARD.");
    snprintf(n->lines[1], sizeof n->lines[1], "THE PREVIOUS SAVE STAYS IN SHEET.BAK UNTIL THE NEXT SAVE.");
    n->nlines = 2;
    n->btns[0] = (NoticeBtn){ "KEEP", NB_PLAIN, NA_CLOSE };
    n->btns[1] = (NoticeBtn){ "DELETE", NB_DANGER, NA_DELETE_CHAR };
    n->nbtns = 2;
    n->def = 0;
    n->cancel = 0;
    n->arg = i;
}

void session_delete(int i)
{
    A.roster.used[i] = false;
    memset(&A.roster.slot[i], 0, sizeof A.roster.slot[i]);
    A.dirty = true;
    ov_pop();
    if (!A.temp)
        session_save_now();
    else
        toast("DELETED", C_GOLD);
}

// ---------- lid ----------

void session_lid(bool closed)
{
    if (closed == A.lid)
        return;
    if (closed) {
        A.lid = true;
        memset(&A.press, 0, sizeof A.press);
        A.lidSaved = A.dirty && !A.temp && A.view != VIEW_BOOT;
        if (A.lidSaved)
            A.lidSave = session_save_all();
        return;
    }
    A.lid = false;
    if (!A.lidSaved)
        return;
    A.lidSaved = false;
    if (A.lidSave == SAVE_DONE) {
        toast("SAVED BEFORE SLEEP " G_CHECK, C_GREEN);
    } else {
        Notice *n = notice_push();
        fill_save_failed(n, A.lidSave);
        snprintf(n->title, sizeof n->title, "SAVE BEFORE SLEEP FAILED");
    }
}
