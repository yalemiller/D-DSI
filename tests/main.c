#include <stdio.h>
#include <stdlib.h>

#include "test.h"
#include "tests.h"

int t_checks, t_failures;

char *read_repo_file(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    char *buf = NULL;
    size_t cap = 0, n = 0;
    for (;;) {
        if (n + 4096 + 1 > cap) {
            cap = cap ? cap * 2 : 8192;
            char *b = realloc(buf, cap);
            if (!b) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = b;
        }
        size_t got = fread(buf + n, 1, 4096, f);
        n += got;
        if (got < 4096)
            break;
    }
    fclose(f);
    buf[n] = 0;
    *len = n;
    return buf;
}

static const struct {
    const char *name;
    void (*fn)(void);
} SUITES[] = {
    { "text", test_text },       { "rules", test_rules },     { "save", test_save },       { "libraries", test_libraries },
    { "portrait", test_portrait }, { "storage", test_storage }, { "actions", test_actions },
};

int main(void)
{
    for (size_t i = 0; i < sizeof SUITES / sizeof SUITES[0]; i++) {
        int before = t_failures, checks = t_checks;
        SUITES[i].fn();
        printf("%-10s %4d checks  %s\n", SUITES[i].name, t_checks - checks, t_failures == before ? "ok" : "FAILED");
    }
    printf("\n%d checks, %d failed\n", t_checks, t_failures);
    return t_failures ? 1 : 0;
}
