// UI behavior through the real app code: input rules from the handoff's test
// plan (repeat, touch fires once on release inside, a dialog cancels a press),
// saving on leave and lid close, temp sessions never saving, startup states.
#include "core/storage.h"
#include "drive.h"
#include "test.h"
#include "tests.h"
#include "ui/ui.h"

static void touch(TouchEvent t, int x, int y)
{
    drive_frame(0, t, x, y);
}

static void test_repeat(void)
{
    KeyRepeat r;
    Input in;
    memset(&r, 0, sizeof r);
    int reps = 0, downs = 0, firstRepeat = -1;
    for (int f = 0; f < 31; f++) {
        input_from_held(&r, KBIT(K_UP) | KBIT(K_A), &in);
        if (in.rep & KBIT(K_UP)) {
            reps++;
            if (f > 0 && firstRepeat < 0)
                firstRepeat = f;
        }
        if (in.down & KBIT(K_A))
            downs++;
    }
    CHECK_INT(reps, 5);         // frames 0, 15, 19, 23, 27
    CHECK_INT(firstRepeat, 15); // waits 15 frames, then every 4
    CHECK_INT(downs, 1);        // a held A fires once
}

static void go_fight(void)
{
    drive_tap(210, 8); // FIGHT tab
}

static void test_first_run_and_touch(void)
{
    drive_init(true);
    drive_boot();
    CHECK_INT(A.view, VIEW_SELECT);
    CHECK(memfs_get(&drive_card, FILE_SAV) != NULL);
    CHECK(memfs_get(&drive_card, FILE_SPELLS) != NULL);
    CHECK(memfs_get(&drive_card, FILE_FEATURES) != NULL);
    CHECK(memfs_get(&drive_card, "portrait1.bmp") != NULL);
    CHECK(A.roster.used[0] && A.roster.used[1] && !A.roster.used[2]);
    CHECK(!A.dirty);
    CHECK(spells_find(&A.lib, "SILVERED SHELL") != NULL); // from spells.txt
    CHECK(features_find(&A.flib, "BARREL") != NULL);      // from features.txt
    CHECK_STR(A.toast.text, "CREATED /DATA/POCKETSHEET/");

    drive_press(K_A);
    CHECK_INT(A.view, VIEW_SHEET);
    CHECK_STR(ch()->name, "JEDEDIAH GRAVES");
    go_fight();
    CHECK_INT(A.tab, TAB_FIGHT);

    drive_tap(17, 78); // 7
    CHECK_STR(A.amt, "7");
    // press, drag off, release: nothing
    touch(TOUCH_DOWN, 48, 78);
    touch(TOUCH_MOVE, 200, 170);
    touch(TOUCH_UP, 200, 170);
    CHECK_STR(A.amt, "7");
    // drag back on and release: one action
    touch(TOUCH_DOWN, 48, 78);
    touch(TOUCH_MOVE, 200, 170);
    touch(TOUCH_MOVE, 48, 78);
    touch(TOUCH_UP, 48, 78);
    CHECK_STR(A.amt, "78");
    // a dialog opening mid-press cancels the press
    touch(TOUCH_DOWN, 79, 78);
    drive_frame(KBIT(K_START), TOUCH_MOVE, 79, 78);
    CHECK(ov_top() && ov_top()->type == OV_MENU);
    touch(TOUCH_UP, 79, 78);
    CHECK_STR(A.amt, "78");
    drive_press(K_B);
    CHECK(ov_top() == NULL);

    drive_tap(176, 56); // DAMAGE 78
    CHECK_INT(ch()->hp, 0);
    CHECK(A.dirty);
    CHECK_STR(A.toast.text, "JEDEDIAH IS DOWN!");
    CHECK_STR(A.amt, "");
    ch()->ds = 1;
    ch()->df = 2;
    drive_tap(48, 102);  // 5
    drive_tap(176, 82);  // HEAL
    CHECK_INT(ch()->hp, 5);
    CHECK_INT(ch()->ds + ch()->df, 0);

    // leaving the sheet saves unsaved changes
    int writes = drive_card.writes;
    drive_press(K_START);
    for (int i = 0; i < 5; i++)
        drive_press(K_DOWN);
    drive_press(K_A); // CHARACTERS
    CHECK_INT(A.view, VIEW_SELECT);
    CHECK_INT(drive_card.writes, writes + 1);
    CHECK(!A.dirty);
    drive_restart();
    drive_boot();
    CHECK_INT(A.roster.slot[0].hp, 5); // survived the reboot
}

