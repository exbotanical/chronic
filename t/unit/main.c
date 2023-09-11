#include "tap.c/tap.h"
#include "tests.h"
int main(int argc, char **argv) {
  plan(5);

  run_parser_tests();

  done_testing();
}
