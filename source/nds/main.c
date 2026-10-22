// DS-mode entry point: video setup, the 60 Hz loop and the lid.
//
// Top screen: main engine, mode 5, BG3 16-bit bitmap in VRAM A.
// Bottom screen: sub engine, mode 5, BG3 8-bit bitmap in VRAM C.
// The app draws into RAM back buffers; they're copied to VRAM by DMA at the
// start of the next VBlank, only after a frame that changed something.
#include <nds.h>

#include "fs_fat.h"
#include "ui/app.h"

static uint16_t topBuf[SCREEN_W * SCREEN_H] __attribute__((aligned(32)));
static uint8_t botBuf[SCREEN_W * SCREEN_H] __attribute__((aligned(32)));

static uint32_t map_keys(uint32_t k)
{
    static const struct {
        uint32_t nds;
        int key;
    } MAP[] = {
        { KEY_A, K_A },         { KEY_B, K_B },         { KEY_X, K_X },       { KEY_Y, K_Y },
        { KEY_L, K_L },         { KEY_R, K_R },         { KEY_START, K_START }, { KEY_SELECT, K_SELECT },
        { KEY_UP, K_UP },       { KEY_DOWN, K_DOWN },   { KEY_LEFT, K_LEFT }, { KEY_RIGHT, K_RIGHT },
    };
    uint32_t out = 0;
    for (unsigned i = 0; i < sizeof MAP / sizeof MAP[0]; i++)
        if (k & MAP[i].nds)
            out |= KBIT(MAP[i].key);
    return out;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    defaultExceptionHandler();

    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    int bgTop = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    int bgBot = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    uint16_t *vramTop = bgGetGfxPtr(bgTop);
    uint16_t *vramBot = bgGetGfxPtr(bgBot);

    keysSetRepeat(REPEAT_DELAY, REPEAT_RATE);
    // The default ARM7 sleeps by itself a few frames after the lid closes. Keep
    // that off while the app is open, so a save started on lid close always
    // finishes first; sleep is re-enabled right after (see below).
    disableSleep();

    static Storage storage;
    fs_fat_storage(&storage);
    app_init(&storage);

    Surface top, bot;
    surface_init16(&top, topBuf, app_palette());
    surface_init8(&bot, botBuf, app_palette());

    bool pending = false, touching = false, lidClosed = false;
    int lastX = 0, lastY = 0;
    for (;;) {
        swiWaitForVBlank();
        if (pending) {
            const uint16_t *pal = app_palette();
            for (int i = 0; i < C_COUNT; i++)
                BG_PALETTE_SUB[i] = pal[i];
            dmaCopy(topBuf, vramTop, sizeof topBuf);
            dmaCopy(botBuf, vramBot, sizeof botBuf);
            pending = false;
        }

        scanKeys();
        uint32_t held = keysHeld();
        if ((held & KEY_LID) && !lidClosed) {
            lidClosed = true;
            touching = false;
            app_lid(true); // saves now if there are unsaved changes
            enableSleep(); // the ARM7 puts the console to sleep while the lid stays shut
            continue;
        }
        if (lidClosed) {
            if (held & KEY_LID)
                continue;
            lidClosed = false;
            disableSleep();
            app_lid(false); // shows how the save before sleep went
        }

        Input in = { 0 };
        in.down = map_keys(keysDown());
        in.held = map_keys(held);
        in.rep = map_keys(keysDownRepeat());
        if (held & KEY_TOUCH) {
            touchPosition t;
            touchRead(&t);
            in.touch = touching ? TOUCH_MOVE : TOUCH_DOWN;
            in.tx = lastX = t.px;
            in.ty = lastY = t.py;
            touching = true;
        } else if (touching) {
            in.touch = TOUCH_UP;
            in.tx = lastX;
            in.ty = lastY;
            touching = false;
        }

        if (app_frame(&in)) {
            app_render(&top, &bot);
            DC_FlushRange(topBuf, sizeof topBuf);
            DC_FlushRange(botBuf, sizeof botBuf);
            pending = true;
        }
    }
}
