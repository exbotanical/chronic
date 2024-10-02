.PHONY: all test unit_test integ_test valgrind clean fmt
.DELETE_ON_ERROR:

CC          ?= gcc
AR          ?= ar
FMT         ?= clang-format

PROG        := chronic
UNIT_TARGET := unit_test

SRCDIR      := src
INCDIR      := include
DEPSDIR     := deps
TESTDIR     := t

SRC         := $(wildcard $(SRCDIR)/*.c)
TESTS       := $(wildcard $(TESTDIR)/*/*.c)
SRC_NOMAIN  := $(filter-out $(SRCDIR)/main.c, $(SRC))
TEST_DEPS   := $(wildcard $(DEPSDIR)/tap.c/*.c)
DEPS        := $(filter-out $(wildcard $(DEPSDIR)/tap.c/*), $(wildcard $(DEPSDIR)/*/*.c))
UNIT_TESTS  := $(wildcard $(TESTDIR)/unit/*.c)

# TODO: isystem?
INCLUDES         := -I$(DEPSDIR) -I$(INCDIR)
CFLAGS           := -Wall -Wextra -pedantic $(INCLUDES)
UNIT_TEST_CFLAGS := -DUNIT_TEST $(CFLAGS)
LIBS             := -lm -lpthread -lpcre -luuid

all: $(SRC) $(DEPS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $(PROG)

test:
	@$(MAKE) unit_test
	@$(MAKE) integ_test

unit_test: $(UNIT_TESTS) $(DEPS) $(TEST_DEPS) $(SRC_NOMAIN)
	$(CC) $(UNIT_TEST_CFLAGS) $^ $(LIBS) -o $(UNIT_TARGET)
	@./$(UNIT_TARGET)
	@$(MAKE) clean

integ_test: all
	@./$(TESTDIR)/integ/utils/run.bash
	@$(MAKE) clean

valgrind: CFLAGS += -g -O0 -DVALGRIND
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -s ./$(PROG)
	@$(MAKE) clean

clean:
	@rm -f $(UNIT_TARGET) $(PROG) .log*

fmt:
	$(FMT) -i $(SRC) $(TESTS)
