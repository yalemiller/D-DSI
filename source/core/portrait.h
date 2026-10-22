// Portrait loader: an uncompressed 48x48 Windows BMP (24-bit, or 8-bit
// indexed), bottom-up or top-down, converted to RGB555.
#pragma once

#include <stddef.h>
#include <stdint.h>

#define PORTRAIT_SIZE 48

typedef enum {
    BMP_OK,
    BMP_NOT_BMP,
    BMP_HEADER,
    BMP_NOT_48,
    BMP_BITS,
    BMP_TRUNCATED,
    BMP_PALETTE,
} BmpStatus;

// "NOT A BMP", "NOT 48X48", ...
const char *portrait_reason(BmpStatus s);

// Decodes into `out` as RGB555 (bits 0-4 red, 5-9 green, 10-14 blue; bit 15 clear).
BmpStatus portrait_decode(const uint8_t *b, size_t len, uint16_t out[PORTRAIT_SIZE * PORTRAIT_SIZE]);
