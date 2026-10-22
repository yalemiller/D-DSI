#include "save.h"

#include <string.h>

#include "text.h"

// Record size per version, and where the variable parts of a record live.
static const uint16_t RECORD_SIZE[6] = { 0, 96, 112, 224, 752, 1248 };

typedef struct {
    uint16_t weaponOff, weaponRec, spellOff, featOff, identOff;
} Layout;

static Layout layout_for(int ver)
{
    if (ver >= 5)
        return (Layout){ 112, 56, 336, 864, 1216 };
    return (Layout){ 112, 28, 224, 0, 0 };
}

#define SPELL_REC 22
#define FEAT_REC 22

const char *save_reason(ParseStatus s)
{
    switch (s) {
    case PARSE_OK:
        return "OK";
    case PARSE_MISSING:
        return "MISSING";
    case PARSE_READ:
        return "READ ERROR";
    case PARSE_MAGIC:
        return "MAGIC";
    case PARSE_VERSION:
        return "VERSION";
    case PARSE_SIZE:
        return "SIZE";
    case PARSE_SLOTS:
        return "SLOTS";
    case PARSE_CHECKSUM:
        return "CHECKSUM";
    case PARSE_RANGE:
        return "RANGE";
    }
    return "UNKNOWN";
}

uint32_t crc32_calc(const uint8_t *b, size_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        crc ^= b[i];
        for (int k = 0; k < 8; k++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
    return crc ^ 0xFFFFFFFFu;
}

// ---------- byte helpers (little-endian) ----------

static void put16(uint8_t *b, unsigned v)
{
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void put32(uint8_t *b, uint32_t v)
{
    for (int i = 0; i < 4; i++)
        b[i] = (uint8_t)((v >> (8 * i)) & 0xFF);
}

static unsigned get16(const uint8_t *b)
{
    return (unsigned)b[0] | ((unsigned)b[1] << 8);
}

static uint32_t get32(const uint8_t *b)
{
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

static int8_t get_s8(const uint8_t *b)
{
    return (int8_t)(b[0] < 128 ? b[0] : b[0] - 256);
}

static uint8_t from_s8(int v)
{
    return (uint8_t)(v < 0 ? v + 256 : v);
}

static void put_str(uint8_t *b, const char *s, size_t n)
{
    size_t i = 0;
    for (; i < n && s[i]; i++)
        b[i] = (uint8_t)s[i];
    for (; i < n; i++)
        b[i] = 0;
}

static void get_str(const uint8_t *b, size_t n, char *out)
{
    size_t i = 0;
    for (; i < n && b[i]; i++)
        out[i] = (char)b[i];
    out[i] = 0;
}

// ---------- write ----------

static void put_weapon(uint8_t *q, const Weapon *w)
{
    q[0] = 1;
    put_str(q + 1, w->name, WEAPON_NAME_LEN);
    q[13] = (uint8_t)((w->ab & 3) | (w->prof ? 4 : 0));
    q[14] = from_s8(w->bonus);
    q[15] = w->n;
    q[16] = w->die;
    q[17] = w->type;
    put_str(q + 18, w->range, WEAPON_RANGE_LEN);
    q[25] = w->mastery;
    q[26] = from_s8(w->dmg);
    put_str(q + 27, w->props, WEAPON_PROPS_LEN);
}

static void put_record(uint8_t *o, const Character *k)
{
    const Layout L = layout_for(SAVE_VERSION);
    o[0] = 1;
    put_str(o + 1, k->name, NAME_LEN);
    put_str(o + 17, k->cls, CLASS_LEN);
    o[29] = k->level;
    memcpy(o + 30, k->ab, 6);
    o[36] = k->saveProf;
    memcpy(o + 37, k->skill, SKILL_COUNT);
    put16(o + 55, (unsigned)(uint16_t)k->hp);
    put16(o + 57, k->hpMax);
    put16(o + 59, k->temp);
    o[61] = k->ac;
    o[62] = k->speed;
    o[63] = k->ds;
    o[64] = k->df;
    memcpy(o + 65, k->slotMax, 9);
    memcpy(o + 74, k->slotUsed, 9);
    o[83] = k->spellAb;
    o[84] = k->insp;
    put_str(o + 85, k->sub, SUBCLASS_LEN);
    o[101] = k->hitDie;
    o[102] = k->hdUsed;
    for (int j = 0; j < k->nSpells && j < MAX_SPELLS; j++) {
        uint8_t *q = o + L.spellOff + j * SPELL_REC;
        put_str(q, k->spells[j].name, SPELL_NAME_LEN);
        q[20] = (uint8_t)(1 | (k->spells[j].prep ? 2 : 0));
    }
    for (int j = 0; j < k->nWeapons && j < MAX_WEAPONS; j++)
        put_weapon(o + L.weaponOff + j * L.weaponRec, &k->weapons[j]);
    for (int j = 0; j < k->nFeats && j < MAX_FEATS; j++) {
        uint8_t *q = o + L.featOff + j * FEAT_REC;
        put_str(q, k->feats[j].name, FEAT_NAME_LEN);
        q[20] = k->feats[j].used;
        q[21] = 1;
    }
    put_str(o + L.identOff, k->species, SPECIES_LEN);
    put_str(o + L.identOff + 16, k->background, BACKGROUND_LEN);
}

void save_serialize(const Roster *r, uint8_t last, uint8_t out[SAVE_BYTES])
{
    memset(out, 0, SAVE_BYTES);
    memcpy(out, "PKSH", 4);
    put16(out + 4, SAVE_VERSION);
    out[6] = NUM_SLOTS;
    out[7] = last;
    for (int i = 0; i < NUM_SLOTS; i++)
        if (r->used[i])
            put_record(out + SAVE_HEADER + i * SAVE_RECORD, &r->slot[i]);
    put32(out + 8, crc32_calc(out, SAVE_BYTES));
}

// ---------- read ----------

static bool read_weapon(const uint8_t *q, int ver, Weapon *w)
{
    if (q[0] != 1 || q[13] > 7)
        return false;
    memset(w, 0, sizeof *w);
    get_str(q + 1, WEAPON_NAME_LEN, w->name);
    w->ab = q[13] & 3;
    w->prof = (q[13] >> 2) & 1;
    w->bonus = get_s8(q + 14);
    w->n = q[15];
    w->die = q[16];
    w->type = q[17];
    get_str(q + 18, WEAPON_RANGE_LEN, w->range);
    w->mastery = q[25];
    // v3/v4 had one magic bonus for both to-hit and damage: copy it into the damage bonus.
    w->dmg = ver >= 5 ? get_s8(q + 26) : get_s8(q + 14);
    if (ver >= 5)
        get_str(q + 27, WEAPON_PROPS_LEN, w->props);
    return weapon_valid(w);
}

static ParseStatus read_record(const uint8_t *o, int ver, Character *k)
{
    const Layout L = layout_for(ver);
    memset(k, 0, sizeof *k);
    get_str(o + 1, NAME_LEN, k->name);
    get_str(o + 17, CLASS_LEN, k->cls);
    if (ver >= 2)
        get_str(o + 85, SUBCLASS_LEN, k->sub);
    k->level = o[29];
    memcpy(k->ab, o + 30, 6);
    k->saveProf = o[36];
    memcpy(k->skill, o + 37, SKILL_COUNT);
    k->hp = (int16_t)(get16(o + 55) < 32768 ? (int)get16(o + 55) : (int)get16(o + 55) - 65536);
    k->hpMax = (uint16_t)get16(o + 57);
    k->temp = (uint16_t)get16(o + 59);
    k->ac = o[61];
    k->speed = o[62];
    k->ds = o[63];
    k->df = o[64];
    memcpy(k->slotMax, o + 65, 9);
    memcpy(k->slotUsed, o + 74, 9);
    k->spellAb = o[83];
    k->insp = o[84];
    k->hitDie = (ver >= 3 && o[101]) ? o[101] : 8;
    k->hdUsed = ver >= 3 ? o[102] : 0;
    if (ver >= 3) {
        for (int j = 0; j < MAX_WEAPONS; j++) {
            const uint8_t *q = o + L.weaponOff + j * L.weaponRec;
            if (q[0] == 0)
                continue;
            if (!read_weapon(q, ver, &k->weapons[k->nWeapons]))
                return PARSE_RANGE;
            k->nWeapons++;
        }
    }
    if (ver >= 4) {
        for (int j = 0; j < MAX_SPELLS; j++) {
            const uint8_t *q = o + L.spellOff + j * SPELL_REC;
            uint8_t fl = q[20];
            if (fl == 0)
                continue;
            if (fl > 3 || !(fl & 1))
                return PARSE_RANGE;
            KnownSpell *s = &k->spells[k->nSpells];
            get_str(q, SPELL_NAME_LEN, s->name);
            if (!s->name[0] || !text_name_ok(s->name))
                return PARSE_RANGE;
            s->prep = (fl >> 1) & 1;
            k->nSpells++;
        }
    }
    if (ver >= 5) {
        for (int j = 0; j < MAX_FEATS; j++) {
            const uint8_t *q = o + L.featOff + j * FEAT_REC;
            if (q[21] == 0)
                continue;
            if (q[21] != 1 || q[20] > 20)
                return PARSE_RANGE;
            KnownFeat *f = &k->feats[k->nFeats];
            get_str(q, FEAT_NAME_LEN, f->name);
            if (!f->name[0] || !text_name_ok(f->name))
                return PARSE_RANGE;
            f->used = q[20];
            k->nFeats++;
        }
        get_str(o + L.identOff, SPECIES_LEN, k->species);
        get_str(o + L.identOff + 16, BACKGROUND_LEN, k->background);
    }
    return character_valid(k) ? PARSE_OK : PARSE_RANGE;
}

ParseStatus save_parse(const uint8_t *b, size_t len, Roster *out, uint8_t *last, bool *migrated)
{
    if (!b)
        return PARSE_MISSING;
    if (len < SAVE_HEADER || memcmp(b, "PKSH", 4) != 0)
        return PARSE_MAGIC;
    unsigned ver = get16(b + 4);
    if (ver < 1 || ver > SAVE_VERSION)
        return PARSE_VERSION;
    size_t rec = RECORD_SIZE[ver];
    if (len != SAVE_HEADER + rec * NUM_SLOTS)
        return PARSE_SIZE;
    if (b[6] != NUM_SLOTS)
        return PARSE_SLOTS;

    // CRC over the whole file with the CRC field itself zeroed.
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (i >= 8 && i < 12) ? 0 : b[i];
        for (int k = 0; k < 8; k++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
    }
    if ((crc ^ 0xFFFFFFFFu) != get32(b + 8))
        return PARSE_CHECKSUM;

    // Parsed straight into `out` (a Roster is ~4 KB; the DS stack is small).
    memset(out, 0, sizeof *out);
    for (int i = 0; i < NUM_SLOTS; i++) {
        const uint8_t *o = b + SAVE_HEADER + i * rec;
        if (o[0] == 0)
            continue;
        if (o[0] != 1)
            return PARSE_RANGE;
        ParseStatus s = read_record(o, (int)ver, &out->slot[i]);
        if (s != PARSE_OK)
            return s;
        out->used[i] = true;
    }
    *last = b[7] < NUM_SLOTS ? b[7] : NUM_SLOTS - 1;
    *migrated = ver < SAVE_VERSION;
    return PARSE_OK;
}
