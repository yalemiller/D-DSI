// Color roles and the five themes. Every channel is a multiple of 8, so each
// color is exact in the DS's RGB555.
#pragma once

#include <stdint.h>

enum {
    C_BG,
    C_PANEL,
    C_PANEL2,
    C_LINE,
    C_TEXT,
    C_DIM,
    C_GOLD,  // focus / accent
    C_RED,   // damage / errors
    C_GREEN, // heal / saved
    C_BLUE,  // spells / edit / temp HP
    C_INK,   // text on accent fills
    C_COUNT
};

#define C_NONE 0xFF // "don't draw" in icon color lists

enum { THEME_TORCHLIGHT, THEME_GRIMOIRE, THEME_ELVEN, THEME_PARCHMENT, THEME_NIGHT, THEME_COUNT };

// The user prefers Night (handoff README), so it's the default.
#define THEME_DEFAULT THEME_NIGHT

typedef struct {
    const char *name;
    uint16_t c[C_COUNT]; // RGB555, bit 15 clear
} Theme;

extern const Theme THEMES[THEME_COUNT];
