#include "tap.c/tap.h"
#include "tests.h"
#include "utils.h"

void round_ts_test(void) {
  typedef struct {
    time_t ts;
    unsigned short unit;
    time_t expect;
  } TestCase;

  TestCase tests[] = {
      {.ts = 105, .unit = 10, .expect = 110},
      {.ts = 103, .unit = 10, .expect = 100},
      {.ts = 50, .unit = 100, .expect = 100},
      {.ts = 53, .unit = 1, .expect = 53},
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    ok(round_ts(tc.ts, tc.unit) == tc.expect, "Rounds %d to %d given unit %d",
       tc.ts, tc.unit, tc.expect);
  }
}

void get_sleep_duration_test() {
  typedef struct {
    unsigned short ival;
    time_t now;
    unsigned short expect;
  } TestCase;

  TestCase tests[] = {
      {.ival = 5, .now = 1697811728219, .expect = 2},
      {.ival = 10, .now = 1697811728219, .expect = 2},
      {.ival = 30, .now = 1697812811409, .expect = 22},
      {.ival = 30, .now = 1697812854805, .expect = 6},
      {.ival = 60, .now = 1697811728219, .expect = 2},
      {.ival = 60, .now = 1697812625779, .expect = 42},
      {.ival = 60, .now = 1697812679910, .expect = 31},
      {.ival = 60, .now = 1697811728200, .expect = 21},

  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    unsigned short actual = get_sleep_duration(tc.ival, tc.now);
    ok(tc.expect == actual,
       "Computes sleep duration as %d given interval %d and curr time %ld",
       tc.expect, tc.ival, tc.now);
  }
}

void run_time_tests(void) {
  round_ts_test();
  get_sleep_duration_test();
}
