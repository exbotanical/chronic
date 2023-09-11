#include "parser.h"

#include <time.h>

#include "defs.h"
#include "tap.c/tap.h"
#include "tests.h"

// void test_parser(void) {
//   cron_config c = parse_file("t/fixtures/crontab.example");
//   printf("done\n");
//   printf("VAL: %d\n", c.variables->count);
//   printf("after print\n");
// }

typedef struct {
  char* input;
  bool expect_err;
} CleanStrTestCase;

void test_parse(void) {
  time_t now = 1694387623;
  time_t ret = 1694387625;

  CleanStrTestCase tests[] = {
      {.input = "*/5 * * * * * /bin/sh command # with a comment",
       .expect_err = false},
      {.input = "*/5 * * * * * /bin/sh command#", .expect_err = false},
      {.input = "   */5 * * * * * /bin/sh command ", .expect_err = false},
      {.input = "*/5 * * * x /bin/sh command ", .expect_err = true},
      {.input = "/bin/sh command ", .expect_err = true},
  };

  for (unsigned int i = 0; i < sizeof(tests) / sizeof(CleanStrTestCase); i++) {
    CleanStrTestCase tc = tests[i];
    Job job;

    time_t actual = parse(now, &job, tc.input);

    if (tc.expect_err) {
      ok(ERR == actual);
    } else {
      is(ret, actual);
    }
  }
}

void test_process_file(void) {
  array_t* lines = process_dir("./t/fixtures/valid");

  foreach (lines, i) {
    printf("LINE: %s\n", array_get(lines, i));
  }
}

void run_parser_tests(void) {
  test_parse();
  test_process_file();
}
