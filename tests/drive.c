#include "drive.h"

#include <string.h>

MemFs drive_card;
Surface drive_top, drive_bot;

static Storage storage;
static KeyRepeat repeat;
static uint16_t topPx[SCREEN_W * SCREEN_H];
static uint8_t botPx[SCREEN_W * SCREEN_H];

static bool mem_mount(void *ctx)
{
    return !((MemFs *)ctx)->missing;
}

void drive_init(bool card)
{
    memfs_clear(&drive_card);
    memfs_init(&drive_card, &storage.fs);
    drive_card.missing = !card;
    storage.mount = mem_mount;
    memset(&repeat, 0, sizeof repeat);
    app_init(&storage);
    surface_init16(&drive_top, topPx, app_palette());
    surface_init8(&drive_bot, botPx, app_palette());
}

void drive_restart(void)
{
    memset(&repeat, 0, sizeof repeat);
    app_init(&storage);
}

void drive_frame(uint32_t held, TouchEvent touch, int x, int y)
{
    Input in;
    memset(&in, 0, sizeof in);
    input_from_held(&repeat, held, &in);
    in.touch = touch;
    in.tx = x;
    in.ty = y;
    app_frame(&in);
    app_render(&drive_top, &drive_bot);
}

void drive_wait(int frames)
{
    for (int i = 0; i < frames; i++)
        drive_frame(0, TOUCH_NONE, 0, 0);
}

void drive_press(int key)
{
    drive_frame(KBIT(key), TOUCH_NONE, 0, 0);
    drive_frame(0, TOUCH_NONE, 0, 0);
}

void drive_hold(int key, int frames)
{
    for (int i = 0; i < frames; i++)
        drive_frame(KBIT(key), TOUCH_NONE, 0, 0);
    drive_frame(0, TOUCH_NONE, 0, 0);
}

void drive_tap(int x, int y)
{
    drive_frame(0, TOUCH_DOWN, x, y);
    drive_frame(0, TOUCH_UP, x, y);
}

void drive_boot(void)
{
    drive_wait(52);
}

void drive_rgb(unsigned char *out)
{
    const uint16_t *pal = app_palette();
    memset(out, 0, DRIVE_W * DRIVE_H * 3);
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            uint16_t t = topPx[y * SCREEN_W + x], b = pal[botPx[y * SCREEN_W + x] % C_COUNT];
            unsigned char *p = out + (y * DRIVE_W + x) * 3;
            unsigned char *q = out + ((y + SCREEN_H + 8) * DRIVE_W + x) * 3;
            for (int c = 0; c < 3; c++) {
                p[c] = (unsigned char)(((t >> (5 * c)) & 31) << 3);
                q[c] = (unsigned char)(((b >> (5 * c)) & 31) << 3);
            }
        }
    }
}
