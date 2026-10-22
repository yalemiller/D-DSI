#include "spells.h"

#include <stdlib.h>
#include <string.h>

#include "libfile.h"
#include "text.h"

void spells_free(SpellLib *lib)
{
    for (int i = 0; i < lib->count; i++)
        free(lib->items[i].owned);
    lib->count = 0;
}

void spells_init(SpellLib *lib)
{
    spells_free(lib);
    for (int i = 0; i < SPELL_BUILTIN_COUNT && i < SPELL_LIB_MAX; i++) {
        const SpellDef *d = &SPELL_BUILTINS[i];
        Spell *s = &lib->items[lib->count++];
        memset(s, 0, sizeof *s);
        text_copy(s->name, d->name, SPELL_FIELD_LEN);
        s->level = d->level;
        text_copy(s->school, d->school, SPELL_FIELD_LEN);
        text_copy(s->time, d->time, SPELL_FIELD_LEN);
        text_copy(s->range, d->range, SPELL_FIELD_LEN);
        text_copy(s->comp, d->comp, SPELL_FIELD_LEN);
        text_copy(s->dur, d->dur, SPELL_FIELD_LEN);
        s->conc = d->conc;
        s->ritual = d->ritual;
        s->desc = d->desc;
    }
}

const Spell *spells_find(const SpellLib *lib, const char *name)
{
    for (int i = 0; i < lib->count; i++)
        if (!strcmp(lib->items[i].name, name))
            return &lib->items[i];
    return NULL;
}

// Adds or replaces by name; takes ownership of s->owned.
static void lib_put(SpellLib *lib, const Spell *s)
{
    for (int i = 0; i < lib->count; i++) {
        if (!strcmp(lib->items[i].name, s->name)) {
            free(lib->items[i].owned);
            lib->items[i] = *s;
            return;
        }
    }
    if (lib->count < SPELL_LIB_MAX)
        lib->items[lib->count++] = *s;
    else
        free(s->owned);
}

typedef struct {
    bool active, body, nameOk;
    Spell s;
    DescBuf desc;
} Entry;

static void finish(SpellLib *lib, Entry *e, LibParseResult *r)
{
    if (!e->active)
        return;
    e->active = false;
    char *desc = desc_finish(&e->desc, SPELL_DESC_MAX);
    if (e->nameOk && e->s.level >= 0 && desc) {
        // Valid entries past the limit are dropped without being counted, as in the prototype.
        if (r->added < SPELL_FILE_MAX) {
            e->s.owned = desc;
            e->s.desc = desc;
            e->s.fromFile = true;
            lib_put(lib, &e->s);
            r->added++;
        } else {
            free(desc);
        }
    } else {
        free(desc);
        r->skipped++;
    }
}

LibParseResult spells_parse(SpellLib *lib, const char *text, size_t len)
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
            memset(&e.s, 0, sizeof e.s);
            e.active = true;
            e.body = false;
            size_t nl = lib_clean_trim(line + 1, n - 1, buf, sizeof buf);
            e.nameOk = nl > 0 && nl <= SPELL_FIELD_LEN && text_name_ok(buf);
            text_copy(e.s.name, buf, SPELL_FIELD_LEN);
            e.s.level = -1;
            continue;
        }
        if (!e.active)
            continue;
        char key[16];
        const char *val;
        size_t vlen;
        if (!e.body && lib_match_key(line, n, false, key, sizeof key, &val, &vlen)) {
            char *field = !strcmp(key, "school")       ? e.s.school
                          : !strcmp(key, "time")       ? e.s.time
                          : !strcmp(key, "range")      ? e.s.range
                          : !strcmp(key, "components") ? e.s.comp
                          : !strcmp(key, "duration")   ? e.s.dur
                                                       : NULL;
            bool known = field || !strcmp(key, "level") || !strcmp(key, "tags");
            if (known) {
                lib_clean_trim(val, vlen, buf, sizeof buf);
                if (!strcmp(key, "level")) {
                    if (!strcmp(buf, "C") || !strcmp(buf, "CANTRIP") || !strcmp(buf, "0"))
                        e.s.level = 0;
                    else if (buf[0] >= '1' && buf[0] <= '9' && !buf[1])
                        e.s.level = (int8_t)(buf[0] - '0');
                    else
                        e.s.level = -1;
                } else if (!strcmp(key, "tags")) {
                    e.s.conc = strstr(buf, "CONC") != NULL;
                    e.s.ritual = strstr(buf, "RITUAL") != NULL;
                } else {
                    text_copy(field, buf, SPELL_FIELD_LEN);
                }
                continue;
            }
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
