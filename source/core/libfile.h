// Shared plumbing for the spells.txt / features.txt parsers: line splitting,
// "key: value" headers and description assembly, matching the prototype.
#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *p;
    const char *end;
} LineIter;

// Starts at `text`, skipping a UTF-8 byte-order mark.
void line_iter_init(LineIter *it, const char *text, size_t len);
// Next line (split on \n or \r\n) with surrounding whitespace trimmed.
bool line_iter_next(LineIter *it, const char **line, size_t *len);

// Matches /^([a-z]+)\s*:\s*(.*)$/i (with digits allowed in the key when
// `digits` is set). On a match, writes the lowercased key (truncated to
// keycap-1) and returns the value slice.
bool lib_match_key(const char *line, size_t len, bool digits, char *key, size_t keycap, const char **val,
                   size_t *vallen);

// text_clean() into out, then trim spaces. Returns the length.
size_t lib_clean_trim(const char *s, size_t len, char *out, size_t cap);

// Description builder: tokens are joined with spaces like lines.join(' ').
typedef struct {
    char *buf;
    size_t len, cap;
    int tokens;
    bool oom;
} DescBuf;

void desc_init(DescBuf *d);
void desc_free(DescBuf *d);
void desc_push(DescBuf *d, const char *tok, size_t len);
// Pushes text_clean(line) as one token, with no length limit.
void desc_push_clean(DescBuf *d, const char *line, size_t len);
// Collapses " \n " to "\n" and runs of spaces to one, trims, truncates to
// `max` chars, and returns a malloc'd string ("NO DESCRIPTION." when empty),
// or NULL if memory ran out. Empties the builder.
char *desc_finish(DescBuf *d, size_t max);
