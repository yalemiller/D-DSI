// First-run content: the two sample characters, the library file templates
// and Jedediah's portrait (source/core/demo_files.c is generated).
#pragma once

#include "save.h"

extern const char DEMO_SPELLS_TEMPLATE[];
extern const char DEMO_SPELLS_JED[];
extern const char DEMO_FEATURES_TEMPLATE[];
extern const char DEMO_FEATURES_JED[];
extern const unsigned char DEMO_PORTRAIT1_BMP[6966];

// Slot 1: Jedediah Graves, Ranger 2 (from his paper sheet; HP 20/20 and level 2 inferred).
void demo_jedediah(Character *k);
// Slot 2: Maren Oakholt, Cleric 5.
void demo_maren(Character *k);
// Jedediah, Maren, and an empty third slot.
void demo_roster(Roster *r);
