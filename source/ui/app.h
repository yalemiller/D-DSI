// The app as the platform sees it: start it with a storage device, feed it
// one Input per 60 Hz frame, and let it draw both screens.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "core/fs.h"
#include "gfx/draw.h"
#include "input.h"

typedef struct {
    // Mounts the card and creates the app folder. False when there's no card.
    bool (*mount)(void *ctx);
    Fs fs;
} Storage;

void app_init(const Storage *storage);
// One frame: effects tick, then at most one action from touch or keys.
// Returns true when the screens need redrawing.
bool app_frame(const Input *in);
// Draws both screens into their back buffers.
void app_render(Surface *top, Surface *bot);
// The current theme's colors (for the bottom screen's palette).
const uint16_t *app_palette(void);
// Lid closed: saves right away if there are unsaved changes (true if it tried).
// Lid opened: shows the result of that save.
void app_lid(bool closed);
