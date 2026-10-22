// Drives the real app code on the host: an in-memory SD card, scripted keys
// and touches, and both screens rendered every frame (for UI tests and the
// screenshot runner).
#pragma once

#include <stdbool.h>

#include "memfs.h"
#include "ui/app.h"

extern MemFs drive_card;
extern Surface drive_top, drive_bot;

// Resets the app and empties the card. With card false, mounting fails (no SD card).
void drive_init(bool card);
// Restarts the app (a reboot) keeping the card's files.
void drive_restart(void);
// One frame with these keys held (KBIT mask) and an optional touch event.
void drive_frame(uint32_t held, TouchEvent touch, int x, int y);
void drive_wait(int frames);
void drive_press(int key);           // pressed for one frame, then released
void drive_hold(int key, int frames); // held, then released
void drive_tap(int x, int y);        // touch down and up on the same spot
void drive_boot(void);               // runs past the boot splash
// Both screens as RGB888, top above bottom with an 8 px gap: 256 x 392.
void drive_rgb(unsigned char *out);
#define DRIVE_W 256
#define DRIVE_H 392
