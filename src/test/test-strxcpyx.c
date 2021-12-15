/* SPDX-License-Identifier: LGPL-2.1+ */

#include <stdio.h>

#include "string-util.h"
#include "strxcpyx.h"
#include "util.h"

static void test_strpcpy(void) {
        char target[25];
        char *s = target;
        size_t space_left;
        bool truncated;

        space_left = sizeof(target);
        space_left = strpcpy_full(&s, space_left, "12345", &truncated);
        assert_se(!truncated);
        space_left = strpcpy_full(&s, space_left, "hey hey hey", &truncated);
        assert_se(!truncated);
        space_left = strpcpy_full(&s, space_left, "waldo", &truncated);
        assert_se(!truncated);
        space_left = strpcpy_full(&s, space_left, "ba", &truncated);
        assert_se(!truncated);
        space_left = strpcpy_full(&s, space_left, "r", &truncated);
        assert_se(!truncated);
        assert_se(space_left == 1);
        assert_se(streq(target, "12345hey hey heywaldobar"));

        space_left = strpcpy_full(&s, space_left, "", &truncated);
        assert_se(!truncated);
        assert_se(space_left == 1);
        assert_se(streq(target, "12345hey hey heywaldobar"));

        space_left = strpcpy_full(&s, space_left, "f", &truncated);
        assert_se(truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "12345hey hey heywaldobar"));

        space_left = strpcpy_full(&s, space_left, "", &truncated);
        assert_se(!truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "12345hey hey heywaldobar"));

        space_left = strpcpy_full(&s, space_left, "foo", &truncated);
        assert_se(truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "12345hey hey heywaldobar"));
}

static void test_strpcpyf(void) {
        char target[25];
        char *s = target;
        size_t space_left;
        bool truncated;

        space_left = sizeof(target);
        space_left = strpcpyf_full(&s, space_left, &truncated, "space left: %zu. ", space_left);
        assert_se(!truncated);
        space_left = strpcpyf_full(&s, space_left, &truncated, "foo%s", "bar");
        assert_se(!truncated);
        assert_se(space_left == 3);
        assert_se(streq(target, "space left: 25. foobar"));

        space_left = strpcpyf_full(&s, space_left, &truncated, "%i", 42);
        assert_se(!truncated);
        assert_se(space_left == 1);
        assert_se(streq(target, "space left: 25. foobar42"));

        space_left = strpcpyf_full(&s, space_left, &truncated, "%s", "");
        assert_se(!truncated);
        assert_se(space_left == 1);
        assert_se(streq(target, "space left: 25. foobar42"));

        space_left = strpcpyf_full(&s, space_left, &truncated, "%c", 'x');
        assert_se(truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "space left: 25. foobar42"));

        space_left = strpcpyf_full(&s, space_left, &truncated, "%s", "");
        assert_se(!truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "space left: 25. foobar42"));

        space_left = strpcpyf_full(&s, space_left, &truncated, "abc%s", "hoge");
        assert_se(truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "space left: 25. foobar42"));

        /* test overflow */
        s = target;
        space_left = strpcpyf_full(&s, 12, &truncated, "00 left: %i. ", 999);
        assert_se(truncated);
        assert_se(streq(target, "00 left: 99"));
        assert_se(space_left == 0);
        assert_se(target[12] == '2');
}

static void test_strpcpyl(void) {
        char target[25];
        char *s = target;
        size_t space_left;
        bool truncated;

        space_left = sizeof(target);
        space_left = strpcpyl_full(&s, space_left, &truncated, "waldo", " test", " waldo. ", NULL);
        assert_se(!truncated);
        space_left = strpcpyl_full(&s, space_left, &truncated, "Banana", NULL);
        assert_se(!truncated);
        assert_se(space_left == 1);
        assert_se(streq(target, "waldo test waldo. Banana"));

        space_left = strpcpyl_full(&s, space_left, &truncated, "", "", "", NULL);
        assert_se(!truncated);
        assert_se(space_left == 1);
        assert_se(streq(target, "waldo test waldo. Banana"));

        space_left = strpcpyl_full(&s, space_left, &truncated, "", "x", "", NULL);
        assert_se(truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "waldo test waldo. Banana"));

        space_left = strpcpyl_full(&s, space_left, &truncated, "hoge", NULL);
        assert_se(truncated);
        assert_se(space_left == 0);
        assert_se(streq(target, "waldo test waldo. Banana"));
}

static void test_strscpy(void) {
        char target[25];
        size_t space_left;
        bool truncated;

        space_left = sizeof(target);
        space_left = strscpy_full(target, space_left, "12345", &truncated);
        assert_se(!truncated);

        assert_se(streq(target, "12345"));
        assert_se(space_left == 20);
}

static void test_strscpyl(void) {
        char target[25];
        size_t space_left;
        bool truncated;

        space_left = sizeof(target);
        space_left = strscpyl_full(target, space_left, &truncated, "12345", "waldo", "waldo", NULL);
        assert_se(!truncated);

        assert_se(streq(target, "12345waldowaldo"));
        assert_se(space_left == 10);
}

static void test_sd_event_code_migration(void) {
        char b[100 * DECIMAL_STR_MAX(unsigned) + 1];
        char c[100 * DECIMAL_STR_MAX(unsigned) + 1], *p;
        unsigned i;
        size_t l;
        int o;

        for (i = o = 0; i < 100; i++)
                o += snprintf(&b[o], sizeof(b) - o, "%u ", i);

        p = c;
        l = sizeof(c);
        for (i = 0; i < 100; i++)
                l = strpcpyf(&p, l, "%u ", i);

        assert_se(streq(b, c));
}

int main(int argc, char *argv[]) {
        test_strpcpy();
        test_strpcpyf();
        test_strpcpyl();
        test_strscpy();
        test_strscpyl();

        test_sd_event_code_migration();

        return 0;
}
