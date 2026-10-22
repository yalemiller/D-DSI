// Sheet actions: the rules behind each button, separate from how the UI
// reports them. Each returns what happened so the UI can pick the copy and
// effects; none of them touch storage.
#pragma once

#include <stdbool.h>

#include "character.h"
#include "features.h"
#include "spells.h"

typedef struct {
    int before, after; // HP
    int absorbed;      // taken by temp HP
} HpChange;

// Temp HP absorbs damage first.
HpChange act_damage(Character *k, int n);
// Healing from 0 HP clears death saves.
HpChange act_heal(Character *k, int n);
// Temp HP doesn't stack: keeps the higher value. Returns true if it went up.
bool act_temp_hp(Character *k, int n, int *old);

// Long rest: full HP, all slots and hit dice, every feature use except MANUAL
// ones; temp HP and death saves cleared; inspiration set if any feature on the
// sheet has `inspire`.
void act_long_rest(Character *k, const FeatureLib *lib);
// Short rest: spends `dice` hit dice and regains `hp` (capped at max);
// SHORT trackers reset, SHORT1 trackers get one use back.
HpChange act_short_rest(Character *k, const FeatureLib *lib, int dice, int hp);
// n x ((die + 1) / 2 + CON mod), at least 0.
int act_short_rest_average(const Character *k, int dice);

// Spend (+1) or restore (-1) a slot of level index 0..8. False when nothing changes.
bool act_slot(Character *k, int level, int delta);
// Sets a level's maximum (0..4), clamping slots already used.
void act_set_slot_max(Character *k, int level, int max);

typedef enum { FEAT_USED, FEAT_RESTORED, FEAT_NONE_LEFT, FEAT_FULL, FEAT_UNTRACKED } FeatUse;
// delta +1 spends a use, -1 gives one back.
FeatUse act_feat_use(Character *k, const FeatureLib *lib, const char *name, int delta);

typedef enum { PREP_ON, PREP_OFF, PREP_CANTRIP, PREP_UNKNOWN } PrepResult;
PrepResult act_toggle_prepared(Character *k, const SpellLib *lib, const char *name);

typedef enum { PICK_ADDED, PICK_REMOVED, PICK_FULL } PickResult;
// Adds the spell if it isn't on the sheet, removes it if it is.
PickResult act_toggle_spell(Character *k, const char *name);
PickResult act_toggle_feat(Character *k, const char *name);
bool act_remove_spell(Character *k, const char *name);
bool act_remove_feat(Character *k, const char *name);

// Appends a new weapon (fails at MAX_WEAPONS). Returns its index or -1.
int act_add_weapon(Character *k, const char *name);
void act_delete_weapon(Character *k, int idx);

// Level and max HP edits keep the dependent numbers legal.
void act_set_level(Character *k, int level);
void act_set_hp_max(Character *k, int max);
