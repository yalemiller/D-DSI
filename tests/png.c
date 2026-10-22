// Minimal PNG writer (RGB, uncompressed deflate blocks) for the screenshot runner.
#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/save.h" // crc32_calc: PNG uses the same CRC-32

static void be32(unsigned char *b, uint32_t v)
{
    b[0] = (unsigned char)(v >> 24);
    b[1] = (unsigned char)(v >> 16);
    b[2] = (unsigned char)(v >> 8);
    b[3] = (unsigned char)v;
}

static bool chunk(FILE *f, const char *type, const unsigned char *data, size_t len)
{
    unsigned char head[8];
    be32(head, (uint32_t)len);
    memcpy(head + 4, type, 4);
    unsigned char *crcbuf = malloc(len + 4);
    if (!crcbuf)
        return false;
    memcpy(crcbuf, type, 4);
    if (len)
        memcpy(crcbuf + 4, data, len);
    unsigned char crc[4];
    be32(crc, crc32_calc(crcbuf, len + 4));
    free(crcbuf);
    return fwrite(head, 1, 8, f) == 8 && (!len || fwrite(data, 1, len, f) == len) && fwrite(crc, 1, 4, f) == 4;
}

bool png_write(const char *path, const unsigned char *rgb, int w, int h)
{
    size_t row = (size_t)w * 3 + 1, raw = row * (size_t)h;
    size_t blocks = (raw + 65534) / 65535;
    size_t zlen = 2 + raw + blocks * 5 + 4;
    unsigned char *z = malloc(zlen);
    if (!z)
        return false;
    // zlib stream of stored blocks over the filtered rows (filter 0)
    unsigned char *p = z;
    *p++ = 0x78;
    *p++ = 0x01;
    uint32_t a = 1, b = 0;
    size_t done = 0;
    unsigned char *rawbuf = malloc(raw);
    if (!rawbuf) {
        free(z);
        return false;
    }
    for (int y = 0; y < h; y++) {
        rawbuf[y * row] = 0;
        memcpy(rawbuf + y * row + 1, rgb + (size_t)y * w * 3, (size_t)w * 3);
    }
    for (size_t i = 0; i < raw; i++) {
        a = (a + rawbuf[i]) % 65521;
        b = (b + a) % 65521;
    }
    while (done < raw) {
        size_t n = raw - done > 65535 ? 65535 : raw - done;
        *p++ = done + n == raw ? 1 : 0;
        *p++ = (unsigned char)(n & 255);
        *p++ = (unsigned char)(n >> 8);
        *p++ = (unsigned char)(~n & 255);
        *p++ = (unsigned char)((~n >> 8) & 255);
        memcpy(p, rawbuf + done, n);
        p += n;
        done += n;
    }
    be32(p, (b << 16) | a);
    p += 4;
    free(rawbuf);

    FILE *f = fopen(path, "wb");
    if (!f) {
        free(z);
        return false;
    }
    static const unsigned char SIG[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n' };
    unsigned char ihdr[13];
    be32(ihdr, (uint32_t)w);
    be32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8; // bit depth
    ihdr[9] = 2; // RGB
    ihdr[10] = ihdr[11] = ihdr[12] = 0;
    bool ok = fwrite(SIG, 1, 8, f) == 8 && chunk(f, "IHDR", ihdr, 13) && chunk(f, "IDAT", z, (size_t)(p - z)) &&
              chunk(f, "IEND", NULL, 0);
    free(z);
    return fclose(f) == 0 && ok;
}
