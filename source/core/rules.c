#include "rules.h"

#include <stdio.h>
#include <string.h>

#include "text.h"

int rules_mod(int score)
{
    int d = score - 10;
    return d >= 0 ? d / 2 : -((1 - d) / 2);
}

int rules_pb(const Character *k)
{
    return 2 + (k->level - 1) / 4;
}

int rules_skill(const Character *k, int skill)
{
    return rules_mod(k->ab[SKILLS[skill].ability]) + k->skill[skill] * rules_pb(k);
}

int rules_save(const Character *k, int ability)
{
    return rules_mod(k->ab[ability]) + (((k->saveProf >> ability) & 1) ? rules_pb(k) : 0);
}

int rules_passive_perception(const Character *k)
{
    return 10 + rules_skill(k, SKILL_PERCEPTION);
}

int rules_spell_dc(const Character *k)
{
    return 8 + rules_pb(k) + rules_mod(k->ab[k->spellAb]);
}

int rules_spell_attack(const Character *k)
{
    return rules_pb(k) + rules_mod(k->ab[k->spellAb]);
}

int rules_weapon_ability(const Character *k, const Weapon *w)
{
    switch (w->ab) {
    case WAB_STR:
        return AB_STR;
    case WAB_DEX:
        return AB_DEX;
    case WAB_FINESSE:
        return rules_mod(k->ab[AB_DEX]) > rules_mod(k->ab[AB_STR]) ? AB_DEX : AB_STR;
    default:
        return k->spellAb == SPELL_AB_NONE ? -1 : k->spellAb;
    }
}

int rules_weapon_mod(const Character *k, const Weapon *w)
{
    int i = rules_weapon_ability(k, w);
    return i < 0 ? 0 : rules_mod(k->ab[i]);
}

int rules_weapon_to_hit(const Character *k, const Weapon *w)
{
    return rules_weapon_mod(k, w) + (w->prof ? rules_pb(k) : 0) + w->bonus;
}

char *rules_weapon_damage(const Weapon *w, char *buf, size_t cap)
{
    char sg[12];
    snprintf(buf, cap, "%uD%u%s", (unsigned)w->n, (unsigned)w->die, w->dmg ? text_signed(sg, w->dmg) : "");
    return buf;
}

int rules_weapon_is_ranged(const Weapon *w)
{
    return strchr(w->range, '/') || strstr(w->props, "AMMUNITION") || strstr(w->props, "BARREL");
}

int rules_graze(const Character *k, const Weapon *w)
{
    int m = rules_weapon_mod(k, w);
    return m > 0 ? m : 0;
}

int rules_topple_dc(const Character *k, const Weapon *w)
{
    return 8 + rules_weapon_mod(k, w) + rules_pb(k);
}

char *rules_mastery_tag(const Character *k, const Weapon *w, char *buf, size_t cap)
{
    if (w->mastery == MASTERY_TOPPLE)
        snprintf(buf, cap, "TOPPLE DC %d", rules_topple_dc(k, w));
    else if (w->mastery == MASTERY_GRAZE)
        snprintf(buf, cap, "GRAZE %d", rules_graze(k, w));
    else
        snprintf(buf, cap, "%s", MASTERY_NAMES[w->mastery < MASTERY_COUNT ? w->mastery : 0]);
    return buf;
}

