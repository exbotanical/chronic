#include "defs.h"
#include "tap.c/tap.h"

void test_parser(void) {
  cron_config c = parse_file("t/fixtures/crontab.example");
  printf("done\n");
  printf("VAL: %d\n", c.variables->count);
  printf("after print\n");
}

int main(int argc, char const *argv[]) {
  plan(1);

  test_parser();

  done_testing();
}
