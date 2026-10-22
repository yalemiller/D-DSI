#include "features.h"

#include <stdlib.h>
#include <string.h>

#include "libfile.h"
#include "rules.h"
#include "text.h"

const char *const FCAT_NAMES[FCAT_COUNT] = { "CLASS", "SUBCLASS", "SPECIES", "BACKGROUND", "FEAT", "ITEM", "OTHER" };
const char *const FCAT_SHORT[FCAT_COUNT] = { "CLS", "SUB", "SPC", "BKG", "FT", "ITM", "OTH" };
const char *const RESET_NAMES[5] = { "", "LONG REST", "SHORT REST", "1 PER SHORT", "MANUAL" };

void features_free(FeatureLib *lib)
{
    for (int i = 0; i < lib->count; i++)
        free(lib->items[i].owned);
    lib->count = 0;
}

void features_init(FeatureLib *lib)
{
    features_free(lib);
    for (int i = 0; i < FEATURE_BUILTIN_COUNT && i < FEAT_LIB_MAX; i++) {
        const FeatureDef *d = &FEATURE_BUILTINS[i];
        Feature *f = &lib->items[lib->count++];
        memset(f, 0, sizeof *f);
        text_copy(f->name, d->name, FEAT_NAME_LEN);
        f->cat = d->cat;
        f->usesKind = d->usesKind;
        f->usesVal = d->usesVal;
        f->reset = d->reset;
        f->inspire = d->inspire;
        f->desc = d->desc;
    }
}

const Feature *features_find(const FeatureLib *lib, const char *name)
{
    for (int i = 0; i < lib->count; i++)
        if (!strcmp(lib->items[i].name, name))
            return &lib->items[i];
    return NULL;
}

int features_max_uses(const Character *k, const Feature *f)
{
    if (!f)
        return 0;
    switch (f->usesKind) {
    case USES_FIXED:
        return f->usesVal > 20 ? 20 : f->usesVal;
    case USES_PB:
        return rules_pb(k);
    case USES_ABILITY: {
        int m = rules_mod(k->ab[f->usesVal]);
        return m > 1 ? m : 1;
    }
    default:
        return 0;
    }
}

void features_property_key(const char *prop, size_t len, char *out)
{
    size_t n = len;
    // /\s*\(.*\)$/ -> "": cut at the first '(' when the text ends with ')'
    if (n && prop[n - 1] == ')') {
        const char *open = memchr(prop, '(', n);
        if (open) {
            n = (size_t)(open - prop);
            while (n && (prop[n - 1] == ' ' || prop[n - 1] == '\t'))
                n--;
        }
    }
    // /\s+\d+$/ -> ""
    size_t d = n;
    while (d && prop[d - 1] >= '0' && prop[d - 1] <= '9')
        d--;
    if (d < n && d && (prop[d - 1] == ' ' || prop[d - 1] == '\t')) {
        while (d && (prop[d - 1] == ' ' || prop[d - 1] == '\t'))
            d--;
        n = d;
    }
    if (n > WEAPON_PROPS_LEN)
        n = WEAPON_PROPS_LEN;
    memcpy(out, prop, n);
    out[n] = 0;
}

static void lib_put(FeatureLib *lib, const Feature *f)
{
    for (int i = 0; i < lib->count; i++) {
        if (!strcmp(lib->items[i].name, f->name)) {
            free(lib->items[i].owned);
            lib->items[i] = *f;
            return;
        }
    }
    if (lib->count < FEAT_LIB_MAX)
        lib->items[lib->count++] = *f;
    else
        free(f->owned);
}

typedef struct {
    bool active, body, nameOk, bad;
    Feature f;
    DescBuf desc;
} Entry;

static void finish(FeatureLib *lib, Entry *e, LibParseResult *r)
{
    if (!e->active)
        return;
    e->active = false;
    char *desc = desc_finish(&e->desc, FEAT_DESC_MAX);
    if (e->nameOk && !e->bad && desc) {
        // Valid entries past the limit are dropped without being counted, as in the prototype.
        if (r->added < FEAT_FILE_MAX) {
            if (e->f.reset == RESET_UNSET && e->f.usesKind != USES_NONE)
                e->f.reset = RESET_LONG;
            e->f.owned = desc;
            e->f.desc = desc;
            e->f.fromFile = true;
            lib_put(lib, &e->f);
            r->added++;
        } else {
            free(desc);
        }
    } else {
        free(desc);
        r->skipped++;
    }
}

