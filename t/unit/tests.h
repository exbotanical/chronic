#ifndef TESTS_H
#define TESTS_H

#define ITER_CASES_TEST(tests, type) \
  for (unsigned int i = 0; i < sizeof(tests) / sizeof(type); i++)

void run_parser_tests(void);
void run_fs_tests(void);
void run_time_tests(void);

#endif /* TESTS_H */