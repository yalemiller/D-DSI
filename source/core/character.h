// The character model: one sheet, its weapons, known spells and features,
// plus the fixed name tables the rules and the UI share.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#define NUM_SLOTS 3
#define MAX_WEAPONS 4
#define MAX_SPELLS 24
#define MAX_FEATS 16

#define NAME_LEN 16
#define CLASS_LEN 12
#define SUBCLASS_LEN 16
#define SPECIES_LEN 16
#define BACKGROUND_LEN 16
#define WEAPON_NAME_LEN 12
#define WEAPON_RANGE_LEN 7
#define WEAPON_PROPS_LEN 28
#define SPELL_NAME_LEN 20
#define FEAT_NAME_LEN 20

#define SKILL_COUNT 18
#define SKILL_PERCEPTION 11
#define DTYPE_COUNT 13
#define MASTERY_COUNT 9
#define SPELL_AB_NONE 255

enum { AB_STR, AB_DEX, AB_CON, AB_INT, AB_WIS, AB_CHA };
enum { WAB_STR, WAB_DEX, WAB_FINESSE, WAB_SPELL };
enum { MASTERY_NONE, MASTERY_CLEAVE, MASTERY_GRAZE, MASTERY_NICK, MASTERY_PUSH, MASTERY_SAP, MASTERY_SLOW,
       MASTERY_TOPPLE, MASTERY_VEX };

typedef struct {
    char name[WEAPON_NAME_LEN + 1];
    uint8_t ab;      // WAB_*
    uint8_t prof;    // 0/1
    int8_t bonus;    // to-hit bonus, -5..10
    int8_t dmg;      // damage bonus added to the dice, -10..30
    uint8_t n;       // number of dice, 1..9
    uint8_t die;     // 4, 6, 8, 10, 12
    uint8_t type;    // index into DTYPE_NAMES
    char range[WEAPON_RANGE_LEN + 1];
    uint8_t mastery; // MASTERY_*
    char props[WEAPON_PROPS_LEN + 1];
} Weapon;

typedef struct {
    char name[SPELL_NAME_LEN + 1];
    uint8_t prep;
} KnownSpell;

typedef struct {
    char name[FEAT_NAME_LEN + 1];
    uint8_t used;
} KnownFeat;

typedef struct {
    char name[NAME_LEN + 1];
    char cls[CLASS_LEN + 1];
    char sub[SUBCLASS_LEN + 1];
    char species[SPECIES_LEN + 1];
    char background[BACKGROUND_LEN + 1];
    uint8_t level;
    uint8_t ab[6];
    uint8_t saveProf; // bit i = proficient in save i
    uint8_t skill[SKILL_COUNT]; // 0 none, 1 proficient, 2 expertise
    int16_t hp;
    uint16_t hpMax;
    uint16_t temp;
    uint8_t ac;
    uint8_t speed;
    uint8_t ds; // death save successes
    uint8_t df; // death save failures
    uint8_t slotMax[9];
    uint8_t slotUsed[9];
    uint8_t spellAb; // AB_*, or SPELL_AB_NONE
    uint8_t insp;
    uint8_t hitDie;
    uint8_t hdUsed;
    uint8_t nWeapons;
    Weapon weapons[MAX_WEAPONS];
    uint8_t nSpells;
    KnownSpell spells[MAX_SPELLS];
    uint8_t nFeats;
    KnownFeat feats[MAX_FEATS];
} Character;

typedef struct {
    const char *name;
    uint8_t ability;
} SkillDef;

extern const char *const AB_NAMES[6];      // STR ... CHA
extern const char *const AB_FULL[6];       // STRENGTH ... CHARISMA
extern const char *const ORD_NAMES[9];     // 1ST ... 9TH
extern const SkillDef SKILLS[SKILL_COUNT]; // alphabetical, as stored in the save
extern const char *const DTYPE_NAMES[DTYPE_COUNT];
extern const char *const DTYPE_SHORT[DTYPE_COUNT];
extern const char *const MASTERY_NAMES[MASTERY_COUNT];
extern const char *const WAB_NAMES[4];
extern const uint8_t DIE_SIZES[5];
extern const uint8_t HIT_DIE_SIZES[4];

// newChar(): a blank level 1 adventurer.
void character_init(Character *k, const char *name);
// newWeapon(): 1D8 slashing, STR, proficient, 5 FT.
void weapon_init(Weapon *w, const char *name);

// Every range and charset check the save format requires (valid() in the prototype).
bool character_valid(const Character *k);
bool weapon_valid(const Weapon *w);

int character_find_spell(const Character *k, const char *name);
int character_find_feat(const Character *k, const char *name);
