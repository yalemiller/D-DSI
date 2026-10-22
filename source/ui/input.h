// One frame of input, independent of libnds. The platform fills it from
// scanKeys()/keysDownRepeat()/touchRead(); host tests fill it from scripts.
#pragma once

#include <stdbool.h>
#include <stdint.h>

enum { K_A, K_B, K_X, K_Y, K_L, K_R, K_START, K_SELECT, K_UP, K_DOWN, K_LEFT, K_RIGHT, K_COUNT };
#define KBIT(k) (1u << (k))

#define REPEAT_DELAY 15 // frames, as keysSetRepeat(15, 4)
#define REPEAT_RATE 4

typedef enum { TOUCH_NONE, TOUCH_DOWN, TOUCH_MOVE, TOUCH_UP } TouchEvent;

typedef struct {
    uint32_t down; // pressed this frame
    uint32_t held;
    uint32_t rep;  // pressed this frame, or an auto-repeat tick while held
    TouchEvent touch;
    int tx, ty;    // touch position (for UP: the last position)
} Input;

// Key repeat for platforms without keysDownRepeat(): the prototype's scan().
typedef struct {
    uint32_t prev;
    uint16_t count[K_COUNT];
} KeyRepeat;

void input_from_held(KeyRepeat *r, uint32_t held, Input *out);
