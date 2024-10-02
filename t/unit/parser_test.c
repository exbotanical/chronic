#include "parser.h"

#include "constants.h"
#include "cronentry.h"
#include "tap.c/tap.h"
#include "tests.h"

void
strip_comment_test (void) {
  typedef struct {
    char* input;
    char* expect;
  } TestCase;

  TestCase tests[] = {
    {.input = "RET # dekef", .expect = "RET "},
    {.input = "RET ##",      .expect = "RET "},
    {.input = "RET##",       .expect = "RET" },
    {.input = "# ee",        .expect = ""    },
    {.input = "",            .expect = ""    },
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    char* s     = s_copy(tc.input);
    strip_comment(s);

    is(s, tc.expect, "Expect '%s'", tc.expect);

    free(s);
  }
}

void
until_nth_of_char_test (void) {
  typedef struct {
    char* input;
    char  delim;
    int   num;
    char* expect;
  } TestCase;

  TestCase tests[] = {
    {.input = "* * * * * RET", .delim = ' ', .num = 5, .expect = " RET"},
    {.input = "xxxRET",        .delim = 'x', .num = 3, .expect = "xRET"},
    {.input = "RET",           .delim = ' ', .num = 2, .expect = NULL  },
    {.input = " RET",          .delim = 'x', .num = 1, .expect = NULL  },
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    char* s     = s_copy(tc.input);
    char* ret   = until_nth_of_char(s, tc.delim, tc.num);

    is(ret, tc.expect, "Expect '%s'", tc.expect ? tc.expect : "NULL");

    free(s);
  }
}

void
parse_cmd_test (void) {
  typedef struct {
    char* input;
    int   spaces_until_cmd;
    bool  expect_err;
  } TestCase;

  TestCase tests[] = {
    {.input = "* * * * * /bin/sh command",   .spaces_until_cmd = 5,  .expect_err = false},
    {.input = "* * * * /bin/sh command",     .spaces_until_cmd = 4,  .expect_err = false},
    {.input = "   /bin/sh command",          .spaces_until_cmd = 3,  .expect_err = false},
    {.input = "*/5 * * *   /bin/sh command", .spaces_until_cmd = 6,  .expect_err = false},
    {.input = "/bin/sh command",             .spaces_until_cmd = 5,  .expect_err = true },
    {.input = " ",                           .spaces_until_cmd = 1,  .expect_err = true },
    {.input = " ",                           .spaces_until_cmd = 11, .expect_err = true },
  };

  char* expected = "/bin/sh command";
  ITER_CASES_TEST(tests, TestCase) {
    TestCase   tc = tests[i];
    cron_entry entry;

    char*    s   = s_copy(tc.input);
    retval_t ret = parse_cmd(s, &entry, tc.spaces_until_cmd);

    if (tc.expect_err) {
      ok(ret == ERR, "Expect result to be ERR (%d)", ERR);
    } else {
      ok(ret == OK, "Expect result to be OK (%d)", OK);
      is(expected, entry.cmd, "Expect '%s'", expected);
    }

    free(s);
  }
}

void
parse_entry_test (void) {
  typedef struct {
    char* input;
    bool  expect_err;
  } TestCase;

  TestCase tests[] = {
    {.input = "*/5 * * * * /bin/sh command # with a comment", .expect_err = false},
    {.input = "*/5 * * * * /bin/sh command#",                 .expect_err = false},
    {.input = "   */5 * * * * /bin/sh command ",              .expect_err = false},
    {.input = "*/5 * * * x /bin/sh command ",                 .expect_err = true },
    {.input = "/bin/sh command ",                             .expect_err = true },
  };

  char* expected = "/bin/sh command";
  ITER_CASES_TEST(tests, TestCase) {
    TestCase   tc = tests[i];
    cron_entry entry;

    retval_t ret = parse_entry(&entry, tc.input);

    if (tc.expect_err) {
      ok(ret == ERR, "Expect result to be ERR (%d)", ERR);
    } else {
      ok(ret == OK, "Expect result to be OK (%d)", OK);
      is(expected, entry.cmd, "Expect '%s'", expected);
    }
  }
}

void
is_comment_line_test (void) {
  typedef struct {
    char* input;
    bool  ret;
  } TestCase;

  TestCase tests[] = {
    {.input = "# comment",               .ret = true },
    {.input = "     #comment",           .ret = true },
    {.input = "not_a_comment # comment", .ret = false},
    {.input = "not_a_comment",           .ret = false},
    {.input = "",                        .ret = false},
    {.input = " ",                       .ret = false},
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    bool ret    = is_comment_line(tc.input);

    ok(tc.ret == ret, "Expect is_comment_line -> %d", ret);
  }
}

void
should_parse_line_test (void) {
  typedef struct {
    char* input;
    bool  ret;
  } TestCase;

  TestCase tests[] = {
    {.input = "# comment",               .ret = false},
    {.input = "     #comment",           .ret = false},
    {.input = "",                        .ret = false},
    {.input = " ",                       .ret = false},
    {.input = "   ",                     .ret = false},
    {.input = "not_a_comment # comment", .ret = true },
    {.input = "not_a_comment",           .ret = true },
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    bool ret    = should_parse_line(tc.input);

    ok(tc.ret == ret, "Expect should_parse_line -> %d", ret);
  }
}

void
run_parser_tests (void) {
  strip_comment_test();
  until_nth_of_char_test();
  parse_cmd_test();
  parse_entry_test();
  is_comment_line_test();
  should_parse_line_test();
}
