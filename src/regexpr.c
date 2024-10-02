#include "regexpr.h"

#include <errno.h>
#include <pcre.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "libutil/libutil.h"
#include "log.h"
#include "panic.h"

static const char *VARIABLE_PATTERN
  = "^([a-zA-Z_-][a-zA-Z0-9_-]*)=\"([^\"]*)\"(?<! )$";

static int ovecsize = 32;
static int ovector[32];  // TODO:

static pthread_once_t init_regex_cache_once = PTHREAD_ONCE_INIT;
static hash_table    *regex_cache;

static void
init_regex_cache (void) {
  regex_cache = ht_init_or_panic(1, NULL);
}

static hash_table *
get_regex_cache (void) {
  pthread_once(&init_regex_cache_once, init_regex_cache);

  return regex_cache;
}

static pcre *
regex_compile (const char *pattern) {
  const char *error;
  int         erroffset;

  pcre *re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
  if (re == NULL) {
    panic("[%s@L%d] PCRE compilation failed at offset %d: %s; errno: %d\n", __func__, __LINE__, erroffset, error, errno);
  }

  return re;
}

static pcre *
regex_cache_get (hash_table *cache, const char *pattern) {
  ht_entry *r = ht_search(cache, pattern);
  if (r) {
    return r->value;
  }

  pcre *re = regex_compile(pattern);
  if (!re) {
    return NULL;
  }

  ht_insert(cache, pattern, re);

  return re;
}

array_t *
regex_matches (pcre *re, char *cmp) {
  int rc = pcre_exec(re, NULL, cmp, strlen(cmp), 0, 0, ovector, ovecsize);
  if (rc >= 0) {
    array_t *matches = array_init_or_panic();

    int i;

    for (i = 0; i < rc; i++) {
      int start = ovector[2 * i];
      int end   = ovector[2 * i + 1];
      int len   = end - start;

      char match[len + 1];
      strncpy(match, cmp + start, len);
      match[len] = '\0';

      array_push_or_panic(matches, s_copy_or_panic(match));
    }

    return matches;
  } else if (rc == PCRE_ERROR_NOMATCH) {
    return NULL;
  }

  printf("matching error on regex\n");

  return NULL;
}

// ??? typedef enum { Match, NoMatch } MatchResult;
bool
match_variable (char *line, hash_table *vars) {
  pcre *re = regex_cache_get(get_regex_cache(), VARIABLE_PATTERN);
  if (!re) {
    panic("[%s@L%d] an error occurred when compiling regex\n", __func__, __LINE__);
  }

  array_t *matches = regex_matches(re, line);
  if (matches && array_size(matches) == 3) {
    ht_insert(vars, s_copy_or_panic(array_get_or_panic(matches, 1)), s_copy_or_panic(array_get_or_panic(matches, 2)));
    array_free(matches, free);

    return true;
  }

  return false;
}
