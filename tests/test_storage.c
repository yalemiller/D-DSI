#include "core/demo.h"
#include "core/storage.h"
#include "memfs.h"
#include "test.h"
#include "tests.h"

static MemFs m;
static Fs fs;
static Roster r, loaded;
static uint8_t bytes[SAVE_BYTES];

void test_storage(void)
{
    memfs_init(&m, &fs);
    demo_roster(&r);

    // first run: nothing on the card
    LoadResult lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_FIRST_RUN);
    CHECK(!loaded.used[0]);

    // the first save writes sheet.sav and leaves no temp file
    CHECK_INT(storage_save(&fs, &r, 1), SAVE_DONE);
    CHECK(memfs_get(&m, FILE_SAV) && !memfs_get(&m, FILE_TMP) && !memfs_get(&m, FILE_BAK));
    lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_OK);
    CHECK_INT(lr.last, 1);
    static uint8_t a[SAVE_BYTES], b[SAVE_BYTES];
    save_serialize(&loaded, 1, a);
    save_serialize(&r, 1, b);
    CHECK(!memcmp(a, b, SAVE_BYTES));

    // the second save keeps the previous file as sheet.bak
    r.slot[0].hp = 7;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_DONE);
    CHECK(memfs_get(&m, FILE_SAV) && memfs_get(&m, FILE_BAK));
    lr = storage_load(&fs, &loaded);
    CHECK_INT(loaded.slot[0].hp, 7);

    // write failure: nothing changes on the card
    MemFile *sav = memfs_get(&m, FILE_SAV);
    memcpy(bytes, sav->data, SAVE_BYTES);
    m.readonly = true;
    r.slot[0].hp = 5;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_FAIL_WRITE);
    m.readonly = false;
    CHECK(!memcmp(memfs_get(&m, FILE_SAV)->data, bytes, SAVE_BYTES));

    // read-back mismatch: the temp file is removed, sheet.sav untouched
    m.corruptReads = true;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_FAIL_VERIFY);
    m.corruptReads = false;
    CHECK(!memfs_get(&m, FILE_TMP));
    CHECK(!memcmp(memfs_get(&m, FILE_SAV)->data, bytes, SAVE_BYTES));

    // rename failure is reported, never claimed as saved
    m.failRename = true;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_FAIL_RENAME);
    m.failRename = false;
    memfs_clear(&m);

    // damaged sheet.sav + good sheet.bak: restore and keep the damaged file as sheet.bad
    r.slot[0].hp = 20;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_DONE);
    r.slot[0].hp = 9;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_DONE); // bak has hp 20, sav has hp 9
    memfs_get(&m, FILE_SAV)->data[SAVE_HEADER + 3] ^= 0x5A;
    lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_BACKUP_RESTORED);
    CHECK_INT(lr.savReason, PARSE_CHECKSUM);
    CHECK(lr.savExisted);
    CHECK_INT(loaded.slot[0].hp, 20);
    CHECK(memfs_get(&m, FILE_BAD) && !memfs_get(&m, FILE_SAV) && memfs_get(&m, FILE_BAK));

    // sheet.sav missing, backup good: restored, nothing renamed
    lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_BACKUP_RESTORED);
    CHECK_INT(lr.savReason, PARSE_MISSING);
    CHECK(!lr.savExisted);

    // both damaged
    memfs_get(&m, FILE_BAK)->data[0] = 'X';
    memfs_put(&m, FILE_SAV, "junk", 4);
    lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_BOTH_BAD);
    CHECK_INT(lr.savReason, PARSE_MAGIC);
    CHECK_INT(lr.bakReason, PARSE_MAGIC);
    CHECK(!loaded.used[0] && !loaded.used[1]);
    CHECK_INT(memfs_get(&m, FILE_BAD)->len, 4); // the newest damaged file replaced the old sheet.bad

    // bak missing and sav damaged
    memfs_clear(&m);
    memfs_put(&m, FILE_SAV, "PKSH", 4);
    lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_BOTH_BAD);
    CHECK_INT(lr.bakReason, PARSE_MISSING);

    // a v1 file (304 bytes) loads as migrated
    memfs_clear(&m);
    static uint8_t v1[16 + 96 * 3];
    memset(v1, 0, sizeof v1);
    memcpy(v1, "PKSH", 4);
    v1[4] = 1;
    v1[6] = 3;
    uint8_t *o = v1 + 16 + 96; // slot 2
    o[0] = 1;
    memcpy(o + 1, "OLDIE", 5);
    o[29] = 1;
    memset(o + 30, 10, 6);
    o[55] = o[57] = 10;
    o[61] = 10;
    o[62] = 30;
    o[83] = 255;
    uint32_t c = crc32_calc(v1, sizeof v1);
    for (int i = 0; i < 4; i++)
        v1[8 + i] = (uint8_t)(c >> (8 * i));
    memfs_put(&m, FILE_SAV, v1, sizeof v1);
    lr = storage_load(&fs, &loaded);
    CHECK_INT(lr.kind, LOAD_MIGRATED);
    CHECK(!loaded.used[0] && loaded.used[1]);
    CHECK_STR(loaded.slot[1].name, "OLDIE");

    // an unreadable sheet.sav counts as existing, not as a first run
    memfs_clear(&m);
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_DONE);
    m.failReads = true;
    lr = storage_load(&fs, &loaded);
    m.failReads = false;
    CHECK_INT(lr.kind, LOAD_BOTH_BAD);
    CHECK_INT(lr.savReason, PARSE_READ);

    // no card at all looks like a first run to storage; the app checks the mount first
    memfs_clear(&m);
    m.missing = true;
    CHECK_INT(storage_save(&fs, &r, 0), SAVE_FAIL_WRITE);
    m.missing = false;
    CHECK_STR(storage_save_code(SAVE_FAIL_VERIFY), "VERIFY");
    memfs_clear(&m);
}
