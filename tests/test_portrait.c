#include "core/demo.h"
#include "core/portrait.h"
#include "test.h"
#include "tests.h"

static uint8_t bmp[8192];
static uint16_t px[PORTRAIT_SIZE * PORTRAIT_SIZE];

static void put32(uint8_t *b, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        b[i] = (uint8_t)(v >> (8 * i));
}

// A 48x48 BMP. 24-bit is all black except data row 0, pixel 0 = red.
// 8-bit has a 4-color palette (0 black, 2 blue) and every pixel = 2.
static size_t make(int bpp, int height, uint32_t ncol)
{
    memset(bmp, 0, sizeof bmp);
    uint32_t stride = bpp == 24 ? 144 : 48;
    uint32_t off = 54 + (bpp == 8 ? ncol * 4 : 0);
    uint32_t size = off + stride * 48;
    bmp[0] = 'B';
    bmp[1] = 'M';
    put32(bmp + 2, size);
    put32(bmp + 10, off);
    put32(bmp + 14, 40);
    put32(bmp + 18, 48);
    put32(bmp + 22, (uint32_t)height);
    bmp[26] = 1;
    bmp[28] = (uint8_t)bpp;
    put32(bmp + 46, ncol);
    if (bpp == 24) {
        bmp[off + 2] = 255; // B, G, R
        bmp[off + 3] = 50;
        bmp[off + 4] = 100;
        bmp[off + 5] = 200;
    } else {
        bmp[54 + 2 * 4 + 0] = 255; // palette entry 2: blue
        memset(bmp + off, 2, stride * 48);
    }
    return size;
}

void test_portrait(void)
{
    size_t n = make(24, 48, 0);
    CHECK_INT(n, 6966);
    CHECK_INT(portrait_decode(bmp, n, px), BMP_OK);
    CHECK_INT(px[47 * 48 + 0], 31);                          // bottom-up: data row 0 is the bottom row
    CHECK_INT(px[47 * 48 + 1], 25 | (12 << 5) | (6 << 10));  // (200,100,50) >> 3
    CHECK_INT(px[0], 0);
    n = make(24, -48, 0);
    CHECK_INT(portrait_decode(bmp, n, px), BMP_OK);
    CHECK_INT(px[0], 31); // top-down

    n = make(8, 48, 4);
    CHECK_INT(portrait_decode(bmp, n, px), BMP_OK);
    CHECK_INT(px[0], 31 << 10);
    CHECK_INT(px[48 * 48 - 1], 31 << 10);
    bmp[54 + 16 + 5] = 4; // index past the 4-color palette
    CHECK_INT(portrait_decode(bmp, n, px), BMP_PALETTE);
    n = make(8, 48, 4);
    put32(bmp + 46, 0); // 0 means 256 colors, which don't fit before the pixels
    CHECK_INT(portrait_decode(bmp, n, px), BMP_PALETTE);

    n = make(24, 48, 0);
    CHECK_INT(portrait_decode(bmp, 53, px), BMP_NOT_BMP);
    CHECK_INT(portrait_decode(NULL, 0, px), BMP_NOT_BMP);
    CHECK_INT(portrait_decode(bmp, n - 1, px), BMP_TRUNCATED);
    bmp[1] = 'X';
    CHECK_INT(portrait_decode(bmp, n, px), BMP_NOT_BMP);
    make(24, 48, 0);
    put32(bmp + 14, 12);
    CHECK_INT(portrait_decode(bmp, n, px), BMP_HEADER);
    make(24, 48, 0);
    bmp[26] = 2;
    CHECK_INT(portrait_decode(bmp, n, px), BMP_HEADER);
    make(24, 48, 0);
    put32(bmp + 18, 32);
    CHECK_INT(portrait_decode(bmp, n, px), BMP_NOT_48);
    make(24, 47, 0);
    CHECK_INT(portrait_decode(bmp, n, px), BMP_NOT_48);
    make(24, 48, 0);
    bmp[28] = 16;
    CHECK_INT(portrait_decode(bmp, n, px), BMP_BITS);
    make(24, 48, 0);
    bmp[30] = 1;
    CHECK_INT(portrait_decode(bmp, n, px), BMP_BITS);
    make(24, 48, 0);
    put32(bmp + 10, 0xFFFFFFF0u); // offset past the end, no overflow
    CHECK_INT(portrait_decode(bmp, n, px), BMP_TRUNCATED);

    CHECK_INT(portrait_decode(DEMO_PORTRAIT1_BMP, sizeof DEMO_PORTRAIT1_BMP, px), BMP_OK);
    CHECK_STR(portrait_reason(BMP_NOT_48), "NOT 48X48");
}
