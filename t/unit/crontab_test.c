#include "crontab.h"

#include <fcntl.h>
#include <string.h>

#include "cronentry.h"
#include "tap.c/tap.h"
#include "tests.h"
#include "util.h"

static char* setup_test_directory() {
  char template[] = "/tmp/tap_test_dir.XXXXXX";
  // Need to copy because mkdtemp overwrites ^ which is locally scoped
  char* dirname = s_copy(mkdtemp(template));

  if (dirname == NULL) {
    perror("mkdtemp failed: ");
    fail();
  }

  return dirname;
}

static void setup_test_file(const char* dirname, const char* filename,
                            const char* content) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dirname, filename);

  int fd = open(path, O_CREAT | O_WRONLY, 0644);
  if (fd == -1) {
    perror("Failed to create test crontab file");
    exit(EXIT_FAILURE);
  }

  if (content) {
    write(fd, content, strlen(content));
  }

  close(fd);
}

static void modify_test_file(const char* dirname, const char* filename,
                             const char* content) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dirname, filename);

  int fd = open(path, O_APPEND | O_WRONLY);
  if (fd == -1) {
    perror("Failed to open the test crontab file for modification");
    exit(EXIT_FAILURE);
  }

  write(fd, content, strlen(content));

  close(fd);
}

static void cleanup_test_directory(char* dirname) { rmdir(dirname); }

static void cleanup_test_file(char* dirname, char* fname) {
  char path[256];
  snprintf(path, sizeof(path), "%s/%s", dirname, fname);
  unlink(path);
}

// TODO: close?
static int get_fd(char* fpath) { return open(fpath, O_RDONLY | O_NONBLOCK, 0); }

static void validate_entry(CronEntry* entry, CronEntry* expected) {
  is(expected->cmd, entry->cmd, "Expect cmd '%s'", expected->cmd);
  is(expected->schedule, entry->schedule, "Expect schedule '%s'",
     expected->schedule);
  is(expected->owner_uname, entry->owner_uname, "Expect owner_uname '%s'",
     expected->owner_uname);
}

static bool has_filename_comparator(void* el, void* compare_to) {
  return s_equals((char*)el, compare_to);
}

void get_filenames_test() {
  char* dirname = setup_test_directory();
  setup_test_file(dirname, "file1.txt", NULL);
  setup_test_file(dirname, "file2.txt", NULL);
  setup_test_file(dirname, "file3.txt", NULL);

  array_t* result = get_filenames(dirname);

  ok(result != NULL, "Expect non-NULL result");
  ok(array_size(result) == 3, "Expect 3 files in the test directory");

  // Depending on the OS, the order may be different
  ok(array_includes(result, has_filename_comparator, "file1.txt") == true,
     "Should contain file1.txt");
  ok(array_includes(result, has_filename_comparator, "file2.txt") == true,
     "Should contain file2.txt");
  ok(array_includes(result, has_filename_comparator, "file3.txt") == true,
     "Should contain file3.txt");

  cleanup_test_file(dirname, "file1.txt");
  cleanup_test_file(dirname, "file2.txt");
  cleanup_test_file(dirname, "file3.txt");
  cleanup_test_directory(dirname);
}

void new_crontab_test(void) {
  unsigned int num_expected_entries = 4;
  char* test_fpath = "./t/fixtures/new_crontab_test";

  int crontab_fd = OK - 1;
  if ((crontab_fd = get_fd(test_fpath)) < OK) {
    fail();
  }

  time_t now = time(NULL);
  char* test_uname = "some_user";

  CronEntry expected_entries[] = {
      {.cmd = "script.sh",
       .schedule = "*/1 * * * *",
       .owner_uname = test_uname},
      {.cmd = "exec.sh", .schedule = "*/2 * * * *", .owner_uname = test_uname},
      {.cmd = "echo whatever > /tmp/hi",
       .schedule = "*/10 * * * *",
       .owner_uname = test_uname},
      {.cmd = "date", .schedule = "*/15 * * * *", .owner_uname = test_uname}};

  Crontab* ct = new_crontab(crontab_fd, false, now, now, test_uname);
  array_t* entries = ct->entries;

  ok(ct->mtime == now, "Expect mtime to equal given mtime (%ld == %ld)",
     ct->mtime, now);
  ok(array_size(entries) == num_expected_entries,
     "Expect %d entries to have been created", num_expected_entries);

  for (unsigned int i = 0; i < num_expected_entries; i++) {
    CronEntry expected = expected_entries[i];
    CronEntry* actual = array_get(entries, i);

    validate_entry(actual, &expected);
    free(actual);
  }
}

void test_scan_crontabs() {
  // Test Setup: Create some crontab files with known content.
  char* usr_dirname = setup_test_directory();
  setup_test_file(usr_dirname, "user1", "* * * * * echo 'test1'\n");
  setup_test_file(usr_dirname, "user2", "* * * * * echo 'test2'\n");
  setup_test_file(usr_dirname, "user3", "* * * * * echo 'test3'\n");

  DirConfig usr_dir;
  usr_dir.name = usr_dirname;
  usr_dir.is_root = false;

  hash_table* db = ht_init(0);
  hash_table* new_db = ht_init(0);
  time_t now = time(NULL);
  scan_crontabs(db, new_db, usr_dir, now);
  *db = *new_db;

  ok(db->count == 3, "Two crontab files should have been processed");

  Crontab* ct1 = (Crontab*)ht_get(db, "user1");
  Crontab* ct2 = (Crontab*)ht_get(db, "user2");
  Crontab* ct3 = (Crontab*)ht_get(db, "user3");

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

  new_db = ht_init(0);
  scan_crontabs(db, new_db, usr_dir, now);
  *db = *new_db;

  ct1 = (Crontab*)ht_get(db, "user1");
  ct2 = (Crontab*)ht_get(db, "user2");
  ct3 = (Crontab*)ht_get(db, "user3");

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
  // TODO: free memory, cleanup hash table, etc...
}

void run_fs_tests(void) {
  get_filenames_test();
  new_crontab_test();
  test_scan_crontabs();
}
