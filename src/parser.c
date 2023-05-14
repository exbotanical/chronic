#include <pthread.h>

#include "defs.h"

typedef struct {
  char* command;
  time_t curr_date;
  enum { UTC } tz;
} cron_parse_opts;

char* parse_entry(char* entry);
char* parse_expr(char* expr, cron_parse_opts opts);

static hash_table* regex_cache;

static pthread_once_t init_regex_cache_once = PTHREAD_ONCE_INIT;
static void init_regex_cache(void) { regex_cache = ht_init(0 /* TODO: */); }

static hash_set* get_regex_cache(void) {
  pthread_once(&init_regex_cache_once, init_regex_cache);

  return regex_cache;
}

static const char* COMMENT_PATTERN = "^#";
static const char* VARIABLE_PATTERN = "^(.*)=(.*)$";

cron_config parse_str(char* data) {
  cron_config config = {.errors = ht_init(0),
                        .variables = ht_init(0),
                        .expressions = array_init()};

  array_t* blocks = s_split(data, "\n");

  foreach (blocks, i) {
    char* block = array_get(blocks, i);
    char* matches = NULL;
    char* entry = s_trim(block);

    if (strlen(entry) > 0) {
      pcre* re = regex_cache_get(get_regex_cache(), "");
      if (!re) {
        write_to_log("an error occurred when compiling regex\n");
        exit(1);
      }

      if (regex_match(re, COMMENT_PATTERN)) {
        continue;
      } else {
        array_t* matches = regex_matches(re, VARIABLE_PATTERN);
        if (matches) {
          ht_insert(config.variables, array_get(matches, 1),
                    array_get(matches, 2));
        } else {
          char* result = parse_entry(s_concat("0 ", entry));

          array_push(config.expressions, result);
        }
      }
    }
  }

  return config;
}

char* parse_entry(char* entry) {
  array_t* atoms = s_split(entry, " ");
  unsigned int sz = array_size(atoms);

  if (sz == 6) {
    return parse_expr(entry, (cron_parse_opts){});
  } else if (sz > 6) {
    // TODO: err handling
    char* joined = str_join(array_slice(atoms, 0, 6), " ");

    char* command = str_join(array_slice(atoms, 6, array_size(atoms)), " ");

    return parse_expr(joined, (cron_parse_opts){.command = command});
  } else {
    // error
  }
}

cron_config parse_file(char* filepath) {
  return parse_str(read_file(filepath));
}

char* parse_expr(char* expr, cron_parse_opts opts) {
  if (opts.curr_date == NULL) {
    opts.curr_date = to_current_date(opts);
  }
}

char* to_current_date(cron_parse_opts opts) {}

/*

syncdir()
  remove_updates_f()

  for dirinfo in dir:
    if dirinfo.name == .:
      continue
    if dirinfo.name = CRONUPDATE:
      continue
    syncdir(dirinfo, dir)



syncfile(dirinfo, dir)
  maxlines = 256

  open(dirinfo.name)
  if (stat(fileno(handle)) and stat.uuid = this):





while 1:
  if been_five_min(t1, now):
    foreach file in crontabs:
      if file.ts < now - 5s:
        config.ts = time(NULL)

        cron_jobs.push(parse(file))

  foreach job in cron_job:
    if job.ready:
      run(job)

parse(file) {
  data = read(file)
  for line in data:
    if get_time(line)
}
*/
