#include "cli.h"
#include "constants.h"
#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"

// Externs initialized in main
CliOptions opts = {0};

char hostname[SMALL_BUFFER] = "unit_test";
array_t* job_queue = NULL;
const char* job_state_names[] = {0};
#include "libutil/libutil.h"
int main(int argc, char** argv) {
  // plan(118);

  // run_parser_tests();
  // run_regexpr_tests();
  // run_crontab_tests();
  // run_time_tests();

  // done_testing();

  array_t* arr = array_init();

  array_push(arr, "a");
  array_push(arr, "b");
  array_push(arr, "c");

  array_push(arr, "x");
  array_push(arr, "y");
  array_push(arr, "z");

  foreach (arr, i) {
    char* v = array_get(arr, i);
    if (s_equals(v, "x") || s_equals(v, "b")) {
      printf("before rm %d\n", array_size(arr));
      array_remove(arr, i);
      printf("after rm %d\n", array_size(arr));
    }
  }
}
