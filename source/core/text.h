// Text helpers shared by the save format, the library parsers and the UI:
// charset checks, the library-file cleaner, word wrap and number formatting.
#pragma once

#include <stdbool.h>
#include <stddef.h>

// Custom glyphs of the 5x7 font (assets/font5x7.h), one byte each.
// Always use them as separate string literals ("A" G_DOT "B"), never "\x87B".
#define G_HEART "\x80"
#define G_CHECK "\x81"
#define G_CROSS "\x82"
#define G_TRI_RIGHT "\x83"
#define G_TRI_LEFT "\x84"
#define G_TRI_UP "\x85"
#define G_TRI_DOWN "\x86"
#define G_DOT "\x87"
#define G_RING "\x88"
#define G_DIAMOND "\x89"
#define G_ARROW "\x8A"

// Names, classes, ranges: A-Z 0-9 space ' . - & / ( )
bool text_name_ok(const char *s);
// Weapon property text also allows , + ! ? :
bool text_props_ok(const char *s);

// Library-file cleaner: uppercase, curly quotes -> ', dashes -> -, ; -> ',',
// and drop anything outside A-Z 0-9 space ' . , : - + / ( ) ! ? & = < > and newline.
// Reads `len` bytes of UTF-8 and writes at most cap-1 bytes plus a terminator.
// Returns the length written.
size_t text_clean(const char *in, size_t len, char *out, size_t cap);

// Copies at most `n` characters of src into dst (which holds n+1) and terminates it.
void text_copy(char *dst, const char *src, size_t n);

// "+3" / "-1" / "+0" into buf (at least 12 bytes). Returns buf.
char *text_signed(char *buf, int n);

// One wrapped line: a slice of the source text (len 0 = blank line).
typedef struct {
    const char *p;
    int len;
} TextLine;

// Word wrap exactly like the prototype's wrap(): words are split on single
// spaces, a line never exceeds `cols` unless one word is longer, and leading
// empty words are dropped. Writes at most `max` lines and returns the total.
int text_wrap(const char *s, int len, int cols, TextLine *out, int max);

// Wraps a description: each '\n' starts a paragraph with one blank line
// between paragraphs (the prototype's split('\n').flatMap(...) pattern).
// With collapse_blanks, blank lines at the start or after another blank are
// dropped, as the hover summary card does. Writes at most `max` lines and
// returns the total.
int text_layout(const char *s, int cols, bool collapse_blanks, TextLine *out, int max);
