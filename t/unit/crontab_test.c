#define _ATFILE_SOURCE 1  // For utimensat, AT_FDCWD
#include "crontab.h"

#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "cronentry.h"
#include "tap.c/tap.h"
#include "tests.h"
#include "utils/retval.h"

static void
validate_entry (cron_entry* entry, cron_entry* expected) {
  is(expected->cmd, entry->cmd, "Expect cmd '%s'", expected->cmd);
  is(expected->schedule, entry->schedule, "Expect schedule '%s'", expected->schedule);
  is(
    expected->parent->uname,
    entry->parent->uname,
    "Expect parent->uname '%s'",
    expected->parent->uname
  );
}

static void
new_crontab_test (void) {
  unsigned int num_expected_entries = 4;
  char*        test_fpath           = "./t/fixtures/crontab.party";

  int crontab_fd                    = OK - 1;
  if ((crontab_fd = get_fd(test_fpath)) < OK) {
    fail();
  }

  time_t now         = time(NULL);
  char*  test_uname  = "some_user";

  crontab_t* ct      = new_crontab(crontab_fd, false, now, now, test_uname);
  array_t*   entries = ct->entries;

  // clang-format off
  cron_entry expected_entries[] = {
    { .cmd = "script.sh",               .schedule = "*/1 * * * *",  .parent = ct },
    { .cmd = "exec.sh",                 .schedule = "*/2 * * * *",  .parent = ct },
    { .cmd = "echo whatever > /tmp/hi", .schedule = "*/10 * * * *", .parent = ct },
    { .cmd = "date",                    .schedule = "*/15 * * * *", .parent = ct }
  };
  // clang-format on

  ok(ct->mtime == now, "Expect mtime to equal given mtime (%ld == %ld)", ct->mtime, now);
  ok(array_size(entries) == num_expected_entries, "Expect %d entries to have been created", num_expected_entries);

  for (unsigned int i = 0; i < num_expected_entries; i++) {
    cron_entry  expected = expected_entries[i];
    cron_entry* actual   = array_get(entries, i);

    validate_entry(actual, &expected);
    free(actual);
  }

  ok(ct->vars->count == 3, "has 3 environment variables");
  is(ht_get(ct->vars, "PATH"), "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", "has the PATH environment variable");
  is(ht_get(ct->vars, "SHELL"), "/bin/echo", "has the SHELL environment variable");
  is(ht_get(ct->vars, "KEY"), "VALUE", "has the KEY environment variable");

  array_t* expected_envp_entries = array_init();

  HT_ITER_START(ct->vars)
  array_push(expected_envp_entries, s_fmt("%s=%s", entry->key, entry->value));
  HT_ITER_END

  foreach (expected_envp_entries, i) {
    char* expected = array_get(expected_envp_entries, i);
    is(ct->envp[i], expected, "expect to find %s", expected);
  }

  array_free(expected_envp_entries, free);
  close(crontab_fd);
}

static void
new_crontab_test_2 (void) {
  unsigned int num_expected_entries = 4;
  char*        test_fpath           = "./t/fixtures/crontab.basic";

  int crontab_fd                    = OK - 1;
  if ((crontab_fd = get_fd(test_fpath)) < OK) {
    fail();
  }

  time_t now         = time(NULL);
  char*  test_uname  = "some_user";

  crontab_t* ct      = new_crontab(crontab_fd, false, now, now, test_uname);
  array_t*   entries = ct->entries;

  // clang-format off
  cron_entry expected_entries[] = {
    { .cmd      = "root    cd / && run-parts --report /etc/cron.hourly",
      .schedule = "17 * * * *",
      .parent   = ct
    },
    { .cmd      = "root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.daily )",
      .schedule = "25 6 * * *",
      .parent   = ct
    },
    { .cmd      = "root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.weekly )",
      .schedule = "47 6 * * 7",
      .parent   = ct
    },
    { .cmd      = "root	test -x /usr/sbin/anacron || ( cd / && run-parts --report /etc/cron.monthly )",
      .schedule = "52 6 1 * *",
      .parent   = ct
    }
  };
  // clang-format on

  ok(ct->mtime == now, "Expect mtime to equal given mtime (%ld == %ld)", ct->mtime, now);
  ok(array_size(entries) == num_expected_entries, "Expect %d entries to have been created", num_expected_entries);

  for (unsigned int i = 0; i < num_expected_entries; i++) {
    cron_entry  expected = expected_entries[i];
    cron_entry* actual   = array_get(entries, i);

    validate_entry(actual, &expected);
    free(actual);
  }

  ok(ct->vars->count == 2, "has 2 environment variables");
  is(ht_get(ct->vars, "PATH"), "/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin", "has the PATH environment variable");
  is(ht_get(ct->vars, "SHELL"), "/bin/sh", "has the SHELL environment variable");

  array_t* expected_envp_entries = array_init();

  HT_ITER_START(ct->vars)
  array_push(expected_envp_entries, s_fmt("%s=%s", entry->key, entry->value));
  HT_ITER_END

  foreach (expected_envp_entries, i) {
    char* expected = array_get(expected_envp_entries, i);
    is(ct->envp[i], expected, "expect to find %s", expected);
  }

  array_free(expected_envp_entries, free);
  close(crontab_fd);
}

