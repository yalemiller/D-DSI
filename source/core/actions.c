#include "actions.h"

#include <string.h>

#include "rules.h"
#include "text.h"

HpChange act_damage(Character *k, int n)
{
    HpChange c;
    c.absorbed = n < (int)k->temp ? n : (int)k->temp;
    k->temp = (uint16_t)(k->temp - c.absorbed);
    c.before = k->hp;
    int hp = k->hp - (n - c.absorbed);
    k->hp = (int16_t)(hp > 0 ? hp : 0);
    c.after = k->hp;
    return c;
}

HpChange act_heal(Character *k, int n)
{
    HpChange c = { k->hp, 0, 0 };
    int hp = k->hp + n;
    k->hp = (int16_t)(hp < (int)k->hpMax ? hp : (int)k->hpMax);
    if (c.before == 0 && k->hp > 0)
        k->ds = k->df = 0;
    c.after = k->hp;
    return c;
}

bool act_temp_hp(Character *k, int n, int *old)
{
    *old = k->temp;
    if (n > (int)k->temp) {
        k->temp = (uint16_t)n;
        return true;
    }
    return false;
}

void act_long_rest(Character *k, const FeatureLib *lib)
{
    k->hp = (int16_t)k->hpMax;
    k->temp = 0;
    k->ds = k->df = 0;
    memset(k->slotUsed, 0, sizeof k->slotUsed);
    k->hdUsed = 0;
    bool inspire = false;
    for (int i = 0; i < k->nFeats; i++) {
        const Feature *f = features_find(lib, k->feats[i].name);
        if (!f || f->reset != RESET_MANUAL)
            k->feats[i].used = 0;
        if (f && f->inspire)
            inspire = true;
    }
    if (inspire)
        k->insp = 1;
}

HpChange act_short_rest(Character *k, const FeatureLib *lib, int dice, int hp)
{
    HpChange c = { k->hp, 0, 0 };
    k->hdUsed = (uint8_t)(k->hdUsed + dice);
    int v = k->hp + hp;
    k->hp = (int16_t)(v < (int)k->hpMax ? v : (int)k->hpMax);
    if (c.before == 0 && k->hp > 0)
        k->ds = k->df = 0;
    for (int i = 0; i < k->nFeats; i++) {
        const Feature *f = features_find(lib, k->feats[i].name);
        if (f && f->reset == RESET_SHORT)
            k->feats[i].used = 0;
        else if (f && f->reset == RESET_SHORT1 && k->feats[i].used > 0)
            k->feats[i].used--;
    }
    c.after = k->hp;
    return c;
}

int act_short_rest_average(const Character *k, int dice)
{
    // Math.floor(n * ((die + 1) / 2 + con)) with the halves kept exact.
    int twice = dice * (k->hitDie + 1 + 2 * rules_mod(k->ab[AB_CON]));
    int avg = twice >= 0 ? twice / 2 : -((1 - twice) / 2);
    return avg > 0 ? avg : 0;
}

bool act_slot(Character *k, int level, int delta)
{
    if (delta > 0 && k->slotUsed[level] < k->slotMax[level]) {
        k->slotUsed[level]++;
        return true;
    }
    if (delta < 0 && k->slotUsed[level] > 0) {
        k->slotUsed[level]--;
        return true;
    }
    return false;
}

void act_set_slot_max(Character *k, int level, int max)
{
    k->slotMax[level] = (uint8_t)max;
    if (k->slotUsed[level] > max)
        k->slotUsed[level] = (uint8_t)max;
}

FeatUse act_feat_use(Character *k, const FeatureLib *lib, const char *name, int delta)
{
    int i = character_find_feat(k, name);
    int max = features_max_uses(k, features_find(lib, name));
    if (i < 0 || !max)
        return FEAT_UNTRACKED;
    KnownFeat *f = &k->feats[i];
    if (delta > 0) {
        if (f->used >= max)
            return FEAT_NONE_LEFT;
        f->used++;
        return FEAT_USED;
    }
    if (f->used == 0)
        return FEAT_FULL;
    f->used--;
    return FEAT_RESTORED;
}

PrepResult act_toggle_prepared(Character *k, const SpellLib *lib, const char *name)
{
    int i = character_find_spell(k, name);
    if (i < 0)
        return PREP_UNKNOWN;
    const Spell *sp = spells_find(lib, name);
    if (sp && sp->level == 0)
        return PREP_CANTRIP;
    k->spells[i].prep ^= 1;
    return k->spells[i].prep ? PREP_ON : PREP_OFF;
}

PickResult act_toggle_spell(Character *k, const char *name)
{
    if (act_remove_spell(k, name))
        return PICK_REMOVED;
    if (k->nSpells >= MAX_SPELLS)
        return PICK_FULL;
    KnownSpell *s = &k->spells[k->nSpells++];
    text_copy(s->name, name, SPELL_NAME_LEN);
    s->prep = 0;
    return PICK_ADDED;
}

PickResult act_toggle_feat(Character *k, const char *name)
{
    if (act_remove_feat(k, name))
        return PICK_REMOVED;
    if (k->nFeats >= MAX_FEATS)
        return PICK_FULL;
    KnownFeat *f = &k->feats[k->nFeats++];
    text_copy(f->name, name, FEAT_NAME_LEN);
    f->used = 0;
    return PICK_ADDED;
}

bool act_remove_spell(Character *k, const char *name)
{
    int i = character_find_spell(k, name);
    if (i < 0)
        return false;
    memmove(&k->spells[i], &k->spells[i + 1], (size_t)(k->nSpells - i - 1) * sizeof k->spells[0]);
    k->nSpells--;
    return true;
}

bool act_remove_feat(Character *k, const char *name)
{
    int i = character_find_feat(k, name);
    if (i < 0)
        return false;
    memmove(&k->feats[i], &k->feats[i + 1], (size_t)(k->nFeats - i - 1) * sizeof k->feats[0]);
    k->nFeats--;
    return true;
}

int act_add_weapon(Character *k, const char *name)
{
    if (k->nWeapons >= MAX_WEAPONS)
        return -1;
    weapon_init(&k->weapons[k->nWeapons], name);
    return k->nWeapons++;
}

void act_delete_weapon(Character *k, int idx)
{
    if (idx < 0 || idx >= k->nWeapons)
        return;
    memmove(&k->weapons[idx], &k->weapons[idx + 1], (size_t)(k->nWeapons - idx - 1) * sizeof k->weapons[0]);
    k->nWeapons--;
}

void act_set_level(Character *k, int level)
{
    k->level = (uint8_t)level;
    if (k->hdUsed > level)
        k->hdUsed = (uint8_t)level;
}

void act_set_hp_max(Character *k, int max)
{
    if (k->hp == (int)k->hpMax)
        k->hp = (int16_t)max;
    k->hpMax = (uint16_t)max;
    if (k->hp > max)
        k->hp = (int16_t)max;
}
