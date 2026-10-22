#include "character.h"

#include <string.h>

#include "text.h"

const char *const AB_NAMES[6] = { "STR", "DEX", "CON", "INT", "WIS", "CHA" };
const char *const AB_FULL[6] = { "STRENGTH", "DEXTERITY", "CONSTITUTION", "INTELLIGENCE", "WISDOM", "CHARISMA" };
const char *const ORD_NAMES[9] = { "1ST", "2ND", "3RD", "4TH", "5TH", "6TH", "7TH", "8TH", "9TH" };

const SkillDef SKILLS[SKILL_COUNT] = {
    { "ACROBATICS", AB_DEX },    { "ANIMAL HANDLING", AB_WIS }, { "ARCANA", AB_INT },
    { "ATHLETICS", AB_STR },     { "DECEPTION", AB_CHA },       { "HISTORY", AB_INT },
    { "INSIGHT", AB_WIS },       { "INTIMIDATION", AB_CHA },    { "INVESTIGATION", AB_INT },
    { "MEDICINE", AB_WIS },      { "NATURE", AB_INT },          { "PERCEPTION", AB_WIS },
    { "PERFORMANCE", AB_CHA },   { "PERSUASION", AB_CHA },      { "RELIGION", AB_INT },
    { "SLEIGHT OF HAND", AB_DEX }, { "STEALTH", AB_DEX },       { "SURVIVAL", AB_WIS },
};

const char *const DTYPE_NAMES[DTYPE_COUNT] = { "SLASHING", "PIERCING", "BLUDGEONING", "FIRE", "COLD",
                                               "LIGHTNING", "THUNDER", "ACID", "POISON", "NECROTIC",
                                               "RADIANT", "FORCE", "PSYCHIC" };
const char *const DTYPE_SHORT[DTYPE_COUNT] = { "SLASH", "PIERCE", "BLUDGEON", "FIRE", "COLD",
                                               "LIGHTNING", "THUNDER", "ACID", "POISON", "NECROTIC",
                                               "RADIANT", "FORCE", "PSYCHIC" };
const char *const MASTERY_NAMES[MASTERY_COUNT] = { "NONE", "CLEAVE", "GRAZE", "NICK", "PUSH",
                                                   "SAP", "SLOW", "TOPPLE", "VEX" };
const char *const WAB_NAMES[4] = { "STR", "DEX", "FINESSE", "SPELL" };
const uint8_t DIE_SIZES[5] = { 4, 6, 8, 10, 12 };
const uint8_t HIT_DIE_SIZES[4] = { 6, 8, 10, 12 };

void character_init(Character *k, const char *name)
{
    memset(k, 0, sizeof *k);
    text_copy(k->name, name, NAME_LEN);
    text_copy(k->cls, "ADVENTURER", CLASS_LEN);
    k->level = 1;
    for (int i = 0; i < 6; i++)
        k->ab[i] = 10;
    k->hp = 10;
    k->hpMax = 10;
    k->ac = 10;
    k->speed = 30;
    k->spellAb = SPELL_AB_NONE;
    k->hitDie = 8;
}

void weapon_init(Weapon *w, const char *name)
{
    memset(w, 0, sizeof *w);
    text_copy(w->name, name, WEAPON_NAME_LEN);
    w->ab = WAB_STR;
    w->prof = 1;
    w->n = 1;
    w->die = 8;
    w->type = 0;
    text_copy(w->range, "5 FT", WEAPON_RANGE_LEN);
}

static bool in_list(uint8_t v, const uint8_t *list, int n)
{
    for (int i = 0; i < n; i++)
        if (list[i] == v)
            return true;
    return false;
}

bool weapon_valid(const Weapon *w)
{
    return w->name[0] && text_name_ok(w->name) && text_name_ok(w->range) && text_props_ok(w->props) &&
           w->ab <= 3 && w->prof <= 1 && w->dmg >= -10 && w->dmg <= 30 && w->bonus >= -5 && w->bonus <= 10 &&
           w->n >= 1 && w->n <= 9 && in_list(w->die, DIE_SIZES, 5) && w->type < DTYPE_COUNT &&
           w->mastery < MASTERY_COUNT;
}

bool character_valid(const Character *k)
{
    if (!k->name[0] || !text_name_ok(k->name) || !text_name_ok(k->cls) || !text_name_ok(k->sub) ||
        !text_name_ok(k->species) || !text_name_ok(k->background))
        return false;
    if (k->level < 1 || k->level > 20 || k->saveProf >= 64)
        return false;
    for (int i = 0; i < 6; i++)
        if (k->ab[i] < 1 || k->ab[i] > 30)
            return false;
    for (int i = 0; i < SKILL_COUNT; i++)
        if (k->skill[i] > 2)
            return false;
    if (k->hpMax < 1 || k->hpMax > 999 || k->hp < 0 || k->hp > (int)k->hpMax || k->temp > 999)
        return false;
    if (k->ac > 40 || k->speed > 120 || k->ds > 3 || k->df > 3)
        return false;
    for (int i = 0; i < 9; i++)
        if (k->slotMax[i] > 4 || k->slotUsed[i] > k->slotMax[i])
            return false;
    if (k->spellAb > 5 && k->spellAb != SPELL_AB_NONE)
        return false;
    if (k->insp > 1 || !in_list(k->hitDie, HIT_DIE_SIZES, 4) || k->hdUsed > k->level)
        return false;
    if (k->nWeapons > MAX_WEAPONS || k->nSpells > MAX_SPELLS || k->nFeats > MAX_FEATS)
        return false;
    for (int i = 0; i < k->nWeapons; i++)
        if (!weapon_valid(&k->weapons[i]))
            return false;
    for (int i = 0; i < k->nSpells; i++) {
        if (!k->spells[i].name[0] || !text_name_ok(k->spells[i].name) || k->spells[i].prep > 1)
            return false;
        for (int j = 0; j < i; j++)
            if (!strcmp(k->spells[i].name, k->spells[j].name))
                return false;
    }
    for (int i = 0; i < k->nFeats; i++) {
        if (!k->feats[i].name[0] || !text_name_ok(k->feats[i].name) || k->feats[i].used > 20)
            return false;
        for (int j = 0; j < i; j++)
            if (!strcmp(k->feats[i].name, k->feats[j].name))
                return false;
    }
    return true;
}

int character_find_spell(const Character *k, const char *name)
{
    for (int i = 0; i < k->nSpells; i++)
        if (!strcmp(k->spells[i].name, name))
            return i;
    return -1;
}

int character_find_feat(const Character *k, const char *name)
{
    for (int i = 0; i < k->nFeats; i++)
        if (!strcmp(k->feats[i].name, name))
            return i;
    return -1;
}
