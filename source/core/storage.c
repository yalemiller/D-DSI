#include "storage.h"

#include <stdlib.h>
#include <string.h>

const char *storage_save_code(SaveResult r)
{
    switch (r) {
    case SAVE_DONE:
        return "OK";
    case SAVE_FAIL_WRITE:
        return "WRITE";
    case SAVE_FAIL_VERIFY:
        return "VERIFY";
    case SAVE_FAIL_RENAME:
        return "RENAME";
    }
    return "?";
}

uint8_t *storage_read(const Fs *fs, const char *name, size_t *len)
{
    uint8_t *data = NULL;
    *len = 0;
    if (fs->read(fs->ctx, name, &data, len) != FS_OK) {
        free(data);
        *len = 0;
        return NULL;
    }
    return data;
}

SaveResult storage_save(const Fs *fs, const Roster *r, uint8_t last)
{
    static uint8_t bytes[SAVE_BYTES]; // static: the DS stack is small
    save_serialize(r, last, bytes);
    if (!fs->write(fs->ctx, FILE_TMP, bytes, SAVE_BYTES))
        return SAVE_FAIL_WRITE;
    size_t n;
    uint8_t *back = storage_read(fs, FILE_TMP, &n);
    bool same = back && n == SAVE_BYTES && !memcmp(back, bytes, SAVE_BYTES);
    free(back);
    if (!same) {
        fs->remove(fs->ctx, FILE_TMP);
        return SAVE_FAIL_VERIFY;
    }
    if (fs->exists(fs->ctx, FILE_SAV)) {
        fs->remove(fs->ctx, FILE_BAK); // may not exist
        if (!fs->rename(fs->ctx, FILE_SAV, FILE_BAK))
            return SAVE_FAIL_RENAME;
    }
    if (!fs->rename(fs->ctx, FILE_TMP, FILE_SAV))
        return SAVE_FAIL_RENAME;
    return SAVE_DONE;
}

static ParseStatus read_and_parse(const Fs *fs, const char *name, Roster *out, uint8_t *last, bool *migrated)
{
    uint8_t *data = NULL;
    size_t len = 0;
    FsStatus st = fs->read(fs->ctx, name, &data, &len);
    ParseStatus p;
    if (st == FS_MISSING)
        p = PARSE_MISSING;
    else if (st != FS_OK)
        p = PARSE_READ;
    else
        p = save_parse(data, len, out, last, migrated);
    free(data);
    return p;
}

LoadResult storage_load(const Fs *fs, Roster *out)
{
    LoadResult res;
    memset(&res, 0, sizeof res);
    bool migrated = false;
    res.savReason = read_and_parse(fs, FILE_SAV, out, &res.last, &migrated);
    if (res.savReason == PARSE_OK) {
        res.kind = migrated ? LOAD_MIGRATED : LOAD_OK;
        return res;
    }
    res.bakReason = read_and_parse(fs, FILE_BAK, out, &res.last, &migrated);
    if (res.savReason == PARSE_MISSING && res.bakReason == PARSE_MISSING) {
        memset(out, 0, sizeof *out);
        res.kind = LOAD_FIRST_RUN;
        return res;
    }
    if (res.savReason != PARSE_MISSING) {
        // Keep the damaged file for the user to inspect.
        fs->remove(fs->ctx, FILE_BAD);
        res.savExisted = fs->rename(fs->ctx, FILE_SAV, FILE_BAD);
    }
    if (res.bakReason == PARSE_OK) {
        res.kind = LOAD_BACKUP_RESTORED;
        return res;
    }
    memset(out, 0, sizeof *out);
    res.last = 0;
    res.kind = LOAD_BOTH_BAD;
    return res;
}
