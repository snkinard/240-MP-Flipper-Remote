CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic

.DEFAULT_GOAL := test

TEST_BINARY := build/remote_input_model_test
TEST_SOURCES := remote_input_model.c tests/remote_input_model_test.c
UFBT_TOOLCHAIN := ./scripts/ufbt-toolchain.sh

.PHONY: setup verify test lint build launch

setup:
	$(UFBT_TOOLCHAIN) setup

verify:
	$(UFBT_TOOLCHAIN) verify

test: $(TEST_BINARY)
	./$(TEST_BINARY)

lint:
	$(UFBT_TOOLCHAIN) lint

build:
	$(UFBT_TOOLCHAIN) build

launch:
	$(UFBT_TOOLCHAIN) launch

$(TEST_BINARY): $(TEST_SOURCES) remote_input_model.h
	mkdir -p build
	$(CC) $(CFLAGS) -I. $(TEST_SOURCES) -o $(TEST_BINARY)
