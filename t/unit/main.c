#include "tap.c/tap.h"
#include "tests.h"

int main(int argc, char **argv) {
  plan(11);

  run_parser_tests();
  run_fs_tests();

  done_testing();
}
