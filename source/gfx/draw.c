#include "draw.h"

#include <string.h>

#include "font5x7.h"
#include "icons_data.h"

void surface_init16(Surface *s, uint16_t *px, const uint16_t *pal)
{
    memset(s, 0, sizeof *s);
    s->w = SCREEN_W;
    s->h = SCREEN_H;
    s->px16 = px;
    s->pal = pal;
    s->alpha = 256;
}

void surface_init8(Surface *s, uint8_t *px, const uint16_t *pal)
{
    memset(s, 0, sizeof *s);
    s->w = SCREEN_W;
    s->h = SCREEN_H;
    s->px8 = px;
    s->pal = pal;
    s->alpha = 256;
}

int floordiv(int a, int b)
{
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

static uint16_t blend555(uint16_t dst, uint16_t src, int a)
{
    int r = ((src & 31) * a + (dst & 31) * (256 - a)) >> 8;
    int g = (((src >> 5) & 31) * a + ((dst >> 5) & 31) * (256 - a)) >> 8;
    int b = (((src >> 10) & 31) * a + ((dst >> 10) & 31) * (256 - a)) >> 8;
    return (uint16_t)(r | (g << 5) | (b << 10));
}

static void fill_span(Surface *s, int x0, int x1, int y, uint8_t col)
{
    if (s->px8) {
        memset(s->px8 + y * s->w + x0, col, (size_t)(x1 - x0));
        return;
    }
    uint16_t c = s->pal[col];
    uint16_t *p = s->px16 + y * s->w;
    if (s->alpha >= 256) {
        for (int x = x0; x < x1; x++)
            p[x] = (uint16_t)(c | 0x8000);
    } else if (s->alpha > 0) {
        for (int x = x0; x < x1; x++)
            p[x] = (uint16_t)(blend555(p[x], c, s->alpha) | 0x8000);
    }
}

void d_rect(Surface *s, int x, int y, int w, int h, uint8_t col)
{
    if (w < 0) {
        x += w;
        w = -w;
    }
    if (h < 0) {
        y += h;
        h = -h;
    }
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > s->w ? s->w : x + w, y1 = y + h > s->h ? s->h : y + h;
    if (x0 >= x1 || y0 >= y1)
        return;
    for (int yy = y0; yy < y1; yy++)
        fill_span(s, x0, x1, yy, col);
}

void d_box(Surface *s, int x, int y, int w, int h, uint8_t col, int t)
{
    d_rect(s, x, y, w, t, col);
    d_rect(s, x, y + h - t, w, t, col);
    d_rect(s, x, y, t, h, col);
    d_rect(s, x + w - t, y, t, h, col);
}

// Characters the prototype's FONT defines; anything else draws as '?'.
static bool has_glyph(unsigned char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c >= 0x80 && c <= 0x8A))
        return true;
    return c && strchr(" !?.,:'-+=/()&<>_", c) != NULL;
}

static int glyph_index(unsigned char c)
{
    if (c >= 'a' && c <= 'z')
        c = (unsigned char)(c - 'a' + 'A');
    if (!has_glyph(c))
        c = '?';
    if (c >= 0x80)
        return 96 + (c - 0x80);
    return c - 32;
}

int d_twn(int len, int sc)
{
    return len > 0 ? (len * 6 - 1) * sc : 0;
}

int d_tw(const char *str, int sc)
{
    return d_twn((int)strlen(str), sc);
}

void d_textn(Surface *s, const char *str, int len, int x, int y, uint8_t col, int sc)
{
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c == ' ')
            continue;
        const uint8_t *g = font5x7[glyph_index(c)];
        int gx = x + i * 6 * sc;
        for (int row = 0; row < 7; row++) {
            uint8_t bits = g[row];
            if (!bits)
                continue;
            for (int b = 0; b < 5; b++)
                if (bits & (0x10 >> b))
                    d_rect(s, gx + b * sc, y + row * sc, sc, sc, col);
        }
    }
}

