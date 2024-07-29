#include "cli.h"
#include "constants.h"
#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"

// Externs initialized in main
cli_opts opts                 = {0};

char hostname[SMALL_BUFFER]   = "unit_test";

user_t usr                    = {0};

array_t* job_queue            = NULL;
array_t* mail_queue           = NULL;

const char* job_state_names[] = {0};

int
main (int argc, char** argv) {
  usr.uid   = 0;
  usr.uname = "root";
  usr.root  = true;

  plan(118);

  run_parser_tests();
  run_regexpr_tests();
  run_crontab_tests();
  run_utils_tests();

  done_testing();
}
