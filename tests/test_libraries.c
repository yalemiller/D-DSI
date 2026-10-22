#include <stdlib.h>

#include "core/demo.h"
#include "core/features.h"
#include "core/spells.h"
#include "core/text.h"
#include "test.h"
#include "tests.h"

static SpellLib slib;
static FeatureLib flib;

static LibParseResult parse_spells(const char *text)
{
    return spells_parse(&slib, text, strlen(text));
}

static LibParseResult parse_feats(const char *text)
{
    return features_parse(&flib, text, strlen(text));
}

static void test_spells(void)
{
    spells_init(&slib);
    CHECK_INT(slib.count, 45);
    CHECK_INT(SPELL_BUILTIN_COUNT, 45);
    const Spell *s = spells_find(&slib, "FIND FAMILIAR");
    CHECK(s && s->level == 1 && s->ritual && !s->conc && !s->fromFile);
    CHECK(s && !strcmp(s->time, "1 HOUR"));
    s = spells_find(&slib, "GUIDANCE");
    CHECK(s && s->level == 0 && s->conc);
    s = spells_find(&slib, "DETECT MAGIC");
    CHECK(s && s->conc && s->ritual);
    for (int i = 0; i < slib.count; i++) {
        CHECK(text_name_ok(slib.items[i].name));
        CHECK(strlen(slib.items[i].desc) <= SPELL_DESC_MAX);
    }

    // the SD card file shipped with the project (CRLF line endings)
    size_t len;
    char *txt = read_repo_file("sd-card/data/pocketsheet/spells.txt", &len);
    CHECK(txt != NULL);
    if (txt) {
        LibParseResult r = spells_parse(&slib, txt, len);
        CHECK_INT(r.added, 1);
        CHECK_INT(r.skipped, 0);
        s = spells_find(&slib, "SILVERED SHELL");
        CHECK(s && s->fromFile && s->level == 1);
        CHECK(s && !strcmp(s->school, "TRANSMUTATION") && !strcmp(s->time, "1 BONUS ACTION"));
        CHECK(s && !strcmp(s->comp, "V, S, M") && !strcmp(s->dur, "1 MINUTE"));
        CHECK(s && !strncmp(s->desc, "YOU TOUCH A WEAPON AND COAT IT IN OTHERWORLDLY BILE.", 52));
        CHECK(s && strstr(s->desc, "ON A HIT.\nHIGHER SLOTS:") != NULL);
        CHECK(s && strstr(s->desc, "OR HIGHER.\nMATERIAL: AN EMPTY PHANTASM SHELL.") != NULL);
        free(txt);
    }
    // the template + Jedediah's entry written on first run parse the same way
    spells_init(&slib);
    LibParseResult r = parse_spells(DEMO_SPELLS_TEMPLATE);
    CHECK_INT(r.added, 0);
    CHECK_INT(r.skipped, 0);
    r = parse_spells(DEMO_SPELLS_JED);
    CHECK_INT(r.added, 1);
    CHECK_INT(slib.count, 46);

    // replacing a built-in, skipping bad entries, fields and tags
    spells_init(&slib);
    r = parse_spells("\xEF\xBB\xBF# comment\n"
                     "@Fire Bolt\nlevel: c\ntags: concentration, ritual\nschool: evocation that is far too long\n"
                     "Line one\n  continues;  here\n\n\nSecond: para\n# not part of it\nlevel: 9\n"
                     "@NO LEVEL\nschool: x\nText\n"
                     "@THIS NAME IS WAY TOO LONG\nlevel: 1\n"
                     "@BAD, NAME\nlevel: 1\n"
                     "@EMPTY\nlevel: 3\n"
                     "@\nlevel: 2\n");
    CHECK_INT(r.added, 2);
    CHECK_INT(r.skipped, 4);
    CHECK_INT(slib.count, 46);
    s = spells_find(&slib, "FIRE BOLT");
    CHECK(s && s->fromFile && s->level == 0 && s->conc && s->ritual);
    CHECK(s && !strcmp(s->school, "EVOCATION THAT IS FA")); // sliced to 20
    CHECK(s && !strcmp(s->desc, "LINE ONE CONTINUES, HERE\n\nSECOND: PARA LEVEL: 9"));
    s = spells_find(&slib, "EMPTY");
    CHECK(s && !strcmp(s->desc, "NO DESCRIPTION."));

    // level forms
    spells_init(&slib);
    r = parse_spells("@A\nlevel: cantrip\n@B\nlevel: 0\n@C\nlevel: 10\n@D\nLEVEL : 4\n");
    CHECK_INT(r.added, 3);
    CHECK_INT(r.skipped, 1);
    CHECK(spells_find(&slib, "A")->level == 0);
    CHECK(spells_find(&slib, "B")->level == 0);
    CHECK(spells_find(&slib, "D")->level == 4);

    // descriptions are capped at 1500 characters
    char *big = malloc(4000);
    strcpy(big, "@BIG\nlevel: 1\n");
    for (int i = 0; i < 300; i++)
        strcat(big, "WORDS ");
    r = parse_spells(big);
    CHECK_INT(r.added, 1);
    CHECK_INT((long long)strlen(spells_find(&slib, "BIG")->desc), 1500);
    free(big);

    // only the first 300 valid entries from a file are used
    spells_init(&slib);
    char *many = malloc(400 * 24);
    many[0] = 0;
    for (int i = 0; i < 310; i++)
        sprintf(many + strlen(many), "@S%03d\nlevel: 1\n", i);
    r = parse_spells(many);
    CHECK_INT(r.added, 300);
    CHECK_INT(r.skipped, 0);
    CHECK_INT(slib.count, 45 + 300);
    CHECK(spells_find(&slib, "S299") != NULL);
    CHECK(spells_find(&slib, "S300") == NULL);
    free(many);
    spells_free(&slib);
}

