#include "fs.h"

#include "defs.h"
#include "tap.c/tap.h"
#include "tests.h"

void test_process_dir(void) {
  array_t* lines = process_dir("./t/fixtures/valid");

  ok(array_size(lines) == 5);

  is(array_get(lines, 0), "*/10 * * * * /path/to/exe");
  is(array_get(lines, 1), "*/10 * * * * /path/to/exe");
  is(array_get(lines, 2), "0 09-18 * * 1-5 /path/to/exe");
  is(array_get(lines, 3), "5 * * * 1 /bin/sh hello_world # this program");
  is(array_get(lines, 4), "1-5 * * * 1,2,3 /bin/sh goodbye");
}

void run_fs_tests(void) { test_process_dir(); }
