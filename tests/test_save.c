#include <stdlib.h>

#include "core/demo.h"
#include "core/save.h"
#include "test.h"
#include "tests.h"

static uint8_t file[SAVE_BYTES];
static Roster in, out;

static void put16(uint8_t *b, unsigned v)
{
    b[0] = (uint8_t)(v & 255);
    b[1] = (uint8_t)(v >> 8);
}

static void fix_crc(uint8_t *b, size_t len)
{
    memset(b + 8, 0, 4);
    uint32_t c = crc32_calc(b, len);
    for (int i = 0; i < 4; i++)
        b[8 + i] = (uint8_t)(c >> (8 * i));
}

static ParseStatus parse(const uint8_t *b, size_t len, bool *migrated)
{
    uint8_t last;
    return save_parse(b, len, &out, &last, migrated);
}

// Builds a pre-v5 file with one character in slot 0.
static size_t build_old(uint8_t *b, int ver)
{
    static const unsigned REC[] = { 0, 96, 112, 224, 752 };
    size_t len = 16 + REC[ver] * 3;
    memset(b, 0, len);
    memcpy(b, "PKSH", 4);
    put16(b + 4, (unsigned)ver);
    b[6] = 3;
    b[7] = 0;
    uint8_t *o = b + 16;
    o[0] = 1;
    memcpy(o + 1, "OLD HERO", 8);
    memcpy(o + 17, "FIGHTER", 7);
    o[29] = 3;
    for (int i = 0; i < 6; i++)
        o[30 + i] = (uint8_t)(10 + i);
    o[36] = 1;
    o[37 + 3] = 1;
    put16(o + 55, 12);
    put16(o + 57, 28);
    put16(o + 59, 0);
    o[61] = 16;
    o[62] = 30;
    o[65] = 2;
    o[83] = 255;
    if (ver >= 2)
        memcpy(o + 85, "CHAMPION", 8);
    if (ver >= 3) {
        o[101] = 10;
        o[102] = 1;
        uint8_t *q = o + 112; // 28-byte weapon records
        q[0] = 1;
        memcpy(q + 1, "LONGSWORD", 9);
        q[13] = 0 | 4;
        q[14] = 1; // old magic bonus
        q[15] = 1;
        q[16] = 8;
        q[17] = 0;
        memcpy(q + 18, "5 FT", 4);
        q[25] = 7; // TOPPLE
    }
    if (ver >= 4) {
        uint8_t *q = o + 224;
        memcpy(q, "BLESS", 5);
        q[20] = 3;
    }
    fix_crc(b, len);
    return len;
}

