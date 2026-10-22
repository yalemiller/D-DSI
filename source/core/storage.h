// Save and load procedures on top of the Fs interface. Never reports a save
// that didn't happen: every step's result is checked.
#pragma once

#include <stdbool.h>

#include "fs.h"
#include "save.h"

#define FILE_SAV "sheet.sav"
#define FILE_BAK "sheet.bak"
#define FILE_TMP "sheet.tmp"
#define FILE_BAD "sheet.bad"
#define FILE_SPELLS "spells.txt"
#define FILE_FEATURES "features.txt"

typedef enum {
    SAVE_DONE,
    SAVE_FAIL_WRITE,  // sheet.tmp couldn't be written
    SAVE_FAIL_VERIFY, // reading sheet.tmp back didn't match
    SAVE_FAIL_RENAME, // a rename failed (sheet.sav may now be sheet.bak)
} SaveResult;

// "WRITE", "VERIFY", "RENAME" (shown as SAVE FAILED (code)).
const char *storage_save_code(SaveResult r);

// 1. write sheet.tmp  2. read it back and compare  3. delete sheet.bak and
// rename sheet.sav -> sheet.bak  4. rename sheet.tmp -> sheet.sav.
SaveResult storage_save(const Fs *fs, const Roster *r, uint8_t last);

typedef enum {
    LOAD_FIRST_RUN,       // no save and no backup: the caller creates the demo characters
    LOAD_OK,              // sheet.sav is valid
    LOAD_MIGRATED,        // sheet.sav was an older version: mark the session unsaved
    LOAD_BACKUP_RESTORED, // sheet.sav was bad or missing, sheet.bak was good
    LOAD_BOTH_BAD,        // neither file is usable: START FRESH or TEMP SESSION
} LoadKind;

typedef struct {
    LoadKind kind;
    uint8_t last;
    ParseStatus savReason; // why sheet.sav was rejected
    ParseStatus bakReason; // why sheet.bak was rejected
    bool savExisted;       // a damaged sheet.sav was kept as sheet.bad
} LoadResult;

// The load cascade. `out` is filled for LOAD_OK, LOAD_MIGRATED and
// LOAD_BACKUP_RESTORED and cleared otherwise. A damaged sheet.sav is kept as sheet.bad.
LoadResult storage_load(const Fs *fs, Roster *out);

// Reads a whole file; NULL when missing or unreadable. Free with free().
uint8_t *storage_read(const Fs *fs, const char *name, size_t *len);
