#include "memfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MemFile *memfs_get(MemFs *m, const char *name)
{
    for (int i = 0; i < MEMFS_FILES; i++)
        if (m->files[i].used && !strcmp(m->files[i].name, name))
            return &m->files[i];
    return NULL;
}

bool memfs_put(MemFs *m, const char *name, const void *data, size_t len)
{
    MemFile *f = memfs_get(m, name);
    for (int i = 0; !f && i < MEMFS_FILES; i++)
        if (!m->files[i].used)
            f = &m->files[i];
    if (!f)
        return false;
    unsigned char *copy = malloc(len ? len : 1);
    if (!copy)
        return false;
    if (len)
        memcpy(copy, data, len);
    free(f->data);
    f->data = copy;
    f->len = len;
    f->used = true;
    snprintf(f->name, sizeof f->name, "%s", name);
    return true;
}

static void drop(MemFile *f)
{
    free(f->data);
    memset(f, 0, sizeof *f);
}

void memfs_clear(MemFs *m)
{
    for (int i = 0; i < MEMFS_FILES; i++)
        drop(&m->files[i]);
}

static FsStatus mem_read(void *ctx, const char *name, uint8_t **data, size_t *len)
{
    MemFs *m = ctx;
    *data = NULL;
    *len = 0;
    if (m->missing)
        return FS_MISSING;
    MemFile *f = memfs_get(m, name);
    if (!f)
        return FS_MISSING;
    if (m->failReads)
        return FS_ERROR;
    *data = malloc(f->len + 1);
    if (!*data)
        return FS_ERROR;
    memcpy(*data, f->data, f->len);
    (*data)[f->len] = 0;
    if (m->corruptReads && f->len)
        (*data)[f->len / 2] ^= 0x5A;
    *len = f->len;
    return FS_OK;
}

static bool mem_write(void *ctx, const char *name, const uint8_t *data, size_t len)
{
    MemFs *m = ctx;
    if (m->missing || m->readonly)
        return false;
    if (!memfs_put(m, name, data, len))
        return false;
    m->writes++;
    return true;
}

static bool mem_exists(void *ctx, const char *name)
{
    MemFs *m = ctx;
    return !m->missing && memfs_get(m, name) != NULL;
}

static bool mem_remove(void *ctx, const char *name)
{
    MemFs *m = ctx;
    MemFile *f = memfs_get(m, name);
    if (m->missing || m->readonly || !f)
        return false;
    drop(f);
    return true;
}

static bool mem_rename(void *ctx, const char *from, const char *to)
{
    MemFs *m = ctx;
    MemFile *f = memfs_get(m, from);
    if (m->missing || m->readonly || m->failRename || !f || memfs_get(m, to))
        return false;
    snprintf(f->name, sizeof f->name, "%s", to);
    return true;
}

void memfs_init(MemFs *m, Fs *fs)
{
    memset(m, 0, sizeof *m);
    fs->ctx = m;
    fs->read = mem_read;
    fs->write = mem_write;
    fs->exists = mem_exists;
    fs->remove = mem_remove;
    fs->rename = mem_rename;
}