static bool parse_uses(const char *v, Feature *f)
{
    if (!strcmp(v, "PB")) {
        f->usesKind = USES_PB;
        return true;
    }
    for (int i = 0; i < 6; i++) {
        if (!strcmp(v, AB_NAMES[i])) {
            f->usesKind = USES_ABILITY;
            f->usesVal = (uint8_t)i;
            return true;
        }
    }
    // /^([1-9]|1[0-9]|20)$/
    size_t n = strlen(v);
    bool ok = (n == 1 && v[0] >= '1' && v[0] <= '9') || (n == 2 && v[0] == '1' && v[1] >= '0' && v[1] <= '9') ||
              (n == 2 && v[0] == '2' && v[1] == '0');
    if (!ok)
        return false;
    f->usesKind = USES_FIXED;
    f->usesVal = (uint8_t)atoi(v);
    return true;
}

static bool parse_reset(const char *v, Feature *f)
{
    char r[16];
    size_t n = 0;
    for (; *v && n + 1 < sizeof r; v++)
        if (*v != ' ' && *v != '\t')
            r[n++] = *v;
    r[n] = 0;
    if (!strcmp(r, "LONG"))
        f->reset = RESET_LONG;
    else if (!strcmp(r, "SHORT"))
        f->reset = RESET_SHORT;
    else if (!strcmp(r, "SHORT1"))
        f->reset = RESET_SHORT1;
    else if (!strcmp(r, "NONE"))
        f->reset = RESET_MANUAL;
    else
        return false;
    return true;
}

LibParseResult features_parse(FeatureLib *lib, const char *text, size_t len)
{
    LibParseResult r = { 0, 0 };
    static Entry e; // static: the DS stack is small
    memset(&e, 0, sizeof e);
    desc_init(&e.desc);
    static char buf[512];
    LineIter it;
    const char *line;
    size_t n;
    line_iter_init(&it, text, len);
    while (line_iter_next(&it, &line, &n)) {
        if (n && line[0] == '#')
            continue;
        if (n && line[0] == '@') {
            finish(lib, &e, &r);
            memset(&e.f, 0, sizeof e.f);
            e.active = true;
            e.body = false;
            e.bad = false;
            size_t nl = lib_clean_trim(line + 1, n - 1, buf, sizeof buf);
            e.nameOk = nl > 0 && nl <= FEAT_NAME_LEN && text_name_ok(buf);
            text_copy(e.f.name, buf, FEAT_NAME_LEN);
            e.f.cat = FCAT_OTHER;
            continue;
        }
        if (!e.active)
            continue;
        char key[16];
        const char *val;
        size_t vlen;
        if (!e.body && lib_match_key(line, n, true, key, sizeof key, &val, &vlen) &&
            (!strcmp(key, "category") || !strcmp(key, "uses") || !strcmp(key, "reset") || !strcmp(key, "tags"))) {
            lib_clean_trim(val, vlen, buf, sizeof buf);
            if (!strcmp(key, "category")) {
                // FCAT.findIndex(c => c.startsWith(v.slice(0, 3)))
                size_t pl = strlen(buf) < 3 ? strlen(buf) : 3;
                int found = -1;
                for (int i = 0; i < FCAT_COUNT && found < 0; i++)
                    if (!strncmp(FCAT_NAMES[i], buf, pl))
                        found = i;
                if (found < 0)
                    e.bad = true;
                else
                    e.f.cat = (uint8_t)found;
            } else if (!strcmp(key, "uses")) {
                if (!parse_uses(buf, &e.f))
                    e.bad = true;
            } else if (!strcmp(key, "reset")) {
                if (!parse_reset(buf, &e.f))
                    e.bad = true;
            } else {
                e.f.inspire = strstr(buf, "INSPIRE") != NULL;
            }
            continue;
        }
        if (!n) {
            if (e.body)
                desc_push(&e.desc, "\n", 1);
            continue;
        }
        e.body = true;
        desc_push_clean(&e.desc, line, n);
    }
    finish(lib, &e, &r);
    desc_free(&e.desc);
    return r;
}