char *rules_mastery_line(const Character *k, const Weapon *w, char *buf, size_t cap)
{
    switch (w->mastery) {
    case MASTERY_CLEAVE:
        snprintf(buf, cap, "CLEAVE: ON HIT, ATTACK A 2ND FOE NEAR IT");
        break;
    case MASTERY_GRAZE:
        snprintf(buf, cap, "GRAZE: ON A MISS, STILL DEAL %d DAMAGE", rules_graze(k, w));
        break;
    case MASTERY_NICK:
        snprintf(buf, cap, "NICK: LIGHT EXTRA ATTACK, NO BONUS ACTION");
        break;
    case MASTERY_PUSH:
        snprintf(buf, cap, "PUSH: ON HIT, SHOVE LARGE OR SMALLER 10 FT");
        break;
    case MASTERY_SAP:
        snprintf(buf, cap, "SAP: ON HIT, TARGET HAS DIS ON NEXT ATTACK");
        break;
    case MASTERY_SLOW:
        snprintf(buf, cap, "SLOW: ON HIT, TARGET SPEED -10 FT");
        break;
    case MASTERY_TOPPLE:
        snprintf(buf, cap, "TOPPLE: ON HIT, CON SAVE DC %d OR PRONE", rules_topple_dc(k, w));
        break;
    case MASTERY_VEX:
        snprintf(buf, cap, "VEX: ON HIT, ADV ON YOUR NEXT ATTACK");
        break;
    default:
        snprintf(buf, cap, "%s", "");
        break;
    }
    return buf;
}

char *rules_mastery_desc(const Character *k, const Weapon *w, char *buf, size_t cap)
{
    switch (w->mastery) {
    case MASTERY_CLEAVE:
        snprintf(buf, cap, "%s",
                 "IF YOU HIT A CREATURE WITH A MELEE ATTACK, YOU CAN MAKE ONE MORE MELEE ATTACK AGAINST A SECOND "
                 "CREATURE WITHIN 5 FEET OF THE FIRST AND WITHIN YOUR REACH. ON A HIT IT TAKES THE WEAPON DAMAGE "
                 "WITHOUT YOUR ABILITY MODIFIER (UNLESS IT IS NEGATIVE).\nONCE PER TURN.");
        break;
    case MASTERY_GRAZE:
        snprintf(buf, cap,
                 "IF YOUR ATTACK ROLL MISSES, THE TARGET STILL TAKES DAMAGE EQUAL TO YOUR ABILITY MODIFIER: %d "
                 "DAMAGE OF THE WEAPON'S TYPE.\nONLY YOUR ABILITY MODIFIER CAN RAISE THIS DAMAGE.",
                 rules_graze(k, w));
        break;
    case MASTERY_NICK:
        snprintf(buf, cap, "%s",
                 "WHEN YOU MAKE THE EXTRA ATTACK FROM THE LIGHT PROPERTY, YOU CAN MAKE IT AS PART OF THE ATTACK "
                 "ACTION INSTEAD OF USING A BONUS ACTION.\nONCE PER TURN.");
        break;
    case MASTERY_PUSH:
        snprintf(buf, cap, "%s",
                 "ON A HIT, YOU CAN PUSH THE CREATURE UP TO 10 FEET STRAIGHT AWAY FROM YOU IF IT IS LARGE OR "
                 "SMALLER.");
        break;
    case MASTERY_SAP:
        snprintf(buf, cap, "%s",
                 "ON A HIT, THE CREATURE HAS DISADVANTAGE ON ITS NEXT ATTACK ROLL BEFORE THE START OF YOUR NEXT "
                 "TURN.");
        break;
    case MASTERY_SLOW:
        snprintf(buf, cap, "%s",
                 "ON A HIT THAT DEALS DAMAGE, YOU CAN REDUCE THE CREATURE'S SPEED BY 10 FEET UNTIL THE START OF "
                 "YOUR NEXT TURN.\nHITTING IT AGAIN DOESN'T REDUCE IT BY MORE THAN 10 FEET.");
        break;
    case MASTERY_TOPPLE:
        snprintf(buf, cap,
                 "ON A HIT, YOU CAN FORCE THE CREATURE TO MAKE A CON SAVE, DC %d (8 + ABILITY MOD + PROF). ON A "
                 "FAILURE IT FALLS PRONE.",
                 rules_topple_dc(k, w));
        break;
    case MASTERY_VEX:
        snprintf(buf, cap, "%s",
                 "ON A HIT THAT DEALS DAMAGE, YOU HAVE ADVANTAGE ON YOUR NEXT ATTACK ROLL AGAINST THAT CREATURE "
                 "BEFORE THE END OF YOUR NEXT TURN.");
        break;
    default:
        snprintf(buf, cap, "%s", "");
        break;
    }
    return buf;
}
