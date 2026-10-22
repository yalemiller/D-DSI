#include "core/demo.h"
#include "core/rules.h"
#include "test.h"
#include "tests.h"

void test_rules(void)
{
    static const int MOD[][2] = { { 1, -5 }, { 2, -4 }, { 3, -4 }, { 8, -1 }, { 9, -1 }, { 10, 0 },
                                  { 11, 0 }, { 12, 1 }, { 17, 3 }, { 30, 10 } };
    for (size_t i = 0; i < sizeof MOD / sizeof MOD[0]; i++)
        CHECK_INT(rules_mod(MOD[i][0]), MOD[i][1]);

    Character k;
    static const int PB[][2] = { { 1, 2 }, { 4, 2 }, { 5, 3 }, { 8, 3 }, { 9, 4 }, { 13, 5 }, { 17, 6 }, { 20, 6 } };
    character_init(&k, "TEST");
    for (size_t i = 0; i < sizeof PB / sizeof PB[0]; i++) {
        k.level = (uint8_t)PB[i][0];
        CHECK_INT(rules_pb(&k), PB[i][1]);
    }

    // Jedediah Graves, Ranger 2: the numbers on his paper sheet.
    char buf[400];
    demo_jedediah(&k);
    CHECK(character_valid(&k));
    CHECK_INT(rules_pb(&k), 2);
    CHECK_INT(rules_skill(&k, 3), 2);   // athletics (STR, proficient)
    CHECK_INT(rules_skill(&k, 6), 5);   // insight
    CHECK_INT(rules_skill(&k, 15), 7);  // sleight of hand: DEX +3 + expertise +4
    CHECK_INT(rules_skill(&k, 0), 3);   // acrobatics, not proficient
    CHECK_INT(rules_passive_perception(&k), 13);
    CHECK_INT(rules_save(&k, AB_STR), 2);
    CHECK_INT(rules_save(&k, AB_DEX), 3);
    CHECK_INT(rules_save(&k, AB_INT), -1);
    CHECK_INT(rules_save(&k, AB_WIS), 5);
    CHECK_INT(rules_spell_dc(&k), 13);
    CHECK_INT(rules_spell_attack(&k), 5);

    const Weapon *lucinda = &k.weapons[0], *whip = &k.weapons[1];
    CHECK_INT(rules_weapon_to_hit(&k, lucinda), 7); // DEX +3, prof +2, Archery +2
    CHECK_STR(rules_weapon_damage(lucinda, buf, sizeof buf), "1D6+3");
    CHECK(rules_weapon_is_ranged(lucinda)); // BARREL
    CHECK_INT(rules_weapon_ability(&k, whip), AB_DEX); // finesse picks the better of STR/DEX
    CHECK_INT(rules_weapon_to_hit(&k, whip), 5);
    CHECK_STR(rules_weapon_damage(whip, buf, sizeof buf), "1D4+3");
    CHECK(!rules_weapon_is_ranged(whip));
    CHECK_STR(rules_mastery_tag(&k, whip, buf, sizeof buf), "SLOW");
    CHECK_STR(rules_mastery_line(&k, whip, buf, sizeof buf), "SLOW: ON HIT, TARGET SPEED -10 FT");

    Weapon w = *whip;
    w.mastery = MASTERY_GRAZE;
    CHECK_STR(rules_mastery_tag(&k, &w, buf, sizeof buf), "GRAZE 3");
    CHECK_STR(rules_mastery_line(&k, &w, buf, sizeof buf), "GRAZE: ON A MISS, STILL DEAL 3 DAMAGE");
    w.mastery = MASTERY_TOPPLE;
    CHECK_STR(rules_mastery_tag(&k, &w, buf, sizeof buf), "TOPPLE DC 13");
    rules_mastery_desc(&k, &w, buf, sizeof buf);
    CHECK(strstr(buf, "DC 13 (8 + ABILITY MOD + PROF)") != NULL);
    for (int m = 1; m < MASTERY_COUNT; m++) {
        w.mastery = (uint8_t)m;
        CHECK(strlen(rules_mastery_line(&k, &w, buf, sizeof buf)) <= 42);
    }
    w.ab = WAB_SPELL;
    CHECK_INT(rules_weapon_ability(&k, &w), AB_WIS);
    k.spellAb = SPELL_AB_NONE;
    CHECK_INT(rules_weapon_ability(&k, &w), -1);
    CHECK_INT(rules_weapon_mod(&k, &w), 0);
    w.dmg = -2;
    CHECK_STR(rules_weapon_damage(&w, buf, sizeof buf), "1D4-2");
    w.dmg = 0;
    CHECK_STR(rules_weapon_damage(&w, buf, sizeof buf), "1D4");

    // Maren Oakholt, Cleric 5.
    demo_maren(&k);
    CHECK(character_valid(&k));
    CHECK_INT(rules_pb(&k), 3);
    CHECK_INT(rules_spell_dc(&k), 14);
    CHECK_INT(rules_spell_attack(&k), 6);
    CHECK_INT(rules_save(&k, AB_WIS), 6);
    CHECK_INT(rules_save(&k, AB_CHA), 4);
    CHECK_INT(rules_skill(&k, 13), 4); // persuasion
    CHECK_INT(rules_weapon_to_hit(&k, &k.weapons[0]), 5);
    CHECK_STR(rules_weapon_damage(&k.weapons[0], buf, sizeof buf), "1D6+2");
    CHECK_INT(rules_weapon_to_hit(&k, &k.weapons[1]), 3);
    CHECK(rules_weapon_is_ranged(&k.weapons[1]));
    CHECK_INT(rules_weapon_to_hit(&k, &k.weapons[2]), 2); // not proficient
    CHECK_STR(rules_mastery_tag(&k, &k.weapons[2], buf, sizeof buf), "PUSH");
}
