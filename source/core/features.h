// Feature library: class features, feats, species traits, item properties and
// notes. Built-ins plus fat:/data/pocketsheet/features.txt. Features can track
// uses (a fixed number, the proficiency bonus, or an ability modifier) that
// reset on rests.
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "character.h"
#include "spells.h" // LibParseResult

#define FEAT_DESC_MAX 1200
#define FEAT_FILE_MAX 200
#define FEAT_LIB_MAX (32 + FEAT_FILE_MAX)
#define FCAT_COUNT 7

enum { FCAT_CLASS, FCAT_SUBCLASS, FCAT_SPECIES, FCAT_BACKGROUND, FCAT_FEAT, FCAT_ITEM, FCAT_OTHER };
enum { USES_NONE, USES_FIXED, USES_PB, USES_ABILITY };
// RESET_UNSET: no tracker. RESET_MANUAL ("none" in the file): the player resets it.
enum { RESET_UNSET, RESET_LONG, RESET_SHORT, RESET_SHORT1, RESET_MANUAL };

extern const char *const FCAT_NAMES[FCAT_COUNT];  // CLASS ... OTHER
extern const char *const FCAT_SHORT[FCAT_COUNT];  // CLS ... OTH
extern const char *const RESET_NAMES[5];          // "", LONG REST, SHORT REST, 1 PER SHORT, MANUAL

typedef struct {
    const char *name;
    uint8_t cat;
    uint8_t usesKind, usesVal; // USES_FIXED: count; USES_ABILITY: AB_*
    uint8_t reset;
    uint8_t inspire;
    const char *desc;
} FeatureDef;

extern const FeatureDef FEATURE_BUILTINS[];
extern const int FEATURE_BUILTIN_COUNT;

typedef struct {
    char name[FEAT_NAME_LEN + 1];
    uint8_t cat;
    uint8_t usesKind, usesVal;
    uint8_t reset;
    bool inspire;
    bool fromFile;
    const char *desc;
    char *owned;
} Feature;

typedef struct {
    Feature items[FEAT_LIB_MAX];
    int count;
} FeatureLib;

void features_init(FeatureLib *lib);
void features_free(FeatureLib *lib);
const Feature *features_find(const FeatureLib *lib, const char *name);
LibParseResult features_parse(FeatureLib *lib, const char *text, size_t len);

// Uses per reset for this character (0 = not tracked). PB = proficiency bonus,
// an ability = max(1, its modifier), a number is capped at 20.
int features_max_uses(const Character *k, const Feature *f);

// The library key for a weapon property: "BARREL 6" -> "BARREL",
// "VERSATILE (1D10)" -> "VERSATILE". `out` holds WEAPON_PROPS_LEN + 1.
void features_property_key(const char *prop, size_t len, char *out);