static void
scan_crontabs_test (void) {
  char* usr_dirname = setup_test_directory();
  setup_test_file(usr_dirname, "user1", "* * * * * echo 'test1'\n");
  setup_test_file(usr_dirname, "user2", "* * * * * echo 'test2'\n");
  setup_test_file(usr_dirname, "user3", "* * * * * echo 'test3'\n");

  dir_config usr_dir = {.is_root = false, .path = usr_dirname};

  hash_table* db     = ht_init(0, (free_fn*)free_crontab);
  hash_table* new_db = ht_init(0, (free_fn*)free_crontab);
  time_t      now    = time(NULL);

  scan_crontabs(db, new_db, &usr_dir, now);
  *db = *new_db;

  ok(db->count == 3, "Three crontab files should have been processed");

  crontab_t* ct1 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user1"));
  crontab_t* ct2 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user2"));
  crontab_t* ct3 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user3"));

  ok(ct1 != NULL, "user1's crontab should exist in the database");
  ok(ct2 != NULL, "user2's crontab should exist in the database");
  ok(ct3 != NULL, "user3's crontab should exist in the database");

  ok(array_size(ct1->entries) == 1, "user1's crontab has 1 entry");
  ok(array_size(ct2->entries) == 1, "user2's crontab has 1 entry");
  ok(array_size(ct3->entries) == 1, "user3's crontab has 1 entry");

  time_t ct1_mtime = ct1->mtime;
  time_t ct3_mtime = ct3->mtime;

  cleanup_test_file(usr_dirname, "user2");
  // Make sure enough time passes for the mtime to be updated
  // (mtime is typically measured in seconds)
  sleep(1);
  modify_test_file(usr_dirname, "user3", "* * * * * echo 'sup dud'\n");

  new_db = ht_init(0, (free_fn*)free_crontab);
  scan_crontabs(db, new_db, &usr_dir, now);
  *db = *new_db;

  ct1 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user1"));
  ct2 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user2"));
  ct3 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user3"));

  ok(ct1 != NULL, "user1's crontab should exist in the database");
  ok(ct2 == NULL, "user2's crontab was deleted from the database");
  ok(ct3 != NULL, "user3's crontab should exist in the database");

  ok(array_size(ct1->entries) == 1, "user1's crontab has 1 entry");
  ok(array_size(ct3->entries) == 2, "user3's crontab has 2 entries");

  ok(ct1_mtime == ct1->mtime, "user1's crontab mtime didn't change");
  ok(ct3_mtime < ct3->mtime, "user3's crontab mtime was updated");

  // Cleanup
  cleanup_test_file(usr_dirname, "user1");
  cleanup_test_file(usr_dirname, "user3");
  cleanup_test_directory(usr_dirname);
  ht_delete_table(db);
}

