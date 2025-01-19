.PHONY: all test unit_test unit_test_dev integ_test valgrind clean fmt
.DELETE_ON_ERROR:

CC          ?= gcc
AR          ?= ar
FMT         ?= clang-format

INSTALL     := install

INSTALL_DIR ?= /usr/local
MAN_DIR     ?= /usr/share/man/man1
PROG        := chronic
MANPAGE     := $(PROG).1
# TODO: Get from env
PROGVERS    := 0.0.1

UNIT_TARGET := unit_test

SRCDIR      := src
INCDIR      := include
DEPSDIR     := deps
TESTDIR     := t

SRC         := $(shell find $(SRCDIR) -name "*.c")
TESTS       := $(shell find $(TESTDIR) -name "*.c")
SRC_NOMAIN  := $(filter-out $(SRCDIR)/main.c, $(SRC))
TEST_DEPS   := $(wildcard $(DEPSDIR)/libtap/*.c)
DEPS        := $(filter-out $(wildcard $(DEPSDIR)/libtap/*), $(wildcard $(DEPSDIR)/*/*.c))
UNIT_TESTS  := $(wildcard $(TESTDIR)/unit/*.c)

INCLUDES         := -I$(DEPSDIR) -I$(INCDIR)
STRICT           := -Wall -Wextra -Wno-missing-field-initializers \
 -Wno-unused-parameter -Wno-unused-function -Wno-unused-value
CFLAGS           := $(STRICT) $(INCLUDES)
UNIT_TEST_CFLAGS := -DUNIT_TEST $(CFLAGS)
LIBS             := -lm -lpthread -lpcre -luuid

all: $(SRC) $(DEPS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $(PROG)

install: all
	$(shell mkdir -p $(INSTALL_DIR)/bin)
	$(INSTALL) $(PROG) $(INSTALL_DIR)/bin/$(PROG)
	$(INSTALL) $(MANPAGE) $(MAN_DIR)/$(MANPAGE)

uninstall:
	$(shell rm $(INSTALL_DIR)/bin/$(PROG))
	$(shell rm $(MAN_DIR)/$(MANPAGE))

test:
	@$(MAKE) unit_test
	@$(MAKE) integ_test

unit_test: $(UNIT_TESTS) $(DEPS) $(TEST_DEPS) $(SRC_NOMAIN)
	$(CC) $(UNIT_TEST_CFLAGS) $^ $(LIBS) -o $(UNIT_TARGET)
	@./$(UNIT_TARGET)
	@$(MAKE) clean

unit_test_dev:
	ls $(INCDIR)/*.h $(SRCDIR)/*.{h,c} $(TESTDIR)/**/*.{h,c} | entr -s 'make -s unit_test'

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