static void test_features(void)
{
    Character k;
    features_init(&flib);
    CHECK_INT(flib.count, 11);
    const Feature *f = features_find(&flib, "LUCKY");
    CHECK(f && f->cat == FCAT_FEAT && f->usesKind == USES_PB && f->reset == RESET_LONG);
    f = features_find(&flib, "RESOURCEFUL");
    CHECK(f && f->inspire && f->cat == FCAT_SPECIES && f->usesKind == USES_NONE && f->reset == RESET_UNSET);
    f = features_find(&flib, "BARDIC INSPIRATION");
    CHECK(f && f->usesKind == USES_ABILITY && f->usesVal == AB_CHA);
    f = features_find(&flib, "CHANNEL DIVINITY");
    CHECK(f && f->usesKind == USES_FIXED && f->usesVal == 2 && f->reset == RESET_SHORT1);

    size_t len;
    char *txt = read_repo_file("sd-card/data/pocketsheet/features.txt", &len);
    CHECK(txt != NULL);
    if (txt) {
        LibParseResult r = features_parse(&flib, txt, len);
        CHECK_INT(r.added, 9);
        CHECK_INT(r.skipped, 0);
        f = features_find(&flib, "BARREL");
        CHECK(f && f->cat == FCAT_ITEM && f->usesKind == USES_FIXED && f->usesVal == 6 && f->reset == RESET_MANUAL);
        f = features_find(&flib, "FAMILIAR FRIEND");
        CHECK(f && f->cat == FCAT_FEAT && f->usesKind == USES_NONE && f->fromFile);
        CHECK(f && strstr(f->desc, "CHARACTER LEVEL.\nWHEN YOU MAKE") != NULL);
        f = features_find(&flib, "LANGUAGES");
        CHECK(f && f->cat == FCAT_OTHER && !strcmp(f->desc, "COMMON, ELVISH, CELESTIAL, THIEVES' CANT."));
        free(txt);
    }
    CHECK_INT(flib.count, 20);

    features_init(&flib);
    LibParseResult r = parse_feats("@A\ncategory: sub\nuses: 20\nreset: Short 1\ntags: inspire\nText\n"
                                   "@B\ncategory: \nuses: wis\n"
                                   "@C\ncategory: nonsense\n"
                                   "@D\nuses: 21\n"
                                   "@E\nreset: weekly\n"
                                   "@F\nuses: 0\n"
                                   "@G\ncategory: spe\nreset: none\n"
                                   "@LUCKY\ncategory: item\nuses: 3\nreset: short\nMy own luck.\n");
    CHECK_INT(r.added, 4);
    CHECK_INT(r.skipped, 4);
    f = features_find(&flib, "A");
    CHECK(f && f->cat == FCAT_SUBCLASS && f->usesVal == 20 && f->reset == RESET_SHORT1 && f->inspire);
    f = features_find(&flib, "B");
    CHECK(f && f->cat == FCAT_CLASS && f->usesKind == USES_ABILITY && f->usesVal == AB_WIS && f->reset == RESET_LONG);
    f = features_find(&flib, "G");
    CHECK(f && f->cat == FCAT_SPECIES && f->reset == RESET_MANUAL && f->usesKind == USES_NONE);
    f = features_find(&flib, "LUCKY");
    CHECK(f && f->fromFile && f->cat == FCAT_ITEM && f->reset == RESET_SHORT && !strcmp(f->desc, "MY OWN LUCK."));
    CHECK_INT(flib.count, 11 + 3);

    // max uses
    demo_jedediah(&k);
    features_init(&flib);
    CHECK_INT(features_max_uses(&k, features_find(&flib, "LUCKY")), 2);            // PB at level 2
    CHECK_INT(features_max_uses(&k, features_find(&flib, "BARDIC INSPIRATION")), 1); // CHA +0 -> at least 1
    CHECK_INT(features_max_uses(&k, features_find(&flib, "FAVORED ENEMY")), 2);
    CHECK_INT(features_max_uses(&k, features_find(&flib, "ARCHERY")), 0);
    CHECK_INT(features_max_uses(&k, NULL), 0);
    k.ab[AB_CHA] = 18;
    CHECK_INT(features_max_uses(&k, features_find(&flib, "BARDIC INSPIRATION")), 4);

    // weapon property keys
    char key[WEAPON_PROPS_LEN + 1];
    features_property_key("BARREL 6", 8, key);
    CHECK_STR(key, "BARREL");
    features_property_key("VERSATILE (1D10)", 16, key);
    CHECK_STR(key, "VERSATILE");
    features_property_key("REACH", 5, key);
    CHECK_STR(key, "REACH");
    features_property_key("AB12", 4, key);
    CHECK_STR(key, "AB12");
    features_property_key("THROWN (20/60) 2", 16, key);
    CHECK_STR(key, "THROWN (20/60)");
    features_free(&flib);
}

void test_libraries(void)
{
    test_spells();
    test_features();
}
