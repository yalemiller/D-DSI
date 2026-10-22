// Spell library: the built-in starter spells plus fat:/data/pocketsheet/spells.txt
// (edited on a PC). Characters store spells by name; this is where names resolve.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SPELL_FIELD_LEN 20
#define SPELL_DESC_MAX 1500
#define SPELL_FILE_MAX 300
#define SPELL_LIB_MAX (64 + SPELL_FILE_MAX)

// A compiled-in entry (source/core/builtin_spells.c is generated).
typedef struct {
    const char *name;
    int8_t level; // 0 = cantrip
    const char *school, *time, *range, *comp, *dur;
    uint8_t conc, ritual;
    const char *desc;
} SpellDef;

extern const SpellDef SPELL_BUILTINS[];
extern const int SPELL_BUILTIN_COUNT;

typedef struct {
    char name[SPELL_FIELD_LEN + 1];
    int8_t level;
    char school[SPELL_FIELD_LEN + 1];
    char time[SPELL_FIELD_LEN + 1];
    char range[SPELL_FIELD_LEN + 1];
    char comp[SPELL_FIELD_LEN + 1];
    char dur[SPELL_FIELD_LEN + 1];
    bool conc, ritual;
    bool fromFile;
    const char *desc; // points at `owned` or at a built-in string
    char *owned;
} Spell;

typedef struct {
    Spell items[SPELL_LIB_MAX];
    int count;
} SpellLib;

typedef struct {
    int added;   // valid entries read from the file (at most SPELL_FILE_MAX)
    int skipped; // invalid entries
} LibParseResult;

// Loads the built-ins (and frees anything parsed before).
void spells_init(SpellLib *lib);
void spells_free(SpellLib *lib);
const Spell *spells_find(const SpellLib *lib, const char *name);

// Parses spells.txt text and merges it into the library: an entry with the
// same name as an existing one replaces it.
LibParseResult spells_parse(SpellLib *lib, const char *text, size_t len);
