#include "crontab.h"

#include <fcntl.h>
#include <string.h>

#include "cronentry.h"
#include "tap.c/tap.h"
#include "tests.h"
#include "util.h"

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

void
new_crontab_test (void) {
  unsigned int num_expected_entries = 4;
  char*        test_fpath           = "./t/fixtures/new_crontab_test";

  int crontab_fd                    = OK - 1;
  if ((crontab_fd = get_fd(test_fpath)) < OK) {
    fail();
  }

  time_t now         = time(NULL);
  char*  test_uname  = "some_user";

  crontab_t* ct      = new_crontab(crontab_fd, false, now, now, test_uname);
  array_t*   entries = ct->entries;

  cron_entry expected_entries[] = {
    {.cmd = "script.sh",               .schedule = "*/1 * * * *",  .parent = ct},
    {.cmd = "exec.sh",                 .schedule = "*/2 * * * *",  .parent = ct},
    {.cmd = "echo whatever > /tmp/hi", .schedule = "*/10 * * * *", .parent = ct},
    {.cmd = "date",                    .schedule = "*/15 * * * *", .parent = ct}
  };

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
  for (unsigned int i = 0; i < ct->vars->capacity; i++) {
    ht_entry* r = ct->vars->entries[i];
    if (!r) {
      continue;
    }
    array_push(expected_envp_entries, s_fmt("%s=%s", r->key, r->value));
  }

  foreach (expected_envp_entries, i) {
    char* expected = array_get(expected_envp_entries, i);
    is(ct->envp[i], expected, "expect to find %s", expected);
  }

  array_free(expected_envp_entries, free);
  close(crontab_fd);
}

void
scan_crontabs_test () {
  char* usr_dirname = setup_test_directory();
  setup_test_file(usr_dirname, "user1", "* * * * * echo 'test1'\n");
  setup_test_file(usr_dirname, "user2", "* * * * * echo 'test2'\n");
  setup_test_file(usr_dirname, "user3", "* * * * * echo 'test3'\n");

  dir_config usr_dir;
  usr_dir.path       = usr_dirname;
  usr_dir.is_root    = false;

  hash_table* db     = ht_init(0, (free_fn*)free_crontab);
  hash_table* new_db = ht_init(0, (free_fn*)free_crontab);
  time_t      now    = time(NULL);
  scan_crontabs(db, new_db, usr_dir, now);
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
  scan_crontabs(db, new_db, usr_dir, now);
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

void
update_db_test (void) {
  char* usr_dirname = setup_test_directory();
  setup_test_file(usr_dirname, "user1", "* * * * * echo 'test1'\n");
  setup_test_file(usr_dirname, "user2", "* * * * * echo 'test2'\n");

  dir_config usr_dir;
  usr_dir.path      = usr_dirname;
  usr_dir.is_root   = false;

  char* sys_dirname = setup_test_directory();
  setup_test_file(sys_dirname, "root1", "* * * * * echo 'test1'\n");
  setup_test_file(sys_dirname, "root2", "* * * * * echo 'test2'\n");

  dir_config sys_dir;
  sys_dir.path    = sys_dirname;
  sys_dir.is_root = true;

  hash_table* db  = ht_init(0, (free_fn*)free_crontab);
  time_t      now = time(NULL);

  db              = update_db(db, now, &usr_dir, &sys_dir, NULL);

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

  time_t ct1_mtime = ct1->mtime;
  time_t ct3_mtime = ct3->mtime;

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

void
run_crontab_tests (void) {
  new_crontab_test();
  scan_crontabs_test();
  update_db_test();
}
