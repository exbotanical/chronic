#include "cli.h"
#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"

// Externs initialized in main
CliOptions opts = {0};

array_t* job_queue = NULL;

int main(int argc, char** argv) {
  plan(101);

  // run_parser_tests();
  run_regexpr_tests();
  // run_crontab_tests();
  // run_time_tests();

  done_testing();
}
