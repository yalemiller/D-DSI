// Screenshot runner: plays scripted sessions through the real app code and
// writes both screens as PNGs (top above bottom), for comparing with the
// prototype without an emulator. Usage: shots OUTDIR
#define _POSIX_C_SOURCE 200809L // mkdir()

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "core/storage.h"
#include "drive.h"
#include "png.h"

static const char *outdir;
static int count, failures;

static void shot(const char *name)
{
    static unsigned char rgb[DRIVE_W * DRIVE_H * 3];
    char path[256];
    drive_rgb(rgb);
    snprintf(path, sizeof path, "%s/%02d-%s.png", outdir, ++count, name);
    if (!png_write(path, rgb, DRIVE_W, DRIVE_H)) {
        fprintf(stderr, "could not write %s\n", path);
        failures++;
    } else {
        printf("%s\n", path);
    }
}

static void presses(int key, int n)
{
    for (int i = 0; i < n; i++)
        drive_press(key);
}

static void session(void)
{
    drive_init(true);
    drive_wait(30);
    shot("boot");
    drive_wait(22); // first run: creates the demo characters
    shot("select-first-run");

    drive_press(K_A); // Jedediah
    shot("skills");
    presses(K_DOWN, 6); // insight
    shot("skills-insight");

    drive_press(K_R);
    shot("feats");
    drive_press(K_X);
    shot("feats-full-page");
    drive_press(K_B);
    presses(K_DOWN, 3);
    shot("feats-list");

    drive_press(K_R);
    shot("magic");
    drive_press(K_A);
    shot("spell-details");
    drive_press(K_B);
    drive_tap(229, 32); // + ADD
    shot("spell-picker");
    drive_press(K_DOWN);
    shot("spell-picker-row");
    drive_press(K_B);

    drive_press(K_R);
    shot("arms");
    drive_press(K_A);
    shot("weapon-details");
    drive_press(K_A);
    shot("weapon-editor");
    drive_press(K_B);
    drive_press(K_B);

    drive_press(K_R);
    shot("fight");
    drive_tap(17, 78);  // 7
    drive_tap(176, 56); // DAMAGE
    drive_wait(4);
    shot("damage");
    drive_wait(60);
    shot("after-damage");

    drive_press(K_START);
    shot("menu");
    presses(K_DOWN, 3);
    drive_press(K_A);
    shot("long-rest-ask");
    drive_press(K_RIGHT);
    drive_press(K_A);
    drive_wait(30);
    shot("long-rest-scene");
    drive_wait(70);
    shot("after-long-rest");

    drive_tap(137, 162); // SHORT REST
    shot("short-rest");
    drive_tap(234, 47); // + die
    drive_tap(128, 122); // USE AVERAGE
    shot("short-rest-filled");
    drive_press(K_START);
    drive_wait(30);
    shot("campfire");
    drive_wait(60);

    drive_press(K_R); // EDIT
    shot("edit-info");
    drive_press(K_X);
    shot("edit-abilities");
    drive_press(K_X);
    shot("edit-skills");
    drive_press(K_X);
    presses(K_DOWN, 5); // LEVEL
    drive_press(K_A);
    shot("stepper-level");
    drive_press(K_UP);
    shot("stepper-level-up");
    drive_press(K_B);
    presses(K_UP, 5); // NAME
    drive_press(K_A);
    shot("keyboard");
    drive_press(K_START);

    drive_press(K_START);
    presses(K_DOWN, 5);
    drive_press(K_A); // CHARACTERS: saves first
    shot("select-saved");
    drive_press(K_DOWN);
    drive_press(K_A); // Maren
    shot("maren");
    presses(K_R, 2);
    shot("maren-magic");
    presses(K_R, 2); // MAGIC -> ARMS -> FIGHT
    drive_tap(17, 78); // 7
    drive_tap(17, 78); // 77
    drive_tap(176, 56);
    drive_wait(60);
    shot("maren-down");
}

static void storage_states(void)
{
    drive_init(false);
    drive_boot();
    shot("no-sd-card");

    // two saves, then damage sheet.sav: the backup is restored
    drive_init(true);
    drive_boot();
    drive_press(K_A);
    drive_press(K_START);
    drive_press(K_DOWN);
    drive_press(K_A); // SAVE NOW: sheet.sav -> sheet.bak
    MemFile *sav = memfs_get(&drive_card, FILE_SAV);
    if (sav)
        sav->data[SAVE_HEADER + 3] ^= 0x5A;
    drive_restart();
    drive_boot();
    shot("backup-restored");

    MemFile *bak = memfs_get(&drive_card, FILE_BAK);
    if (bak)
        bak->data[0] = 'X';
    memfs_put(&drive_card, FILE_SAV, "junk", 4);
    drive_restart();
    drive_boot();
    shot("both-damaged");

    drive_init(true);
    drive_boot();
    memfs_put(&drive_card, "portrait1.bmp", "BM not really", 13);
    drive_restart();
    drive_boot();
    shot("bad-portrait");
}

// Static screens for pixel comparison with the browser prototype (Night theme):
// every step waits 120 frames so toasts and effects have ended.
static void compare(void)
{
    drive_init(true);
    drive_boot();
    drive_wait(120);
    shot("cmp-select");
    static const struct {
        int key;
        const char *name;
    } STEPS[] = {
        { K_A, "cmp-skills" },        { K_R, "cmp-feats" },          { K_R, "cmp-magic" },
        { K_R, "cmp-arms" },          { K_R, "cmp-fight" },          { K_R, "cmp-edit-info" },
        { K_X, "cmp-edit-abilities" }, { K_X, "cmp-edit-skills" },   { K_START, "cmp-menu" },
    };
    for (size_t i = 0; i < sizeof STEPS / sizeof STEPS[0]; i++) {
        drive_press(STEPS[i].key);
        drive_wait(120);
        shot(STEPS[i].name);
    }
}

int main(int argc, char **argv)
{
    outdir = argc > 1 ? argv[1] : "shots";
    mkdir(outdir, 0777);
    session();
    storage_states();
    compare();
    printf("%d screenshots, %d failed\n", count, failures);
    return failures ? 1 : 0;
}
