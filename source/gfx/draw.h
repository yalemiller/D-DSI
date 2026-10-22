// Software drawing into RAM surfaces: the top screen is 16-bit (RGB555 with
// bit 15 set, so portraits sit next to the UI), the bottom screen 8-bit
// (palette index = color role). Everything is integer and pixel-exact with
// the prototype's canvas calls; all drawing is clipped to the surface.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "palette.h"

#define SCREEN_W 256
#define SCREEN_H 192

typedef struct {
    int w, h;
    uint16_t *px16;       // 16-bit surface, or NULL
    uint8_t *px8;         // 8-bit surface, or NULL
    const uint16_t *pal;  // theme colors (RGB555) for 16-bit drawing
    int alpha;            // 0..256; below 256, 16-bit drawing blends (rest-scene fades)
} Surface;

void surface_init16(Surface *s, uint16_t *px, const uint16_t *pal);
void surface_init8(Surface *s, uint8_t *px, const uint16_t *pal);

void d_rect(Surface *s, int x, int y, int w, int h, uint8_t col);
// Outline `t` pixels thick, drawn as four rects like the prototype's BOX().
void d_box(Surface *s, int x, int y, int w, int h, uint8_t col, int t);

// Text in the 5x7 font: 6 px advance at 1x, pixel-doubled at 2x-4x.
// Lowercase draws as uppercase; characters without a glyph draw as '?'.
int d_tw(const char *str, int sc);
int d_twn(int len, int sc);
void d_text(Surface *s, const char *str, int x, int y, uint8_t col, int sc);
void d_textn(Surface *s, const char *str, int len, int x, int y, uint8_t col, int sc);
// Centered in [x, x + w) (rounding down, like Math.floor).
void d_text_c(Surface *s, const char *str, int x, int w, int y, uint8_t col, int sc);
// Right edge at r.
void d_text_r(Surface *s, const char *str, int r, int y, uint8_t col, int sc);

enum {
    IC_SWORD, IC_ARROW, IC_HEART, IC_SHIELD, IC_SKULL, IC_FIRE, IC_FIRE2, IC_MOON, IC_STAR, IC_BOLT, IC_BOOT, IC_EYE,
    IC_MPAGE, IC_MHEART, IC_MSWORD, IC_MSPARK, IC_MBOOK, IC_MQUILL, IC_MSKULL, IC_MSTAR, IC_MSHIELD, IC_COUNT
};
int icon_w(int icon);
int icon_h(int icon);
// Pixels '#', '+', '=' take c0, c1, c2; C_NONE leaves them clear.
void d_icon(Surface *s, int icon, int x, int y, uint8_t c0, uint8_t c1, uint8_t c2, int sc);

// The AC heater shield: flat top with clipped corners, straight upper third,
// then each side insets by round((half - 1) * p^1.8) down to a point.
void d_shield(Surface *s, int x, int y, int w, int h, uint8_t col);

// RGB555 image (16-bit surfaces only).
void d_image(Surface *s, const uint16_t *px, int w, int h, int x, int y);

// Dialog dimming of a 16-bit surface: the prototype's rgba(8,4,16,0.72) wash.
void d_dim(Surface *s);

// Floor division (Math.floor(a / b) for b > 0).
int floordiv(int a, int b);
