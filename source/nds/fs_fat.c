#include "fs_fat.h"

#include <errno.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ROOT_DIR "fat:/data"
#define APP_DIR "fat:/data/pocketsheet"

static bool fat_ready;

static bool path_for(const char *name, char *out, size_t cap)
{
    int n = snprintf(out, cap, APP_DIR "/%s", name);
    return n > 0 && (size_t)n < cap;
}

static bool make_dir(const char *path)
{
    return mkdir(path, 0777) == 0 || errno == EEXIST;
}

static bool fat_mount(void *ctx)
{
    (void)ctx;
    if (!fat_ready)
        fat_ready = fatInitDefault();
    if (!fat_ready)
        return false;
    if (!make_dir(ROOT_DIR) || !make_dir(APP_DIR))
        return false;
    struct stat st;
    return stat(APP_DIR, &st) == 0 && S_ISDIR(st.st_mode);
}

static FsStatus fat_read(void *ctx, const char *name, uint8_t **data, size_t *len)
{
    (void)ctx;
    char path[96];
    *data = NULL;
    *len = 0;
    if (!fat_ready || !path_for(name, path, sizeof path))
        return FS_ERROR;
    FILE *f = fopen(path, "rb");
    if (!f)
        return errno == ENOENT ? FS_MISSING : FS_ERROR;
    long size = -1;
    if (fseek(f, 0, SEEK_END) == 0)
        size = ftell(f);
    if (size < 0 || size > FS_MAX_READ || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return FS_ERROR;
    }
    uint8_t *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return FS_ERROR;
    }
    size_t got = fread(buf, 1, (size_t)size, f);
    bool ok = got == (size_t)size && !ferror(f);
    if (fclose(f) != 0)
        ok = false;
    if (!ok) {
        free(buf);
        return FS_ERROR;
    }
    buf[size] = 0;
    *data = buf;
    *len = (size_t)size;
    return FS_OK;
}

static bool fat_write(void *ctx, const char *name, const uint8_t *data, size_t len)
{
    (void)ctx;
    char path[96];
    if (!fat_ready || !path_for(name, path, sizeof path))
        return false;
    FILE *f = fopen(path, "wb");
    if (!f)
        return false;
    bool ok = fwrite(data, 1, len, f) == len;
    if (fflush(f) != 0)
        ok = false;
    if (fclose(f) != 0)
        ok = false;
    return ok;
}

static bool fat_exists(void *ctx, const char *name)
{
    (void)ctx;
    char path[96];
    struct stat st;
    return fat_ready && path_for(name, path, sizeof path) && stat(path, &st) == 0;
}

static bool fat_remove(void *ctx, const char *name)
{
    (void)ctx;
    char path[96];
    return fat_ready && path_for(name, path, sizeof path) && remove(path) == 0;
}

static bool fat_rename(void *ctx, const char *from, const char *to)
{
    char a[96], b[96];
    if (!fat_ready || !path_for(from, a, sizeof a) || !path_for(to, b, sizeof b) || fat_exists(ctx, to))
        return false;
    return rename(a, b) == 0;
}

void fs_fat_storage(Storage *out)
{
    out->mount = fat_mount;
    out->fs.ctx = NULL;
    out->fs.read = fat_read;
    out->fs.write = fat_write;
    out->fs.exists = fat_exists;
    out->fs.remove = fat_remove;
    out->fs.rename = fat_rename;
}
