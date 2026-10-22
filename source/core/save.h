// Save format v5 (fat:/data/pocketsheet/sheet.sav, 3,760 bytes, little-endian).
// Read and written field by field; v1-v4 files are read and migrated.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "character.h"

#define SAVE_VERSION 5
#define SAVE_HEADER 16
#define SAVE_RECORD 1248
#define SAVE_BYTES (SAVE_HEADER + SAVE_RECORD * NUM_SLOTS)

typedef enum {
    PARSE_OK,
    PARSE_MISSING,
    PARSE_READ, // the file exists but couldn't be read
    PARSE_MAGIC,
    PARSE_VERSION,
    PARSE_SIZE,
    PARSE_SLOTS,
    PARSE_CHECKSUM,
    PARSE_RANGE,
} ParseStatus;

// The three character slots as stored in the file.
typedef struct {
    bool used[NUM_SLOTS];
    Character slot[NUM_SLOTS];
} Roster;

// "MISSING", "MAGIC", "CHECKSUM", ... as shown in notices.
const char *save_reason(ParseStatus s);

uint32_t crc32_calc(const uint8_t *b, size_t n);

// Writes a complete v5 file. `last` is the slot to reopen next time.
void save_serialize(const Roster *r, uint8_t last, uint8_t out[SAVE_BYTES]);

// Parses any version 1-5. On success fills `out` and `last`, and sets
// `migrated` when the file was older than v5 (the session must be marked unsaved).
// On failure `out` holds partial data and must not be used.
ParseStatus save_parse(const uint8_t *b, size_t len, Roster *out, uint8_t *last, bool *migrated);
