#include "cli.h"
#include "globals.h"
#include "libutil/libutil.h"
#include "proginfo.h"
#include "tap.c/tap.h"
#include "tests.h"

// Externs initialized in main
cli_opts   opts = {0};
proginfo_t proginfo;

user_t usr             = {0};

hash_table* db         = NULL;
array_t*    job_queue  = NULL;
array_t*    mail_queue = NULL;

int
main (int _argc, char** _argv) {
  usr.uid   = 0;
  usr.uname = "root";
  usr.root  = true;

  plan(149);

  run_parser_tests();
  run_regexpr_tests();
  run_crontab_tests();
  run_utils_tests();

  done_testing();
}
