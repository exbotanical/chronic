#include "parser.h"

#include <string.h>

#include "constants.h"
#include "cronentry.h"
#include "tap.c/tap.h"
#include "tests.h"

void
strip_comment_test (void) {
  typedef struct {
    char* input;
    char* expect;
  } test_case;

  // clang-format off
  test_case tests[] = {
    { .input = "RET # dekef", .expect = "RET " },
    { .input = "RET ##",      .expect = "RET " },
    { .input = "RET##",       .expect = "RET"  },
    { .input = "# ee",        .expect = ""     },
    { .input = "",            .expect = ""     },
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case tc = tests[i];

    char* s      = s_copy(tc.input);
    strip_comment(s);

    is(s, tc.expect, "Expect '%s'", tc.expect);

    free(s);
  }
}

void
is_comment_line_test (void) {
  typedef struct {
    char* input;
    bool  ret;
  } test_case;

  // clang-format off
  test_case tests[] = {
    { .input = "# comment",               .ret = true  },
    { .input = "     #comment",           .ret = true  },
    { .input = "not_a_comment # comment", .ret = false },
    { .input = "not_a_comment",           .ret = false },
    { .input = "",                        .ret = false },
    { .input = " ",                       .ret = false },
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case tc  = tests[i];
    bool      ret = is_comment_line(tc.input);

    ok(tc.ret == ret, "Expect is_comment_line -> %d", ret);
  }
}

void
parse_schedule_test (void) {
  typedef struct {
    char* input;
    int   num;
    char* expect;
    char* err_msg;
  } test_case;

  // clang-format off
  test_case tests[] = {
    { .input  = "* * * * * RET",                       .expect = "* * * * *"                          },
    { .input  = "*  * * * * RET",                      .expect = "*  * * * *"                         },
    { .input  = "*	* * * * RET",                      .expect = "*	* * * *"                          },
    { .input  = "*	* *   *  *         RET",           .expect = "*	* *   *  *"                       },
    { .input  = "52 6	1 * *         RET",              .expect = "52 6	1 * *"                        },
    { .input  = "52 6	1 * *         ",                 .expect = "52 6	1 * *"                        },
    { .input  = "*/10 * * * *			x",                  .expect = "*/10 * * * *"                       },
    // Failure modes and edge cases
    { .input = "",                                     .err_msg = "empty string"                      },
    { .input = "\t",                                   .err_msg = "only whitespace"                   },
    { .input = " ",                                    .err_msg = "only whitespace"                   },
    { .input = "* * * * *CMD",                         .err_msg = "no space between schedule and cmd" },
    { .input = NULL,                                   .err_msg = "NULL input"                        },
    { .input = "                 ",                    .err_msg = "only whitespace"                   },
    { .input = "* * ",                                 .err_msg = "truncated schedule"                },
    { .input = "* * * * * ",                           .err_msg = "no command"                        },
    { .input = "* *   *                *       * CMD", .err_msg = "more than 32 chars in schedule"    },
    { .input  = "*/10 * * * * * * * * * * *",          .expect = "*/10 * * * *"                       },
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case tc = tests[i];

    char     ret[32];
    retval_t rv = parse_schedule(tc.input, ret);

    if (tc.err_msg) {
      ok(rv == ERR, "returns ERR (reason: %s)", tc.err_msg);
    } else {
      ok(rv == OK, "returns OK");
      is(ret, tc.expect, "Expect '%s'", tc.expect ? tc.expect : "NULL");
    }
  }
}