static void
update_db_test (void) {
  char* usr_dirname = setup_test_directory();
  setup_test_file(usr_dirname, "user1", "* * * * * echo 'test1'\n");
  setup_test_file(usr_dirname, "user2", "* * * * * echo 'test2'\n");
  dir_config usr_dir = {.is_root = false, .path = usr_dirname};

  char* sys_dirname  = setup_test_directory();
  setup_test_file(sys_dirname, "root1", "* * * * * echo 'test1'\n");
  setup_test_file(sys_dirname, "root2", "* * * * * echo 'test2'\n");
  dir_config sys_dir = {.is_root = true, .path = sys_dirname};

  hash_table* db     = ht_init(0, (free_fn*)free_crontab);
  time_t      now    = time(NULL);

  db                 = update_db(db, now, &usr_dir, &sys_dir, NULL);

  ok(db->count == 4, "Four crontab files should have been processed");

  crontab_t* ct1 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user1"));
  crontab_t* ct2 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user2"));
  crontab_t* ct3 = (crontab_t*)ht_get(db, s_fmt("%s/%s", sys_dirname, "root1"));
  crontab_t* ct4 = (crontab_t*)ht_get(db, s_fmt("%s/%s", sys_dirname, "root2"));

  ok(ct1 != NULL, "user1's crontab should exist in the database");
  ok(ct2 != NULL, "user2's crontab should exist in the database");
  ok(ct3 != NULL, "the root1 crontab should exist in the database");
  ok(ct4 != NULL, "the root2 crontab should exist in the database");

  ok(array_size(ct1->entries) == 1, "user1's crontab has 1 entry");
  ok(array_size(ct2->entries) == 1, "user2's crontab has 1 entry");
  ok(array_size(ct3->entries) == 1, "the root1 crontab has 1 entry");
  ok(array_size(ct4->entries) == 1, "the root2 crontab has 1 entry");

  cleanup_test_file(usr_dirname, "user2");
  cleanup_test_file(sys_dirname, "root1");

  // Make sure enough time passes for the mtime to be updated
  // (mtime is typically measured in seconds)
  sleep(1);
  modify_test_file(usr_dirname, "user1", "* * * * * echo 'sup dud'\n");
  modify_test_file(sys_dirname, "root2", "* * * * * echo 'sup dud'\n");

  db  = update_db(db, now, &usr_dir, &sys_dir, NULL);

  ct1 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user1"));
  ct2 = (crontab_t*)ht_get(db, s_fmt("%s/%s", usr_dirname, "user2"));
  ct3 = (crontab_t*)ht_get(db, s_fmt("%s/%s", sys_dirname, "root1"));
  ct4 = (crontab_t*)ht_get(db, s_fmt("%s/%s", sys_dirname, "root2"));

  ok(ct1 != NULL, "user1's crontab should exist in the database");
  ok(ct2 == NULL, "user2's crontab was deleted from the database");
  ok(ct3 == NULL, "the root1 crontab was deleted from the database");
  ok(ct4 != NULL, "the root2 crontab should exist in the database");

  ok(array_size(ct1->entries) == 2, "user1's crontab has 2 entries");
  ok(array_size(ct4->entries) == 2, "the root2 crontab has 2 entries");

  // Cleanup
  cleanup_test_file(usr_dirname, "user1");
  cleanup_test_file(sys_dirname, "root2");
  cleanup_test_directory(usr_dirname);
  cleanup_test_directory(sys_dirname);
  ht_delete_table(db);
}

static void
run_virtual_crontabs_tests (void) {
  char* dirname = setup_test_directory();
  // TODO: must be exec by curr
  setup_test_file(dirname, "1", "doesnt matter");
  setup_test_file(dirname, "2", "doesnt matter either");
  dir_config dir = {.is_root = true, .path = dirname, .cadence = CADENCE_HOURLY};
  char* fpath1   = s_fmt("%s/%s", dirname, "1");
  char* fpath2   = s_fmt("%s/%s", dirname, "2");

  hash_table* db = ht_init(0, (free_fn*)free_crontab);
  db             = update_db(db, time(NULL), &dir, NULL);

  ok(db->count == 2, "Two virtual crontab files should have been processed");

  crontab_t* ct1 = (crontab_t*)ht_get(db, fpath1);
  crontab_t* ct2 = (crontab_t*)ht_get(db, fpath2);

  ok(ct1 != NULL, "a virtual crontab for executable '1' should exist in the database");
  ok(ct2 != NULL, "a virtual crontab for executable '2' should exist in the database");

  ok(array_size(ct1->entries) == 1, "the virtual crontab should have a singleton entry");
  ok(array_size(ct2->entries) == 1, "the virtual crontab should have a singleton entry");

  cron_entry* ce1 = array_get(ct1->entries, 0);
  cron_entry* ce2 = array_get(ct2->entries, 0);

  is(ce1->cmd, fpath1, "uses executable name as cmd");
  is(ce2->cmd, fpath2, "uses executable name as cmd");

  is(ce1->schedule, HOURLY_EXPR, "has a once hourly schedule");
  is(ce2->schedule, HOURLY_EXPR, "has a once hourly schedule");

  cleanup_test_file(dirname, "1");

  db = update_db(db, time(NULL), &dir, NULL);

  ok(db->count == 1, "Only one virtual crontab file exists in the database after deleting one and re-processing");

  free(fpath1);
  free(fpath2);
  cleanup_test_file(dirname, "2");
  cleanup_test_directory(dirname);
}

void
run_crontab_tests (void) {
  new_crontab_test();
  new_crontab_test_2();
  scan_crontabs_test();
  update_db_test();
  run_virtual_crontabs_tests();
}
