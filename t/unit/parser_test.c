#include "defs.h"
#include "tap.c/tap.h"
#include "tests.h"

typedef struct {
  char* input;
  bool expect_err;
} CleanStrTestCase;

void test_parse(void) {
  CleanStrTestCase tests[] = {
      {.input = "*/5 * * * * /bin/sh command # with a comment",
       .expect_err = false},
      {.input = "*/5 * * * * /bin/sh command#", .expect_err = false},
      {.input = "   */5 * * * * /bin/sh command ", .expect_err = false},
      {.input = "*/5 * * * x /bin/sh command ", .expect_err = true},
      {.input = "/bin/sh command ", .expect_err = true},
  };

  for (unsigned int i = 0; i < sizeof(tests) / sizeof(CleanStrTestCase); i++) {
    CleanStrTestCase tc = tests[i];
    Job job;

    bool ret = parse(&job, tc.input);

    if (tc.expect_err) {
      ok(ret == ERR);
    } else {
      ok(ret == OK);
      is(job.cmd, "/bin/sh command");
    }
  }
}
void run_parser_tests(void) { test_parse(); }
