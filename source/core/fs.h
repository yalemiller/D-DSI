// The file interface the core uses. Names are bare file names inside the app
// folder ("sheet.sav"); the platform implementation owns the folder path
// (fat:/data/pocketsheet/ on the DS), so core code can't reach any other drive.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define FS_MAX_READ (1024 * 1024)

typedef enum {
    FS_OK,
    FS_MISSING,
    FS_ERROR,
} FsStatus;

typedef struct Fs {
    void *ctx;
    // Reads a whole file into a malloc'd buffer (with a 0 byte after the data).
    // The caller frees *data. Files over FS_MAX_READ are FS_ERROR.
    FsStatus (*read)(void *ctx, const char *name, uint8_t **data, size_t *len);
    // Creates or truncates, writes everything, flushes and closes: true only if every step worked.
    bool (*write)(void *ctx, const char *name, const uint8_t *data, size_t len);
    bool (*exists)(void *ctx, const char *name);
    bool (*remove)(void *ctx, const char *name);
    // Fails if `to` exists.
    bool (*rename)(void *ctx, const char *from, const char *to);
} Fs;