void
parse_cmd_test (void) {
  typedef struct {
    char* input;
    char* expect;
    char* err_msg;
  } test_case;

  // clang-format off
  test_case tests[] = {
    {
      .input   = "* * * * * /bin/sh command",
      .expect  = "/bin/sh command"
    },
    {
      .input   = "25 6	* * *	root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.daily )",
      .expect  = "root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.daily )"
    },
    // Failure modes and edge cases
    {
      // This will be considered acceptable but the schedule parse will fail.
      .input   = "root root root root root root",
      .expect  = "root"
    },
    {
      .input   = "* * * * *",
      .err_msg = "missing command"
    },
    {
      .input   = "* * * * * ",
      .err_msg = "missing command"
    },
    {
      .input   = "25 6	* * *	root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.daily )",
      .expect  = "root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.daily )"
    },
    { .input   = "* * * * * xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
       "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
       "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
      .err_msg = "too long"
    },
    {
      .input   = NULL,
      .err_msg = "NULL input"
    }
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case tc = tests[i];

    char     ret[256];
    retval_t rv = parse_cmd(tc.input, ret);

    if (tc.err_msg) {
      ok(rv == ERR, "returns ERR (reason: %s)", tc.err_msg);
    } else {
      ok(rv == OK, "returns OK");
      is(ret, tc.expect, "Expect '%s'", tc.expect ? tc.expect : "NULL");
    }
  }
}

void
parse_expr_test (void) {
  typedef struct {
    char* input;
    char* err_msg;
  } test_case;

  // clang-format off
  test_case tests[] = {
    { .input = "* * * * *"                                  },
    { .input = "*/15 * * * *"                               },
    { .input = "*  *  * * *"                                },
    { .input = "* * *",           .err_msg = "too short"    },
    { .input = "* * * * * * * *", .err_msg = "too long"     },
    { .input = "* * * * &",       .err_msg = "invalid char" },
    { .input = "* * * * -1",      .err_msg = "signed int"   },
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case tc = tests[i];

    cron_entry entry;
    memcpy(entry.schedule, tc.input, strlen(tc.input));
    entry.schedule[strlen(tc.input)] = '\0';

    retval_t rv                      = parse_expr(&entry);
    if (tc.err_msg) {
      ok(rv == ERR, "returns ERR (reason: %s)", tc.err_msg);
    } else {
      ok(rv == OK, "returns OK");
    }
  }
}

void
parse_entry_test (void) {
  typedef struct {
    char* input;
    char* expect;
    bool  expect_err;
  } test_case;

  // clang-format off
  test_case tests[] = {
    { .input = s_copy("*/5 * * * * /bin/sh command # with a comment"), .expect     = "/bin/sh command"  },
    { .input = s_copy("*/5 * * * * /bin/sh command#"),                 .expect     = "/bin/sh command"  },
    { .input = s_copy("*/5 * * * * /bin/sh command"),                  .expect     = "/bin/sh command"  },
    { .input = "*/5 * * * x /bin/sh command",                          .expect_err = true               },
    { .input = "/bin/sh command",                                      .expect_err = true               },
    { .input = NULL,                                                   .expect_err = true               },
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case  tc = tests[i];
    cron_entry entry;

    retval_t ret = parse_entry(&entry, tc.input);

    if (tc.expect_err) {
      ok(ret == ERR, "Expect result to be ERR (%d)", ERR);
    } else {
      ok(ret == OK, "Expect result to be OK (%d)", OK);
      is(entry.cmd, tc.expect, "Expect '%s'", tc.expect);
    }
  }
}

void
should_parse_line_test (void) {
  typedef struct {
    char* input;
    bool  ret;
  } test_case;

  // clang-format off
  test_case tests[] = {
    { .input = "# comment",               .ret = false },
    { .input = "     #comment",           .ret = false },
    { .input = "",                        .ret = false },
    { .input = " ",                       .ret = false },
    { .input = "   ",                     .ret = false },
    { .input = "not_a_comment # comment", .ret = true  },
    { .input = "not_a_comment",           .ret = true  },
  };
  // clang-format on

  ITER_CASES_TEST(tests, test_case) {
    test_case tc  = tests[i];
    bool      ret = should_parse_line(tc.input);

    ok(tc.ret == ret, "Expect should_parse_line -> %d", ret);
  }
}

void
run_parser_tests (void) {
  strip_comment_test();
  is_comment_line_test();
  parse_schedule_test();
  parse_cmd_test();
  parse_expr_test();
  parse_entry_test();
  should_parse_line_test();
}
