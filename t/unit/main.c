#include "tap.c/tap.h"
#include "tests.h"

int main(int argc, char **argv) {
  plan(101);

  run_parser_tests();
  run_crontab_tests();
  run_time_tests();

  done_testing();
}