void d_text(Surface *s, const char *str, int x, int y, uint8_t col, int sc)
{
    d_textn(s, str, (int)strlen(str), x, y, col, sc);
}

void d_text_c(Surface *s, const char *str, int x, int w, int y, uint8_t col, int sc)
{
    d_text(s, str, x + floordiv(w - d_tw(str, sc), 2), y, col, sc);
}

void d_text_r(Surface *s, const char *str, int r, int y, uint8_t col, int sc)
{
    d_text(s, str, r - d_tw(str, sc), y, col, sc);
}

int icon_w(int icon)
{
    return ICON_DEFS[icon].w;
}

int icon_h(int icon)
{
    return ICON_DEFS[icon].h;
}

void d_icon(Surface *s, int icon, int x, int y, uint8_t c0, uint8_t c1, uint8_t c2, int sc)
{
    const IconDef *d = &ICON_DEFS[icon];
    for (int yy = 0; yy < d->h; yy++) {
        for (int xx = 0; xx < d->w; xx++) {
            char p = d->rows[yy][xx];
            uint8_t c = p == '#' ? c0 : p == '+' ? c1 : p == '=' ? c2 : C_NONE;
            if (c != C_NONE)
                d_rect(s, x + xx * sc, y + yy * sc, sc, sc, c);
        }
    }
}

// Row insets of the prototype's shield() for the two sizes the sheet draws
// (outline 48x42 and fill 44x38), computed with its exact formula:
// min(half - 1, round((half - 1) * p^1.8)) with no floating point needed here.
static const uint8_t SHIELD_48x42[42] = { 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2,
                                          2, 3, 3, 4, 5, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 19, 20, 21, 23 };
static const uint8_t SHIELD_44x38[38] = { 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                          2, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 17, 18, 19, 21 };

void d_shield(Surface *s, int x, int y, int w, int h, uint8_t col)
{
    const uint8_t *table = (w == 48 && h == 42) ? SHIELD_48x42 : (w == 44 && h == 38) ? SHIELD_44x38 : NULL;
    int half = w / 2, start = (h + 1) / 3; // Math.round(h / 3)
    for (int r = 0; r < h; r++) {
        int inset;
        if (table) {
            inset = table[r];
        } else if (r < 2) {
            inset = 2 - r;
        } else if (r < start) {
            inset = 0;
        } else {
            // Other sizes: p^2 stands in for p^1.8.
            int n = h - 1 - start, k = r - start;
            inset = n > 0 ? ((half - 1) * k * k + n * n / 2) / (n * n) : 0;
        }
        d_rect(s, x + inset, y + r, w - inset * 2, 1, col);
    }
}

void d_image(Surface *s, const uint16_t *px, int w, int h, int x, int y)
{
    if (!s->px16)
        return;
    for (int yy = 0; yy < h; yy++) {
        int sy = y + yy;
        if (sy < 0 || sy >= s->h)
            continue;
        for (int xx = 0; xx < w; xx++) {
            int sx = x + xx;
            if (sx >= 0 && sx < s->w)
                s->px16[sy * s->w + sx] = (uint16_t)(px[yy * w + xx] | 0x8000);
        }
    }
}

void d_dim(Surface *s)
{
    if (!s->px16)
        return;
    // out = 0.72 * (8, 4, 16) + 0.28 * c, computed on 8-bit channels like the canvas.
    static const int OV[3] = { 8, 4, 16 };
    for (int i = 0; i < s->w * s->h; i++) {
        uint16_t p = s->px16[i];
        uint16_t o = 0;
        for (int ch = 0; ch < 3; ch++) {
            int c8 = ((p >> (5 * ch)) & 31) << 3;
            int v = (72 * OV[ch] + 28 * c8 + 50) / 100;
            o = (uint16_t)(o | ((v >> 3) << (5 * ch)));
        }
        s->px16[i] = (uint16_t)(o | 0x8000);
    }
}
