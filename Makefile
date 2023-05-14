CC ?= gcc
AR ?= ar
LINTER ?= clang-format

SRCDIR := src
DEPSDIR := deps
TESTDIR := t

TARGET := chronic
TEST_TARGET := test

SRC := $(wildcard $(SRCDIR)/*.c)
TEST_DEPS := $(wildcard $(DEPSDIR)/tap.c/*.c)
DEPS := $(filter-out $(wildcard $(DEPSDIR)/tap.c/*), $(wildcard $(DEPSDIR)/*/*.c))

CFLAGS := -I$(DEPSDIR) -Wall -Wextra -pedantic
LIBS := -lm -lpcre

TESTS := $(wildcard $(TESTDIR)/*.c)

$(TARGET):
	$(CC) $(CFLAGS) $(SRC) $(DEPS) $(LIBS) -Ideps -Isrc -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)$(TEST_TARGET)

test:
	$(foreach test,$(TESTS),					  																											\
		$(MAKE) .compile_test file=$(test); 																										\
		printf "\033[1;32m\nRunning test $(patsubst $(TESTDIR)/%,%,$(test))...\n==================\n\033[0m";	\
		./test;\
 	)
	rm $(TEST_TARGET)

.compile_test:
	$(CC) $(CFLAGS) $(file) $(filter-out $(SRCDIR)/main.c, $(SRC)) $(DEPS) $(TEST_DEPS) $(LIBS) -Ideps -Isrc -o $(TEST_TARGET)


lint:
	$(LINTER) -i $(wildcard $(SRCDIR)/*) $(wildcard $(TESTDIR)/*)

.PHONY: clean test .compile_test lint
