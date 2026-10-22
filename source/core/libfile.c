#include "libfile.h"

#include <stdlib.h>
#include <string.h>

#include "text.h"

static bool is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
}

void line_iter_init(LineIter *it, const char *text, size_t len)
{
    it->p = text;
    it->end = text + len;
    if (len >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB &&
        (unsigned char)text[2] == 0xBF)
        it->p += 3;
}

bool line_iter_next(LineIter *it, const char **line, size_t *len)
{
    if (it->p > it->end)
        return false;
    const char *s = it->p;
    const char *e = s;
    while (e < it->end && *e != '\n')
        e++;
    it->p = e + 1; // past the end once the last line is returned
    while (s < e && is_ws(*s))
        s++;
    while (e > s && is_ws(e[-1]))
        e--;
    *line = s;
    *len = (size_t)(e - s);
    return true;
}

bool lib_match_key(const char *line, size_t len, bool digits, char *key, size_t keycap, const char **val,
                   size_t *vallen)
{
    size_t i = 0, k = 0;
    while (i < len) {
        char c = line[i];
        bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        bool digit = digits && c >= '0' && c <= '9';
        if (!letter && !digit)
            break;
        if (k + 1 < keycap)
            key[k++] = (char)(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
        i++;
    }
    if (i == 0)
        return false;
    key[k] = 0;
    while (i < len && is_ws(line[i]))
        i++;
    if (i >= len || line[i] != ':')
        return false;
    i++;
    while (i < len && is_ws(line[i]))
        i++;
    *val = line + i;
    *vallen = len - i;
    return true;
}

size_t lib_clean_trim(const char *s, size_t len, char *out, size_t cap)
{
    size_t n = text_clean(s, len, out, cap);
    size_t a = 0;
    while (a < n && is_ws(out[a]))
        a++;
    while (n > a && is_ws(out[n - 1]))
        n--;
    memmove(out, out + a, n - a);
    out[n - a] = 0;
    return n - a;
}

void desc_init(DescBuf *d)
{
    d->buf = NULL;
    d->len = d->cap = 0;
    d->tokens = 0;
    d->oom = false;
}

void desc_free(DescBuf *d)
{
    free(d->buf);
    desc_init(d);
}

static void desc_append(DescBuf *d, const char *s, size_t n)
{
    if (d->oom)
        return;
    if (d->len + n + 1 > d->cap) {
        size_t cap = d->cap ? d->cap : 256;
        while (d->len + n + 1 > cap)
            cap *= 2;
        char *b = realloc(d->buf, cap);
        if (!b) {
            d->oom = true;
            return;
        }
        d->buf = b;
        d->cap = cap;
    }
    memcpy(d->buf + d->len, s, n);
    d->len += n;
    d->buf[d->len] = 0;
}

void desc_push(DescBuf *d, const char *tok, size_t len)
{
    // join(' ') puts a separator between every pair of tokens, even empty ones.
    if (d->tokens++ > 0)
        desc_append(d, " ", 1);
    desc_append(d, tok, len);
}

void desc_push_clean(DescBuf *d, const char *line, size_t len)
{
    if (d->tokens++ > 0)
        desc_append(d, " ", 1);
    size_t start = d->len;
    desc_append(d, line, len); // reserves room; cleaning never grows the text
    if (d->oom)
        return;
    d->len = start + text_clean(line, len, d->buf + start, len + 1);
}

char *desc_finish(DescBuf *d, size_t max)
{
    if (d->oom) {
        desc_free(d);
        return NULL;
    }
    char empty[1] = { 0 };
    char *s = d->buf ? d->buf : empty;
    size_t n = d->len, o = 0;
    // / *\n */g -> "\n"
    for (size_t i = 0; i < n;) {
        size_t j = i;
        while (j < n && s[j] == ' ')
            j++;
        if (j < n && s[j] == '\n') {
            s[o++] = '\n';
            j++;
            while (j < n && s[j] == ' ')
                j++;
            i = j;
        } else if (j > i) {
            while (i < j)
                s[o++] = s[i++];
        } else {
            s[o++] = s[i++];
        }
    }
    n = o;
    // / +/g -> " "
    o = 0;
    for (size_t i = 0; i < n; i++)
        if (!(s[i] == ' ' && o > 0 && s[o - 1] == ' '))
            s[o++] = s[i];
    n = o;
    // trim, then slice(0, max)
    size_t a = 0;
    while (a < n && is_ws(s[a]))
        a++;
    while (n > a && is_ws(s[n - 1]))
        n--;
    if (n - a > max)
        n = a + max;
    const char *src = s + a;
    size_t outLen = n - a;
    if (outLen == 0) {
        src = "NO DESCRIPTION.";
        outLen = strlen(src);
    }
    char *res = malloc(outLen + 1);
    if (res) {
        memcpy(res, src, outLen);
        res[outLen] = 0;
    }
    desc_free(d);
    return res;
}
