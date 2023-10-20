#include "tap.c/tap.h"
#include "tests.h"

int main(int argc, char **argv) {
  plan(86);

  run_parser_tests();
  run_fs_tests();
  run_time_tests();

  done_testing();
}
