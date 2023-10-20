#include "crontab.h"

#include <fcntl.h>
#include <string.h>

#include "job.h"
#include "tap.c/tap.h"
#include "tests.h"
#include "utils.h"

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

static void validate_job(Job* job, Job* expected) {
  is(expected->cmd, job->cmd, "Expect cmd '%s'", expected->cmd);
  is(expected->schedule, job->schedule, "Expect schedule '%s'",
     expected->schedule);
  is(expected->owner_uname, job->owner_uname, "Expect owner_uname '%s'",
     expected->owner_uname);
}

void get_filenames_test() {
  char* dirname = setup_test_directory();
  setup_test_file(dirname, "file1.txt", NULL);
  setup_test_file(dirname, "file2.txt", NULL);
  setup_test_file(dirname, "file3.txt", NULL);

  array_t* result = get_filenames(dirname);

  ok(result != NULL, "Expect non-NULL result");
  ok(array_size(result) == 3, "Expect 3 files in the test directory");

  is(array_get(result, 0), "file3.txt", "Third file should be file3.txt");
  is(array_get(result, 1), "file2.txt", "Second file should be file2.txt");
  is(array_get(result, 2), "file1.txt", "First file should be file1.txt");

  cleanup_test_file(dirname, "file1.txt");
  cleanup_test_file(dirname, "file2.txt");
  cleanup_test_file(dirname, "file3.txt");
  cleanup_test_directory(dirname);
}

void new_crontab_test(void) {
  unsigned int num_expected_jobs = 4;
  char* test_fpath = "./t/fixtures/new_crontab_test";

  int crontab_fd = OK - 1;
  if ((crontab_fd = get_fd(test_fpath)) < OK) {
    fail();
  }

  time_t now = time(NULL);
  char* test_uname = "some_user";

  Job expected_jobs[] = {
      {.cmd = "script.sh",
       .schedule = "*/1 * * * *",
       .owner_uname = test_uname},
      {.cmd = "exec.sh", .schedule = "*/2 * * * *", .owner_uname = test_uname},
      {.cmd = "echo whatever > /tmp/hi",
       .schedule = "*/10 * * * *",
       .owner_uname = test_uname},
      {.cmd = "date", .schedule = "*/15 * * * *", .owner_uname = test_uname}};

  Crontab* ct = new_crontab(crontab_fd, false, now, now, test_uname);
  array_t* jobs = ct->jobs;

  ok(ct->mtime == now, "Expect mtime to equal given mtime (%ld == %ld)",
     ct->mtime, now);
  ok(array_size(jobs) == num_expected_jobs,
     "Expect %d jobs to have been created", num_expected_jobs);

  for (unsigned int i = 0; i < num_expected_jobs; i++) {
    Job expected = expected_jobs[i];
    Job* actual = array_get(jobs, i);

    validate_job(actual, &expected);
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
  time_t now = time(NULL);
  scan_crontabs(db, usr_dir, now);

  ok(db->count == 3, "Two crontab files should have been processed");

  Crontab* ct1 = (Crontab*)ht_get(db, "user1");
  Crontab* ct2 = (Crontab*)ht_get(db, "user2");
  Crontab* ct3 = (Crontab*)ht_get(db, "user3");

  ok(ct1 != NULL, "user1's crontab should exist in the database");
  ok(ct2 != NULL, "user2's crontab should exist in the database");
  ok(ct3 != NULL, "user3's crontab should exist in the database");

  ok(array_size(ct1->jobs) == 1, "user1's crontab has 1 job config");
  ok(array_size(ct2->jobs) == 1, "user2's crontab has 1 job config");
  ok(array_size(ct3->jobs) == 1, "user3's crontab has 1 job config");

  time_t ct1_mtime = ct1->mtime;
  time_t ct3_mtime = ct3->mtime;

  cleanup_test_file(usr_dirname, "user2");
  // Make sure enough time passes for the mtime to be updated
  // (mtime is typically measured in seconds)
  sleep(1);
  modify_test_file(usr_dirname, "user3", "* * * * * echo 'sup dud'\n");
  scan_crontabs(db, usr_dir, now);

  ct1 = (Crontab*)ht_get(db, "user1");
  ct2 = (Crontab*)ht_get(db, "user2");
  ct3 = (Crontab*)ht_get(db, "user3");

  ok(ct1 != NULL, "user1's crontab should exist in the database");
  ok(ct2 == NULL, "user2's crontab was deleted from the database");
  ok(ct3 != NULL, "user3's crontab should exist in the database");

  ok(array_size(ct1->jobs) == 1, "user1's crontab has 1 job config");
  ok(array_size(ct3->jobs) == 2, "user3's crontab has 2 job configs");

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
