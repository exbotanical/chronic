#include "cli.h"
#include "constants.h"
#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"

// Externs initialized in main
CliOptions opts               = {0};

char hostname[SMALL_BUFFER]   = "unit_test";

array_t* job_queue            = NULL;
array_t* mail_queue           = NULL;

const char* job_state_names[] = {0};

int
main (int argc, char** argv) {
  plan(118);

  run_parser_tests();
  run_regexpr_tests();
  run_crontab_tests();
  run_time_tests();

  done_testing();
}
