#include "portrait.h"

const char *portrait_reason(BmpStatus s)
{
    switch (s) {
    case BMP_OK:
        return "OK";
    case BMP_NOT_BMP:
        return "NOT A BMP";
    case BMP_HEADER:
        return "HEADER";
    case BMP_NOT_48:
        return "NOT 48X48";
    case BMP_BITS:
        return "USE 24 OR 8 BIT";
    case BMP_TRUNCATED:
        return "TRUNCATED";
    case BMP_PALETTE:
        return "PALETTE";
    }
    return "BAD FILE";
}

static uint32_t u32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static unsigned u16(const uint8_t *b)
{
    return (unsigned)b[0] | ((unsigned)b[1] << 8);
}

static uint16_t rgb555(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10));
}

BmpStatus portrait_decode(const uint8_t *b, size_t len, uint16_t out[PORTRAIT_SIZE * PORTRAIT_SIZE])
{
    const int64_t P = PORTRAIT_SIZE;
    if (!b || len < 54 || b[0] != 'B' || b[1] != 'M')
        return BMP_NOT_BMP;
    int64_t off = u32(b + 10), dib = u32(b + 14);
    int64_t w = (int32_t)u32(b + 18), h = (int32_t)u32(b + 22);
    unsigned bpp = u16(b + 28);
    uint32_t comp = u32(b + 30);
    if (dib < 40 || u16(b + 26) != 1)
        return BMP_HEADER;
    if (w != P || (h != P && h != -P))
        return BMP_NOT_48;
    if ((bpp != 24 && bpp != 8) || comp != 0)
        return BMP_BITS;
    int64_t stride = ((w * bpp + 31) >> 5) * 4;
    if (off + stride * P > (int64_t)len)
        return BMP_TRUNCATED;
    int64_t pal = 14 + dib, ncol = 0;
    if (bpp == 8) {
        ncol = u32(b + 46);
        if (!ncol)
            ncol = 256;
        if (pal + ncol * 4 > off)
            return BMP_PALETTE;
    }
    for (int64_t y = 0; y < P; y++) {
        const uint8_t *row = b + off + (h > 0 ? P - 1 - y : y) * stride;
        for (int64_t x = 0; x < P; x++) {
            const uint8_t *p;
            if (bpp == 24) {
                p = row + x * 3;
            } else {
                if (row[x] >= ncol)
                    return BMP_PALETTE;
                p = b + pal + row[x] * 4;
            }
            out[y * P + x] = rgb555(p[2], p[1], p[0]);
        }
    }
    return BMP_OK;
}
