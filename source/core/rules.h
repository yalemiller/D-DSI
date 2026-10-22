// 5e-style rules math (SRD 5.1/5.2 terms). Pure functions of a Character.
#pragma once

#include <stddef.h>

#include "character.h"

// floor((score - 10) / 2)
int rules_mod(int score);
// 2 + floor((level - 1) / 4)
int rules_pb(const Character *k);
// ability mod + prof x (0/1/2)
int rules_skill(const Character *k, int skill);
// ability mod + prof if proficient
int rules_save(const Character *k, int ability);
// 10 + Perception
int rules_passive_perception(const Character *k);
// 8 + prof + spell ability mod (call only with a spell ability set)
int rules_spell_dc(const Character *k);
// prof + spell ability mod
int rules_spell_attack(const Character *k);

// The ability a weapon uses: STR, DEX, the better of the two for FINESSE,
// or the spell ability for SPELL (-1 when the character has none).
int rules_weapon_ability(const Character *k, const Weapon *w);
int rules_weapon_mod(const Character *k, const Weapon *w);
// ability mod + prof (if proficient) + hit bonus
int rules_weapon_to_hit(const Character *k, const Weapon *w);
// "1D6+3": the dice plus the damage bonus exactly as entered (no ability mod).
char *rules_weapon_damage(const Weapon *w, char *buf, size_t cap);
// True when the card shows the arrow icon: range has '/' or props mention AMMUNITION or BARREL.
int rules_weapon_is_ranged(const Weapon *w);

// GRAZE damage = max(0, ability mod); TOPPLE DC = 8 + ability mod + prof.
int rules_graze(const Character *k, const Weapon *w);
int rules_topple_dc(const Character *k, const Weapon *w);
// "TOPPLE DC 13", "GRAZE 3", or the mastery name.
char *rules_mastery_tag(const Character *k, const Weapon *w, char *buf, size_t cap);
// One-line effect (<= 42 chars) for the status strip.
char *rules_mastery_line(const Character *k, const Weapon *w, char *buf, size_t cap);
// Full effect text for the details card ("\n" separates paragraphs).
char *rules_mastery_desc(const Character *k, const Weapon *w, char *buf, size_t cap);
