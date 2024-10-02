CC ?= gcc
AR ?= ar
LINTER ?= clang-format

PROG := chronic
UNIT_TARGET := unit_test

SRCDIR := src
INCDIR := include
DEPSDIR := deps
TESTDIR := t

SRC := $(wildcard $(SRCDIR)/*.c)
TEST_DEPS := $(wildcard $(DEPSDIR)/tap.c/*.c)
DEPS := $(filter-out $(wildcard $(DEPSDIR)/tap.c/*), $(wildcard $(DEPSDIR)/*/*.c))

# TODO: isystem?
CFLAGS := -I$(DEPSDIR) -I$(INCDIR) -Wall -Wextra -pedantic
UNIT_TEST_CFLAGS := -DUNIT_TEST -I$(DEPSDIR) -I$(INCDIR)
LIBS := -lm -lpthread -lpcre -luuid

TESTS := $(wildcard $(TESTDIR)/*.c)

$(PROG):
	$(CC) $(CFLAGS) $(SRC) $(DEPS) $(LIBS) -o $(PROG)

test:
	$(MAKE) unit_test

unit_test:
	$(CC) $(UNIT_TEST_CFLAGS) $(wildcard $(TESTDIR)/unit/*.c) $(TEST_DEPS) $(DEPS) $(filter-out $(SRCDIR)/main.c, $(SRC)) $(LIBS) -o $(UNIT_TARGET)
	./$(UNIT_TARGET)
	$(MAKE) clean

integ_test: $(PROG)
	./$(TESTDIR)/integ/utils/run.bash
	$(MAKE) clean

valgrind: CFLAGS += -g -O0 -DVALGRIND
valgrind: $(PROG)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./$(PROG)
	$(MAKE) clean

clean:
	rm -f $(UNIT_TARGET) $(PROG) .log*
# TODO: cleanup Makefile
lint:
	$(LINTER) -i $(SRC) $(wildcard $(TESTDIR)/*/*.c)

.PHONY: test unit_test integ_test valgrind clean lint
