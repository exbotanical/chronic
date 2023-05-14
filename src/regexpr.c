#include "defs.h"

static int ovecsize = 30;
static int ovector[30];  // TODO:

pcre *regex_compile(const char *pattern) {
  const char *error;
  int erroffset;

  pcre *re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
  if (re == NULL) {
    write_to_log("PCRE compilation failed at offset %d: %s; errno: %d\n",
                 erroffset, error, errno);
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

      array_push(matches, match);
    }

    return matches;
  } else if (rc == PCRE_ERROR_NOMATCH) {
    return NULL;
  }

  write_to_log("matching error on regex\n");

  return NULL;
}
