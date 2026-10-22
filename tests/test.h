// Minimal test harness: plain C, no dependencies.
#pragma once

#include <stdio.h>
#include <string.h>

extern int t_checks, t_failures;

#define CHECK(c)                                                                                   \
    do {                                                                                           \
        t_checks++;                                                                                \
        if (!(c)) {                                                                                \
            t_failures++;                                                                          \
            fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c);                  \
        }                                                                                          \
    } while (0)

#define CHECK_INT(a, b)                                                                            \
    do {                                                                                           \
        long long a_ = (long long)(a), b_ = (long long)(b);                                        \
        t_checks++;                                                                                \
        if (a_ != b_) {                                                                            \
            t_failures++;                                                                          \
            fprintf(stderr, "%s:%d: %s is %lld, expected %lld\n", __FILE__, __LINE__, #a, a_, b_); \
        }                                                                                          \
    } while (0)

#define CHECK_STR(a, b)                                                                                     \
    do {                                                                                                    \
        const char *a_ = (a), *b_ = (b);                                                                    \
        t_checks++;                                                                                         \
        if (!a_ || !b_ || strcmp(a_, b_) != 0) {                                                            \
            t_failures++;                                                                                   \
            fprintf(stderr, "%s:%d: %s is \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #a, a_ ? a_ : "(null)", \
                    b_ ? b_ : "(null)");                                                                    \
        }                                                                                                   \
    } while (0)

// Reads a repo file (tests run from the repo root). Returns a malloc'd buffer or NULL.
char *read_repo_file(const char *path, size_t *len);
