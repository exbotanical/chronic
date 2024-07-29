#ifndef TESTS_H
#define TESTS_H

#define ITER_CASES_TEST(tests, type) \
  for (unsigned int i = 0; i < sizeof(tests) / sizeof(type); i++)

char* setup_test_directory();
void
setup_test_file(const char* dirname, const char* filename, const char* content);
void modify_test_file(
  const char* dirname,
  const char* filename,
  const char* content
);
void cleanup_test_directory(char* dirname);
void cleanup_test_file(char* dirname, char* fname);
int  get_fd(char* fpath);
bool has_filename_comparator(void* el, void* compare_to);

void run_parser_tests(void);
void run_crontab_tests(void);
void run_utils_tests(void);
void run_regexpr_tests(void);

#endif /* TESTS_H */
