#include "demo.h"

#include <string.h>

#include "text.h"

static Weapon *add_weapon(Character *k, const char *name)
{
    Weapon *w = &k->weapons[k->nWeapons++];
    weapon_init(w, name);
    return w;
}

static void add_spell(Character *k, const char *name, int prep)
{
    KnownSpell *s = &k->spells[k->nSpells++];
    text_copy(s->name, name, SPELL_NAME_LEN);
    s->prep = (uint8_t)prep;
}

static void add_feat(Character *k, const char *name)
{
    KnownFeat *f = &k->feats[k->nFeats++];
    text_copy(f->name, name, FEAT_NAME_LEN);
    f->used = 0;
}

void demo_jedediah(Character *k)
{
    static const uint8_t ab[6] = { 10, 16, 14, 8, 16, 10 };
    character_init(k, "JEDEDIAH GRAVES");
    text_copy(k->cls, "RANGER", CLASS_LEN);
    text_copy(k->species, "HUMAN (HALF-ELF)", SPECIES_LEN);
    text_copy(k->background, "WAYFARER", BACKGROUND_LEN);
    k->level = 2;
    memcpy(k->ab, ab, 6);
    k->saveProf = 1 | 16;
    k->hp = 20;
    k->hpMax = 20;
    k->ac = 15;
    k->speed = 30;
    k->spellAb = AB_WIS;
    k->hitDie = 10;
    k->skill[3] = k->skill[6] = k->skill[16] = k->skill[17] = 1; // athletics, insight, stealth, survival
    k->skill[15] = 2;                                           // sleight of hand: expertise
    k->slotMax[0] = 2;

    Weapon *w = add_weapon(k, "LUCINDA");
    w->ab = WAB_DEX;
    w->bonus = 2;
    w->dmg = 3;
    w->die = 6;
    w->type = 1;
    w->range[0] = 0;
    w->mastery = MASTERY_VEX;
    text_copy(w->props, "BARREL 6, BLARING, FAN FIRE", WEAPON_PROPS_LEN);

    w = add_weapon(k, "WHIP");
    w->ab = WAB_FINESSE;
    w->dmg = 3;
    w->die = 4;
    w->type = 0;
    text_copy(w->range, "10 FT", WEAPON_RANGE_LEN);
    w->mastery = MASTERY_SLOW;
    text_copy(w->props, "FINESSE, REACH", WEAPON_PROPS_LEN);

    static const char *const spells[] = { "HUNTER'S MARK", "GOODBERRY", "ENSNARING STRIKE", "SILVERED SHELL",
                                          "FIND FAMILIAR" };
    for (size_t i = 0; i < sizeof spells / sizeof spells[0]; i++)
        add_spell(k, spells[i], 1);

    static const char *const feats[] = { "FAVORED ENEMY", "DEFT EXPLORER", "WEAPON MASTERY", "ARCHERY", "LUCKY",
                                         "FAMILIAR FRIEND", "RESOURCEFUL", "BARREL", "BLARING", "FAN FIRE",
                                         "MISFIRE", "LANGUAGES", "TOOLS" };
    for (size_t i = 0; i < sizeof feats / sizeof feats[0]; i++)
        add_feat(k, feats[i]);
}

void demo_maren(Character *k)
{
    static const uint8_t ab[6] = { 14, 10, 14, 10, 17, 12 };
    character_init(k, "MAREN OAKHOLT");
    text_copy(k->cls, "CLERIC", CLASS_LEN);
    text_copy(k->sub, "LIFE DOMAIN", SUBCLASS_LEN);
    k->level = 5;
    memcpy(k->ab, ab, 6);
    k->saveProf = 48;
    k->hp = 31;
    k->hpMax = 38;
    k->ac = 18;
    k->spellAb = AB_WIS;
    k->skill[6] = k->skill[9] = k->skill[13] = k->skill[14] = 1; // insight, medicine, persuasion, religion
    k->slotMax[0] = 4;
    k->slotMax[1] = 3;
    k->slotMax[2] = 2;
    k->slotUsed[0] = 1;

    Weapon *w = add_weapon(k, "MACE");
    w->die = 6;
    w->dmg = 2;
    w->type = 2;
    w->mastery = MASTERY_SAP;

    w = add_weapon(k, "LT CROSSBOW");
    w->ab = WAB_DEX;
    w->type = 1;
    text_copy(w->range, "80/320", WEAPON_RANGE_LEN);
    w->mastery = MASTERY_SLOW;
    text_copy(w->props, "AMMUNITION, LOADING", WEAPON_PROPS_LEN);

    w = add_weapon(k, "WARHAMMER");
    w->die = 8;
    w->dmg = 2;
    w->type = 2;
    w->prof = 0;
    w->mastery = MASTERY_PUSH;
    text_copy(w->props, "VERSATILE (1D10)", WEAPON_PROPS_LEN);

    add_feat(k, "CHANNEL DIVINITY");
    k->hdUsed = 1;

    // A trailing * marks a prepared spell, as in the prototype.
    static const char *const spells[] = { "SACRED FLAME", "GUIDANCE", "SPARE THE DYING", "THAUMATURGY", "BLESS*",
                                          "CURE WOUNDS*", "GUIDING BOLT*", "HEALING WORD*", "SHIELD OF FAITH",
                                          "DETECT MAGIC", "AID*", "LESSER RESTORATION*", "SPIRITUAL WEAPON*",
                                          "HOLD PERSON", "MASS HEALING WORD", "REVIVIFY*", "SPIRIT GUARDIANS*" };
    for (size_t i = 0; i < sizeof spells / sizeof spells[0]; i++) {
        char name[SPELL_NAME_LEN + 2];
        text_copy(name, spells[i], SPELL_NAME_LEN + 1);
        size_t n = strlen(name);
        int prep = n && name[n - 1] == '*';
        if (prep)
            name[n - 1] = 0;
        add_spell(k, name, prep);
    }
}

void demo_roster(Roster *r)
{
    memset(r, 0, sizeof *r);
    demo_jedediah(&r->slot[0]);
    demo_maren(&r->slot[1]);
    r->used[0] = r->used[1] = true;
}
