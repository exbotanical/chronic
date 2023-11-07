#include <errno.h>
#include <pcre.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "libhash/libhash.h"
#include "libutil/libutil.h"

static const char *COMMENT_PATTERN = "^#";
static const char *VARIABLE_PATTERN =
    "^([a-zA-Z]+)=(?!#)(.*?)$";  // " ^(.*)=(.*)$"

static int ovecsize = 30;
static int ovector[30];  // TODO:

static hash_table *regex_cache;

static pthread_once_t init_regex_cache_once = PTHREAD_ONCE_INIT;
static void init_regex_cache(void) { regex_cache = ht_init(0 /* TODO: */); }

static hash_set *get_regex_cache(void) {
  pthread_once(&init_regex_cache_once, init_regex_cache);

  return regex_cache;
}

pcre *regex_compile(const char *pattern) {
  const char *error;
  int erroffset;

  pcre *re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
  if (re == NULL) {
    printf("PCRE compilation failed at offset %d: %s; errno: %d\n", erroffset,
           error, errno);
    exit(1);
  }

  return re;
}

pcre *regex_cache_get(hash_table *cache, const char *pattern) {
  ht_record *r = ht_search(cache, pattern);
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

bool regex_match(pcre *re, char *cmp) {
  return pcre_exec(re, NULL, cmp, strlen(cmp), 0, 0, ovector, ovecsize) > 0;
}

array_t *regex_matches(pcre *re, char *cmp) {
  int rc = pcre_exec(re, NULL, cmp, strlen(cmp), 0, 0, ovector, ovecsize);
  if (rc >= 0) {
    array_t *matches = array_init();

    int i;

    for (i = 0; i < rc; i++) {
      int start = ovector[2 * i];
      int end = ovector[2 * i + 1];
      int len = end - start;

      char match[len + 1];
      strncpy(match, cmp + start, len);
      match[len] = '\0';

      array_push(matches, s_copy(match));
    }

    return matches;
  } else if (rc == PCRE_ERROR_NOMATCH) {
    printf("nope\n");
    return NULL;
  }

  printf("matching error on regex\n");

  return NULL;
}

void getit() {
  pcre *re = regex_cache_get(get_regex_cache(), VARIABLE_PATTERN);
  if (!re) {
    printf("an error occurred when compiling regex\n");
    exit(1);
  }

  array_t *matches = regex_matches(re, "HELLO=WORLD=1");
  if (matches) {
    printf("MATCHED: key=%s value=%s\n", array_get(matches, 1),
           array_get(matches, 2));
  } else
    printf("NO MATCH\n");
}

int main(int argc, char **argv) { getit(); }
