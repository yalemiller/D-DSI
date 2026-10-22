// In-memory Fs for tests, with switches to simulate a missing card, failed
// writes, failed renames and corrupted read-back.
#pragma once

#include <stdbool.h>

#include "core/fs.h"

#define MEMFS_FILES 16

typedef struct {
    char name[32];
    unsigned char *data;
    size_t len;
    bool used;
} MemFile;

typedef struct {
    MemFile files[MEMFS_FILES];
    bool missing;      // no card: every call fails / reads as missing
    bool readonly;     // writes, removes and renames fail
    bool failRename;   // only renames fail
    bool corruptReads; // reads come back with one byte flipped
    bool failReads;    // reads report an I/O error
    int writes;        // successful writes
} MemFs;

void memfs_init(MemFs *m, Fs *fs);
void memfs_clear(MemFs *m);
bool memfs_put(MemFs *m, const char *name, const void *data, size_t len);
MemFile *memfs_get(MemFs *m, const char *name);
