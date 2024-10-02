#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"
#include "util.h"

void
round_ts_test (void) {
  typedef struct {
    time_t         ts;
    unsigned short unit;
    time_t         expect;
  } TestCase;

  TestCase tests[] = {
    {.ts = 105, .unit = 10,  .expect = 110},
    {.ts = 103, .unit = 10,  .expect = 100},
    {.ts = 50,  .unit = 100, .expect = 100},
    {.ts = 53,  .unit = 1,   .expect = 53 },
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    ok(round_ts(tc.ts, tc.unit) == tc.expect, "Rounds %d to %d given unit %d", tc.ts, tc.unit, tc.expect);
  }
}

void
get_sleep_duration_test () {
  typedef struct {
    unsigned short ival;
    time_t         now;
    unsigned short expect;
  } TestCase;

  TestCase tests[] = {
    {.ival = 5,  .now = 1697811728219, .expect = 1 },
    {.ival = 10, .now = 1697811728219, .expect = 1 },
    {.ival = 30, .now = 1697812811409, .expect = 21},
    {.ival = 30, .now = 1697812854805, .expect = 5 },
    {.ival = 60, .now = 1697811728219, .expect = 1 },
    {.ival = 60, .now = 1697812625779, .expect = 41},
    {.ival = 60, .now = 1697812679910, .expect = 30},
    {.ival = 60, .now = 1697811728200, .expect = 20},
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc           = tests[i];

    unsigned short actual = get_sleep_duration(tc.ival, tc.now);
    ok(
      tc.expect == actual,
      "Computes sleep duration as %d given interval %d and curr time %ld "
      "(actual %ld)",
      tc.expect,
      tc.ival,
      tc.now,
      actual
    );
  }
}

void
get_filenames_test () {
  char* dirname = setup_test_directory();
  setup_test_file(dirname, "file1.txt", NULL);
  setup_test_file(dirname, "file2.txt", NULL);
  setup_test_file(dirname, "file3.txt", NULL);

  array_t* result = get_filenames(dirname);

  ok(result != NULL, "Expect non-NULL result");
  ok(array_size(result) == 3, "Expect 3 files in the test directory");

  // Depending on the OS, the order may be different
  ok(array_includes(result, has_filename_comparator, "file1.txt") == true, "Should contain file1.txt");
  ok(array_includes(result, has_filename_comparator, "file2.txt") == true, "Should contain file2.txt");
  ok(array_includes(result, has_filename_comparator, "file3.txt") == true, "Should contain file3.txt");

  cleanup_test_file(dirname, "file1.txt");
  cleanup_test_file(dirname, "file2.txt");
  cleanup_test_file(dirname, "file3.txt");
  cleanup_test_directory(dirname);
}

void
run_utils_tests (void) {
  round_ts_test();
  get_sleep_duration_test();
  get_filenames_test();
}
