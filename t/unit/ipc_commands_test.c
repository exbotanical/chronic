#include <string.h>

#include "api/commands.h"
#include "crontab.h"
#include "globals.h"
#include "job.h"
#include "proginfo.h"
#include "tests.h"
#include "utils/json.h"
#include "utils/time.h"

#define TIMESTAMP_REGEX \
  "\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}(?:\\.\\d{3})?Z"

#define UUID_REGEX \
  "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"

hash_table* test_db;
char*       usr_dirname;
char*       sys_dirname;

static inline void
setup_test_data (void) {
  db                = NULL;

  char* usr_dirname = setup_test_directory();
  setup_test_file(usr_dirname, "user1", "* * * * * echo 'test1'\n");
  setup_test_file(usr_dirname, "user2", "* * * * * echo 'test2'\n");
  dir_config usr_dir = {.is_root = false, .path = usr_dirname};

  char* sys_dirname  = setup_test_directory();
  setup_test_file(sys_dirname, "root1", "* * * * * echo 'test1'\n");
  setup_test_file(sys_dirname, "root2", "* * * * * echo 'test2'\n");
  dir_config sys_dir = {.is_root = true, .path = sys_dirname};

  hash_table* tmp_db = ht_init(0, (free_fn*)free_crontab);
  time_t      now    = time(NULL);

  test_db            = update_db(tmp_db, now, &usr_dir, &sys_dir, NULL);
}

static inline void
teardown_test_data (void) {
  cleanup_test_file(usr_dirname, "user1");
  cleanup_test_file(usr_dirname, "user2");
  cleanup_test_file(sys_dirname, "root1");
  cleanup_test_file(sys_dirname, "root2");
  cleanup_test_directory(usr_dirname);
  cleanup_test_directory(sys_dirname);
  ht_delete_table(test_db);
}

static void
test_write_jobs_info (void) {
  job_queue = array_init();
  setup_test_data();
  db = test_db;

  HT_ITER_START(db)
  crontab_t* ct = entry->value;
  foreach (ct->entries, i) {
    cron_entry* ce = array_get(ct->entries, i);
    array_push(job_queue, (void*)new_cronjob(ce));
  }
  HT_ITER_END

  buffer_t* buf = buffer_init(NULL);
  write_jobs_info(buf);

  // Shitty approx tests until we setup a proper JSON parser
  // TODO: Proper parser
  const char* ret = buffer_state(buf);

  ok(strlen(ret) > 32, "approx len looks OK");
  match_str(ret, "\"id\":\"" UUID_REGEX "\"", "has valid job id");
  match_str(ret, "\"mailto\":\"root\"", "has mailto for root");
  match_str(ret, "\"mailto\":\"user1\"", "has mailto for user1");
  match_str(ret, "\"mailto\":\"user2\"", "has mailto for user2");
  match_str(ret, "\"state\":\"PENDING\"", "has state");
  match_str(ret, "\"next\":\"" TIMESTAMP_REGEX "\"", "has valid next timestamps");

  buffer_free(buf);
  array_free(job_queue, (free_fn*)free_cronjob);
  teardown_test_data();
}

static void
test_write_crontabs_info (void) {
  setup_test_data();
  db            = test_db;
  buffer_t* buf = buffer_init(NULL);
  write_crontabs_info(buf);

  const char* ret = buffer_state(buf);

  ok(strlen(ret) > 32, "approx len looks OK");
  match_str(ret, "\"schedule\":\"* * * * *\"", "has expected schedule");
  match_str(ret, "\"filepath\":\"\\/tmp\\/.*\\/root1\"", "has expected schedule");
  match_str(ret, "\"filepath\":\"\\/tmp\\/.*\\/root2\"", "has expected schedule");
  match_str(ret, "\"filepath\":\"\\/tmp\\/.*\\/user1\"", "has expected schedule");
  match_str(ret, "\"filepath\":\"\\/tmp\\/.*\\/user2\"", "has expected schedule");
  match_str(ret, "\"owner\":\"root\"", "has expected owner");
  match_str(ret, "\"owner\":\"user1\"", "has expected owner");
  match_str(ret, "\"owner\":\"user2\"", "has expected owner");
  match_str(ret, "\"id\":\"" UUID_REGEX "\"", "has valid entry id");
  match_str(ret, "\"next\":\"" TIMESTAMP_REGEX "\"", "has valid next timestamps");

  buffer_free(buf);
  teardown_test_data();
}

static void
test_write_program_info (void) {
  proginfo.pid = 123;
  struct timespec ts;
  get_time(&ts);
  proginfo.start = &ts;
  memcpy(proginfo.version, "777", 4);

  buffer_t* buf = buffer_init(NULL);
  write_program_info(buf);

  hash_table* ht = ht_init(HT_DEFAULT_CAPACITY, free);

  ok(parse_json(buffer_state(buf), ht) == OK, "is valid JSON");
  eq_str(ht_get(ht, "pid"), "123", "has correct pid");
  eq_str(ht_get(ht, "uptime"), "0 days, 0 hours, 0 minutes, 0 seconds", "has correct uptime");
  eq_str(ht_get(ht, "version"), "777", "has correct version");
  match_str(ht_get(ht, "started_at"), "^" TIMESTAMP_REGEX "$", "is valid timestamp (%s)\n", ht_get(ht, "started_at"));

  buffer_free(buf);
  ht_delete_table(ht);
}

void
run_ipc_commands_test (void) {
  test_write_jobs_info();
  test_write_crontabs_info();
  test_write_program_info();
}
