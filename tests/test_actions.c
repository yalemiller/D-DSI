#include "core/actions.h"
#include "core/demo.h"
#include "test.h"
#include "tests.h"

static SpellLib slib;
static FeatureLib flib;

void test_actions(void)
{
    Character k;
    spells_init(&slib);
    features_init(&flib);
    features_parse(&flib, DEMO_FEATURES_JED, strlen(DEMO_FEATURES_JED));

    // damage, temp HP, healing
    demo_jedediah(&k);
    k.temp = 3;
    HpChange c = act_damage(&k, 5);
    CHECK_INT(c.absorbed, 3);
    CHECK_INT(c.before, 20);
    CHECK_INT(c.after, 18);
    CHECK_INT(k.temp, 0);
    c = act_damage(&k, 50);
    CHECK_INT(k.hp, 0);
    k.ds = 2;
    k.df = 1;
    c = act_heal(&k, 4);
    CHECK_INT(c.before, 0);
    CHECK_INT(k.hp, 4);
    CHECK_INT(k.ds + k.df, 0); // healing from 0 clears death saves
    act_heal(&k, 100);
    CHECK_INT(k.hp, 20);
    int old;
    CHECK(act_temp_hp(&k, 5, &old));
    CHECK_INT(old, 0);
    CHECK(!act_temp_hp(&k, 3, &old)); // doesn't stack
    CHECK_INT(k.temp, 5);

    // long rest: MANUAL trackers keep their count; RESOURCEFUL grants inspiration
    int barrel = character_find_feat(&k, "BARREL"), lucky = character_find_feat(&k, "LUCKY");
    k.feats[barrel].used = 3;
    k.feats[lucky].used = 1;
    k.hp = 3;
    k.slotUsed[0] = 2;
    k.hdUsed = 2;
    k.insp = 0;
    act_long_rest(&k, &flib);
    CHECK_INT(k.hp, 20);
    CHECK_INT(k.temp, 0);
    CHECK_INT(k.slotUsed[0], 0);
    CHECK_INT(k.hdUsed, 0);
    CHECK_INT(k.feats[barrel].used, 3);
    CHECK_INT(k.feats[lucky].used, 0);
    CHECK_INT(k.insp, 1);
    // a feature missing from the library resets like a normal one
    FeatureLib *bare = &flib;
    features_init(bare);
    k.feats[barrel].used = 3;
    act_long_rest(&k, bare);
    CHECK_INT(k.feats[barrel].used, 0);

    // short rest
    demo_maren(&k);
    int cd = character_find_feat(&k, "CHANNEL DIVINITY");
    k.feats[cd].used = 2;
    CHECK_INT(act_short_rest_average(&k, 2), 13); // 2 x (4.5 + 2)
    c = act_short_rest(&k, &flib, 2, 10);
    CHECK_INT(c.before, 31);
    CHECK_INT(c.after, 38);
    CHECK_INT(k.hdUsed, 3);
    CHECK_INT(k.feats[cd].used, 1); // SHORT1: one use back
    k.hitDie = 6;
    k.ab[AB_CON] = 3;
    CHECK_INT(act_short_rest_average(&k, 1), 0); // 3.5 - 4 rounds down, then floors at 0

    // spell slots
    demo_jedediah(&k);
    CHECK(act_slot(&k, 0, 1));
    CHECK(act_slot(&k, 0, 1));
    CHECK(!act_slot(&k, 0, 1));
    CHECK_INT(k.slotUsed[0], 2);
    CHECK(act_slot(&k, 0, -1));
    CHECK(!act_slot(&k, 1, -1));
    k.slotUsed[0] = 2;
    act_set_slot_max(&k, 0, 1);
    CHECK_INT(k.slotUsed[0], 1);
    CHECK_INT(k.slotMax[0], 1);

    // feature uses (LUCKY = proficiency bonus = 2)
    features_init(&flib);
    CHECK_INT(act_feat_use(&k, &flib, "LUCKY", 1), FEAT_USED);
    CHECK_INT(act_feat_use(&k, &flib, "LUCKY", 1), FEAT_USED);
    CHECK_INT(act_feat_use(&k, &flib, "LUCKY", 1), FEAT_NONE_LEFT);
    CHECK_INT(act_feat_use(&k, &flib, "LUCKY", -1), FEAT_RESTORED);
    CHECK_INT(act_feat_use(&k, &flib, "LUCKY", -1), FEAT_RESTORED);
    CHECK_INT(act_feat_use(&k, &flib, "LUCKY", -1), FEAT_FULL);
    CHECK_INT(act_feat_use(&k, &flib, "ARCHERY", 1), FEAT_UNTRACKED);
    CHECK_INT(act_feat_use(&k, &flib, "NOT ON SHEET", 1), FEAT_UNTRACKED);

    // prepared spells
    CHECK_INT(act_toggle_prepared(&k, &slib, "HUNTER'S MARK"), PREP_OFF);
    CHECK_INT(act_toggle_prepared(&k, &slib, "HUNTER'S MARK"), PREP_ON);
    CHECK_INT(act_toggle_prepared(&k, &slib, "SILVERED SHELL"), PREP_OFF); // not in this library: still toggles
    CHECK_INT(act_toggle_prepared(&k, &slib, "FIREBALL"), PREP_UNKNOWN);
    demo_maren(&k);
    CHECK_INT(act_toggle_prepared(&k, &slib, "GUIDANCE"), PREP_CANTRIP);

    // adding and removing spells and features
    demo_jedediah(&k);
    CHECK_INT(act_toggle_spell(&k, "FIREBALL"), PICK_ADDED);
    CHECK_INT(k.nSpells, 6);
    CHECK_INT(k.spells[5].prep, 0);
    CHECK_INT(act_toggle_spell(&k, "GOODBERRY"), PICK_REMOVED);
    CHECK_INT(k.nSpells, 5);
    CHECK_STR(k.spells[1].name, "ENSNARING STRIKE"); // order kept
    for (int i = 0; k.nSpells < MAX_SPELLS; i++) {
        char name[8];
        snprintf(name, sizeof name, "X%d", i);
        CHECK_INT(act_toggle_spell(&k, name), PICK_ADDED);
    }
    CHECK_INT(act_toggle_spell(&k, "ONE MORE"), PICK_FULL);
    CHECK(act_remove_spell(&k, "FIREBALL"));
    CHECK(!act_remove_spell(&k, "FIREBALL"));
    CHECK_INT(act_toggle_feat(&k, "LUCKY"), PICK_REMOVED);
    CHECK_INT(act_toggle_feat(&k, "RAGE"), PICK_ADDED);
    CHECK_INT(act_toggle_feat(&k, "SECOND WIND"), PICK_ADDED);
    CHECK_INT(act_toggle_feat(&k, "SKILLFUL"), PICK_ADDED);
    CHECK_INT(act_toggle_feat(&k, "BARDIC INSPIRATION"), PICK_ADDED);
    CHECK_INT(k.nFeats, 16);
    CHECK_INT(act_toggle_feat(&k, "LUCKY"), PICK_FULL);
    CHECK(character_valid(&k));

    // weapons
    demo_jedediah(&k);
    CHECK_INT(act_add_weapon(&k, "DAGGER"), 2);
    CHECK_INT(act_add_weapon(&k, "SLING"), 3);
    CHECK_INT(act_add_weapon(&k, "AXE"), -1);
    CHECK_STR(k.weapons[2].range, "5 FT");
    act_delete_weapon(&k, 0);
    CHECK_INT(k.nWeapons, 3);
    CHECK_STR(k.weapons[0].name, "WHIP");
    CHECK_STR(k.weapons[2].name, "SLING");
    CHECK(character_valid(&k));

    // level and max HP edits
    k.hdUsed = 2;
    act_set_level(&k, 1);
    CHECK_INT(k.hdUsed, 1);
    CHECK_INT(k.hp, 20);
    act_set_hp_max(&k, 25); // at full HP, current follows the max
    CHECK_INT(k.hp, 25);
    k.hp = 10;
    act_set_hp_max(&k, 8);
    CHECK_INT(k.hp, 8);
    CHECK_INT(k.hpMax, 8);
    CHECK(character_valid(&k));

    spells_free(&slib);
    features_free(&flib);
}
