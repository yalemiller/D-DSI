#include "core/text.h"
#include "test.h"
#include "tests.h"

static int wrap_str(const char *s, int cols, TextLine *out, int max)
{
    return text_wrap(s, (int)strlen(s), cols, out, max);
}

static void check_line(TextLine l, const char *want)
{
    char buf[128];
    snprintf(buf, sizeof buf, "%.*s", l.len, l.p ? l.p : "");
    CHECK_STR(buf, want);
}

void test_text(void)
{
    char buf[256];
    TextLine L[16];

    CHECK(text_name_ok("JEDEDIAH GRAVES"));
    CHECK(text_name_ok("HUMAN (HALF-ELF)"));
    CHECK(text_name_ok("HUNTER'S MARK"));
    CHECK(text_name_ok("80/320"));
    CHECK(text_name_ok(""));
    CHECK(!text_name_ok("A,B"));
    CHECK(!text_name_ok("lower"));
    CHECK(text_props_ok("BARREL 6, BLARING, FAN FIRE"));
    CHECK(text_props_ok("VERSATILE (1D10)"));
    CHECK(!text_props_ok("A;B"));

    const char *curly = "Hello; \xE2\x80\x9CWorld\xE2\x80\x9D \xE2\x80\x93 it\xE2\x80\x99s";
    text_clean(curly, strlen(curly), buf, sizeof buf);
    CHECK_STR(buf, "HELLO, 'WORLD' - IT'S");
    const char *junk = "a\tb#c%\"d\"\n1d8+2 = <x>";
    text_clean(junk, strlen(junk), buf, sizeof buf);
    CHECK_STR(buf, "ABC'D'\n1D8+2 = <X>");
    text_clean("caf\xC3\xA9", 5, buf, sizeof buf);
    CHECK_STR(buf, "CAF");
    CHECK_INT(text_clean("abcdef", 6, buf, 4), 3);
    CHECK_STR(buf, "ABC");

    CHECK_STR(text_signed(buf, 3), "+3");
    CHECK_STR(text_signed(buf, 0), "+0");
    CHECK_STR(text_signed(buf, -2), "-2");

    // wrap() parity with the prototype
    CHECK_INT(wrap_str("THE QUICK BROWN FOX", 9, L, 16), 2);
    check_line(L[0], "THE QUICK");
    check_line(L[1], "BROWN FOX");
    CHECK_INT(wrap_str("ABCDEFGHIJKL MN", 5, L, 16), 2);
    check_line(L[0], "ABCDEFGHIJKL");
    check_line(L[1], "MN");
    CHECK_INT(wrap_str(" A", 10, L, 16), 1);
    check_line(L[0], "A");
    CHECK_INT(wrap_str("", 10, L, 16), 0);
    CHECK_INT(wrap_str("A  B", 10, L, 16), 1);
    check_line(L[0], "A  B");
    CHECK_INT(wrap_str("A B C D", 3, L, 1), 2); // total is returned even when out is short
    check_line(L[0], "A B");

    CHECK_INT(text_layout("A\nB", 40, false, L, 16), 3);
    check_line(L[0], "A");
    check_line(L[1], "");
    check_line(L[2], "B");
    CHECK_INT(text_layout("A\n\nB", 40, false, L, 16), 4);
    CHECK_INT(text_layout("A\n\nB", 40, true, L, 16), 3);
    CHECK_INT(text_layout("\nA", 40, true, L, 16), 1);
    check_line(L[0], "A");
    CHECK_INT(text_layout("ONE TWO THREE\nFOUR", 7, false, L, 2), 4); // ONE TWO / THREE / blank / FOUR
    check_line(L[1], "THREE");
}
