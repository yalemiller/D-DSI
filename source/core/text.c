#include "text.h"

#include <stdio.h>
#include <string.h>

static bool is_name_char(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ' || c == '\'' || c == '.' ||
           c == '-' || c == '&' || c == '/' || c == '(' || c == ')';
}

bool text_name_ok(const char *s)
{
    for (; *s; s++)
        if (!is_name_char(*s))
            return false;
    return true;
}

bool text_props_ok(const char *s)
{
    for (; *s; s++)
        if (!is_name_char(*s) && *s != ',' && *s != '+' && *s != '!' && *s != '?' && *s != ':')
            return false;
    return true;
}

static bool is_clean_char(char c)
{
    return is_name_char(c) || c == ',' || c == ':' || c == '+' || c == '!' || c == '?' || c == '=' ||
           c == '<' || c == '>' || c == '\n';
}

size_t text_clean(const char *in, size_t len, char *out, size_t cap)
{
    size_t o = 0;
    if (cap == 0)
        return 0;
    for (size_t i = 0; i < len && o + 1 < cap; i++) {
        unsigned char c = (unsigned char)in[i];
        char r = 0;
        if (c == 0xE2 && i + 2 < len && (unsigned char)in[i + 1] == 0x80) {
            unsigned char t = (unsigned char)in[i + 2];
            if (t == 0x98 || t == 0x99 || t == 0x9C || t == 0x9D)
                r = '\'';
            else if (t == 0x93 || t == 0x94)
                r = '-';
            if (r)
                i += 2;
        } else if (c == '"') {
            r = '\'';
        } else if (c == ';') {
            r = ',';
        } else if (c >= 'a' && c <= 'z') {
            r = (char)(c - 'a' + 'A');
        } else if (c < 0x80 && is_clean_char((char)c)) {
            r = (char)c;
        }
        if (r)
            out[o++] = r;
    }
    out[o] = 0;
    return o;
}

void text_copy(char *dst, const char *src, size_t n)
{
    size_t i = 0;
    for (; i < n && src[i]; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

char *text_signed(char *buf, int n)
{
    snprintf(buf, 12, "%c%d", n >= 0 ? '+' : '-', n >= 0 ? n : -n);
    return buf;
}

int text_wrap(const char *s, int len, int cols, TextLine *out, int max)
{
    int count = 0;
    int ls = 0, le = 0; // current line [ls, le)
    int ws = 0;
    while (ws <= len) {
        int we = ws;
        while (we < len && s[we] != ' ')
            we++;
        if (le == ls) {
            ls = ws;
            le = we;
        } else if (we - ls <= cols) {
            le = we;
        } else {
            if (count < max)
                out[count] = (TextLine){ s + ls, le - ls };
            count++;
            ls = ws;
            le = we;
        }
        ws = we + 1;
    }
    if (le > ls) {
        if (count < max)
            out[count] = (TextLine){ s + ls, le - ls };
        count++;
    }
    return count;
}

int text_layout(const char *s, int cols, bool collapse_blanks, TextLine *out, int max)
{
    int count = 0;
    bool prev_blank = false;
    for (int para = 0;; para++) {
        const char *nl = strchr(s, '\n');
        int len = nl ? (int)(nl - s) : (int)strlen(s);
        if (para > 0) {
            bool drop = collapse_blanks && (count == 0 || prev_blank);
            if (!drop) {
                if (count < max)
                    out[count] = (TextLine){ s, 0 };
                count++;
            }
            prev_blank = true;
        }
        int room = max - count > 0 ? max - count : 0;
        int n = text_wrap(s, len, cols, out + (count < max ? count : max), room);
        if (n > 0)
            prev_blank = false;
        count += n;
        if (!nl)
            break;
        s = nl + 1;
    }
    return count;
}
