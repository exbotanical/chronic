#include "regexpr.h"

#include "libutil/libutil.h"
#include "tap.c/tap.h"
#include "tests.h"

static void
match_variable_test (void) {
  typedef struct {
    char* key;
    char* value;
    char* invalid_str;
  } TestCase;

  TestCase tests[] = {
    {.key = "A", .value = "X", .invalid_str = NULL},
    {.key         = "PATH",
     .value       = "/usr/src/emsdk:/usr/src/emsdk/upstream/emscripten:/home/"
                    "goldmund/.config/nvm/versions/node/v18.16.0/bin:/usr/local/"
                    "sbin:/usr/local/bin:/usr/bin:/usr/lib/jvm/default/bin:/usr/"
                    "bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl:/home/"
                    "goldmund/.local/bin:/usr/local/go/bin:/usr/lib/jvm/default:/"
                    "usr/local/apache-maven/apache-maven-3.8.6/bin:/home/goldmund/"
                    ".local/share/pnpm:/home/goldmund/.cargo/bin", .invalid_str = NULL},
    {.key = "SOME_TRICKY_VAR", .value = "WITH SPACES", .invalid_str = NULL},
    {.key = "ANOTHER_TRICKY_VAR", .value = "WITH=EQ", .invalid_str = NULL},
    {.key = "A_THIRD_TRICKY_VAR", .value = "WITH SPACES AND ' SINGLEQUOT", .invalid_str = NULL},
    {.key = "EMPTY", .value = "", .invalid_str = NULL},
    {.invalid_str = "SOME_INVALID_VAR"},
    {.invalid_str = ""},
    {.invalid_str = "str=\"invalid"},
    {.invalid_str = "~=\"valid_val_but_invalid_key\""}
  };

  hash_table* ht = ht_init(0, NULL);

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];
    if (tc.invalid_str) {
      char* input = s_copy(tc.invalid_str);

      int before  = ht->count;
      match_variable(input, ht);
      int after = ht->count;

      ok(before == after, "doesn't update the hash table when no variable");

      free(input);
    } else {
      char* input = s_fmt("%s=\"%s\"", s_copy(tc.key), s_copy(tc.value));

      match_variable(input, ht);

      is(ht_get(ht, tc.key), tc.value, "parses the variable and updates the hash table");
    }
  }
}

static void
match_string_test (void) {
  typedef struct {
    const char* input;
    const char* regex;
    bool        expect_match;
  } TestCase;

  TestCase tests[] = {
    {.input = "crontabs", .regex = "crontabs", .expect_match = true },
    {.input = "whatever", .regex = "^w.*",     .expect_match = true },
    {.input = "whatever", .regex = "crontabs", .expect_match = false},
    {.input = "whatever", .regex = "^w.*e$",   .expect_match = false},
  };

  ITER_CASES_TEST(tests, TestCase) {
    TestCase tc = tests[i];

    bool actual = match_string(tc.input, tc.regex);
    ok(actual == tc.expect_match, "should return expected match result");
  }
}

void
run_regexpr_tests (void) {
  match_variable_test();
  match_string_test();
}