void test_save(void)
{
    // CRC-32 check value
    CHECK_INT(crc32_calc((const uint8_t *)"123456789", 9), 0xCBF43926u);

    // round trip of the first-run characters
    demo_roster(&in);
    save_serialize(&in, 1, file);
    CHECK(!memcmp(file, "PKSH", 4));
    CHECK_INT(file[4], 5);
    CHECK_INT(file[6], 3);
    CHECK_INT(file[7], 1);
    bool migrated = true;
    uint8_t last = 9;
    CHECK_INT(save_parse(file, SAVE_BYTES, &out, &last, &migrated), PARSE_OK);
    CHECK(!migrated);
    CHECK_INT(last, 1);
    CHECK(out.used[0] && out.used[1] && !out.used[2]);
    CHECK(!memcmp(&out.slot[0], &in.slot[0], sizeof in.slot[0]));
    CHECK(!memcmp(&out.slot[1], &in.slot[1], sizeof in.slot[1]));
    CHECK_STR(out.slot[0].weapons[0].props, "BARREL 6, BLARING, FAN FIRE");
    CHECK_INT(out.slot[0].weapons[0].dmg, 3);
    CHECK_INT(out.slot[0].weapons[0].bonus, 2);
    CHECK_INT(out.slot[0].nFeats, 13);
    CHECK_STR(out.slot[0].species, "HUMAN (HALF-ELF)");
    CHECK_INT(out.slot[1].nSpells, 17);
    CHECK_INT(out.slot[1].spells[4].prep, 1); // BLESS*

    // 0 HP (down) round-trips through the s16 field
    in.slot[0].hp = 0;
    save_serialize(&in, 0, file);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_OK);
    CHECK_INT(out.slot[0].hp, 0);

    // header failures
    save_serialize(&in, 7, file);
    CHECK_INT(save_parse(file, SAVE_BYTES, &out, &last, &migrated), PARSE_OK);
    CHECK_INT(last, 2); // clamped
    CHECK_INT(parse(NULL, 0, &migrated), PARSE_MISSING);
    CHECK_INT(parse(file, 10, &migrated), PARSE_MAGIC);
    file[0] = 'X';
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_MAGIC);
    save_serialize(&in, 0, file);
    put16(file + 4, 6);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_VERSION);
    put16(file + 4, 0);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_VERSION);
    save_serialize(&in, 0, file);
    CHECK_INT(parse(file, SAVE_BYTES - 1, &migrated), PARSE_SIZE);
    file[6] = 2;
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_SLOTS);
    save_serialize(&in, 0, file);
    file[SAVE_HEADER + 3] ^= 0x5A; // what the prototype's "corrupt" button does
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_CHECKSUM);

    // range checks (with a fixed-up CRC so only the field is wrong)
    uint8_t *rec = file + SAVE_HEADER;
    save_serialize(&in, 0, file);
    rec[29] = 0; // level 0
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    rec[1] = 'j'; // lowercase name
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    rec[0] = 2; // used flag
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    put16(rec + 55, 999); // hp > max
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    memcpy(rec + 336 + 22, rec + 336, 20); // duplicate spell name
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    rec[112 + 13] = 8; // weapon flags
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    rec[112 + 26] = 31; // damage bonus over +30
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    rec[864 + 20] = 21; // feature uses over 20
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);
    save_serialize(&in, 0, file);
    rec[74] = 3; // used slots over max (2)
    fix_crc(file, SAVE_BYTES);
    CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_RANGE);

    // migration from every older version
    static uint8_t old[16 + 752 * 3];
    for (int ver = 1; ver <= 4; ver++) {
        size_t len = build_old(old, ver);
        migrated = false;
        CHECK_INT(parse(old, len, &migrated), PARSE_OK);
        CHECK(migrated);
        const Character *k = &out.slot[0];
        CHECK_STR(k->name, "OLD HERO");
        CHECK_STR(k->cls, "FIGHTER");
        CHECK_STR(k->sub, ver >= 2 ? "CHAMPION" : "");
        CHECK_INT(k->level, 3);
        CHECK_INT(k->ab[5], 15);
        CHECK_INT(k->hp, 12);
        CHECK_INT(k->hpMax, 28);
        CHECK_INT(k->hitDie, ver >= 3 ? 10 : 8);
        CHECK_INT(k->hdUsed, ver >= 3 ? 1 : 0);
        CHECK_INT(k->nWeapons, ver >= 3 ? 1 : 0);
        CHECK_INT(k->nSpells, ver >= 4 ? 1 : 0);
        CHECK_INT(k->nFeats, 0);
        CHECK_STR(k->species, "");
        if (ver >= 3) {
            CHECK_INT(k->weapons[0].bonus, 1);
            CHECK_INT(k->weapons[0].dmg, 1); // the old magic bonus applied to damage too
            CHECK_INT(k->weapons[0].mastery, MASTERY_TOPPLE);
            CHECK_STR(k->weapons[0].props, "");
        }
        if (ver >= 4) {
            CHECK_STR(k->spells[0].name, "BLESS");
            CHECK_INT(k->spells[0].prep, 1);
        }
        // a v3 file with hit die 0 reads as D8
        if (ver == 3) {
            old[16 + 101] = 0;
            fix_crc(old, len);
            CHECK_INT(parse(old, len, &migrated), PARSE_OK);
            CHECK_INT(out.slot[0].hitDie, 8);
        }
        // and writing it back produces a current v5 file
        save_serialize(&out, 0, file);
        CHECK_INT(parse(file, SAVE_BYTES, &migrated), PARSE_OK);
        CHECK(!migrated);
    }
    CHECK_STR(save_reason(PARSE_CHECKSUM), "CHECKSUM");
}