static void test_temp_session(void)
{
    drive_init(false);
    drive_boot();
    CHECK(ov_top() && ov_top()->type == OV_NOTICE);
    CHECK_STR(ov_top()->notice.title, "NO SD CARD FOUND");
    drive_press(K_B); // no cancel button: B does nothing
    CHECK(ov_top() != NULL);
    drive_tap(189, 149); // NO-SAVE MODE
    CHECK(ov_top() == NULL);
    CHECK(A.temp);
    CHECK(A.roster.used[0]);
    drive_press(K_A);
    go_fight();
    drive_tap(17, 78);
    drive_tap(176, 56);
    CHECK(A.dirty);
    drive_press(K_START);
    drive_press(K_DOWN);
    drive_press(K_A); // SAVE NOW
    CHECK(ov_top() && !strcmp(ov_top()->notice.title, "SAVING IS OFF"));
    drive_press(K_B);
    drive_press(K_START);
    for (int i = 0; i < 5; i++)
        drive_press(K_DOWN);
    drive_press(K_A); // CHARACTERS
    CHECK_INT(drive_card.writes, 0);
    CHECK_INT(A.view, VIEW_SELECT);

    // TRY AGAIN once the card is in
    drive_init(false);
    drive_boot();
    drive_tap(67, 149);
    CHECK(ov_top() && !strcmp(ov_top()->notice.status, "STILL NO SD CARD."));
    drive_card.missing = false;
    drive_tap(67, 149);
    CHECK(A.mounted);
    CHECK(ov_top() == NULL);
    CHECK(memfs_get(&drive_card, FILE_SAV) != NULL);
}

static void test_keyboard_and_lid(void)
{
    drive_init(true);
    drive_boot();
    drive_press(K_DOWN);
    drive_press(K_DOWN);
    CHECK_INT(A.sel, 2);
    drive_press(K_A); // empty slot: name keyboard
    CHECK(ov_top() && ov_top()->type == OV_KBD);
    drive_tap(14, 77);  // A
    drive_tap(214, 101); // ,
    CHECK_STR(ov_top()->kbd.val, "A,");
    drive_press(K_START);
    CHECK(ov_top() && ov_top()->type == OV_KBD); // refused: names can't have commas
    CHECK(ov_top()->kbd.err[0] != 0);
    CHECK(!A.roster.used[2]);
    drive_tap(182, 150); // DEL
    drive_press(K_START);
    CHECK(ov_top() == NULL);
    CHECK(A.roster.used[2]);
    CHECK_STR(A.roster.slot[2].name, "A");
    CHECK_INT(A.tab, TAB_EDIT);
    CHECK(A.dirty);

    int writes = drive_card.writes;
    app_lid(true);
    CHECK_INT(drive_card.writes, writes + 1);
    CHECK(!A.dirty);
    Input none;
    memset(&none, 0, sizeof none);
    CHECK(!app_frame(&none)); // asleep: nothing to draw
    app_lid(false);
    CHECK_STR(A.toast.text, "SAVED BEFORE SLEEP " G_CHECK);
}

static void test_spells_and_rest(void)
{
    drive_init(true);
    drive_boot();
    drive_press(K_A);
    drive_press(K_R);
    drive_press(K_R); // MAGIC
    CHECK_INT(A.tab, TAB_MAGIC);
    int prep = character_find_spell(ch(), "ENSNARING STRIKE");
    CHECK(prep >= 0 && ch()->spells[prep].prep);
    drive_press(K_Y); // first row: ENSNARING STRIKE
    CHECK(!ch()->spells[prep].prep);
    drive_tap(229, 32); // + ADD
    CHECK(ov_top() && ov_top()->type == OV_SPELLPICK);
    drive_press(K_A); // first library spell: ELDRITCH BLAST
    CHECK(character_find_spell(ch(), "ELDRITCH BLAST") >= 0);
    drive_press(K_A);
    CHECK(character_find_spell(ch(), "ELDRITCH BLAST") < 0);
    drive_press(K_B);

    CHECK_INT(ch()->insp, 0);
    drive_press(K_START);
    for (int i = 0; i < 3; i++)
        drive_press(K_DOWN);
    drive_press(K_A); // LONG REST: asks first
    CHECK(ov_top() && ov_top()->type == OV_NOTICE);
    drive_press(K_RIGHT);
    drive_press(K_A); // REST
    CHECK(ov_top() == NULL);
    CHECK_INT(ch()->insp, 1); // RESOURCEFUL
    CHECK(!A.dirty);          // rests save
}

void test_ui(void)
{
    test_repeat();
    test_first_run_and_touch();
    test_temp_session();
    test_keyboard_and_lid();
    test_spells_and_rest();
}
