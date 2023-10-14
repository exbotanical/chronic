CC ?= gcc
AR ?= ar
LINTER ?= clang-format

PROG := chronic
UNIT_TARGET := unit_test

SRCDIR := src
DEPSDIR := deps
TESTDIR := t

SRC := $(wildcard $(SRCDIR)/*.c)
TEST_DEPS := $(wildcard $(DEPSDIR)/tap.c/*.c)
DEPS := $(filter-out $(wildcard $(DEPSDIR)/tap.c/*), $(wildcard $(DEPSDIR)/*/*.c))

CFLAGS := -I$(DEPSDIR) -Wall -Wextra -pedantic
LIBS := -lm -lpcre -luuid

TESTS := $(wildcard $(TESTDIR)/*.c)

$(PROG):
	$(CC) $(CFLAGS) $(SRC) $(DEPS) $(LIBS) -Ideps -Isrc -o $(PROG)

# Use make -s test 2>/dev/null to see only test results
test:
	$(MAKE) unit_test

unit_test:
	$(CC) $(wildcard $(TESTDIR)/unit/*.c) $(TEST_DEPS) $(DEPS) $(filter-out $(SRCDIR)/main.c, $(SRC)) -I$(SRCDIR) -I$(DEPSDIR) $(LIBS) -o $(UNIT_TARGET)
	./$(UNIT_TARGET)
	$(MAKE) clean

clean:
	rm -f $(UNIT_TARGET) $(PROG) .log

lint:
	$(LINTER) -i $(SRC) $(wildcard $(TESTDIR)/*/*.c)

.PHONY: test unit_test clean lint
