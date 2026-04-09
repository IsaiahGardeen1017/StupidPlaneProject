CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
LDFLAGS ?= -lm

ENGINE_SRC := engine/src/sim.c
ENGINE_INC := engine/include
TEST_SRC := engine/tests/test_sim.c
TEST_BIN := build/test_sim

.PHONY: all test clean

all: test

build:
	mkdir -p build

$(TEST_BIN): $(ENGINE_SRC) $(TEST_SRC) | build
	$(CC) $(CFLAGS) -I$(ENGINE_INC) $(ENGINE_SRC) $(TEST_SRC) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf build
