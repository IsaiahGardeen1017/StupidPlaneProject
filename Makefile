CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2
LDFLAGS ?= -lm

ENGINE_SRC := engine/src/sim.c
WASM_SRC := engine/src/sim_wasm_api.c
ENGINE_INC := engine/include
TEST_SRC := engine/tests/test_sim.c
TEST_BIN := build/test_sim
WASM_OUT_DIR := ui/wasm
WASM_JS := $(WASM_OUT_DIR)/sim.js

.PHONY: all test wasm clean

all: test

build:
	mkdir -p build

$(TEST_BIN): $(ENGINE_SRC) $(TEST_SRC) | build
	$(CC) $(CFLAGS) -I$(ENGINE_INC) $(ENGINE_SRC) $(TEST_SRC) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

wasm: | $(WASM_OUT_DIR)
	@if ! command -v emcc >/dev/null 2>&1; then \
		echo "emcc not found. Install Emscripten to build browser WASM."; \
		exit 1; \
	fi
	emcc $(CFLAGS) -I$(ENGINE_INC) $(ENGINE_SRC) $(WASM_SRC) \
		-s MODULARIZE=1 \
		-s EXPORT_ES6=1 \
		-s ENVIRONMENT=web \
		-s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
		-s EXPORTED_FUNCTIONS='["_sim_wasm_init","_sim_wasm_reset","_sim_wasm_set_control","_sim_wasm_tick","_sim_wasm_get_speed","_sim_wasm_get_altitude","_sim_wasm_get_climb_rate","_sim_wasm_get_pitch_attitude","_sim_wasm_get_roll_attitude","_sim_wasm_get_turn_rate","_sim_wasm_get_aoa_estimate","_sim_wasm_get_throttle","_sim_wasm_get_health","_sim_wasm_get_ammo"]' \
		-o $(WASM_JS)

$(WASM_OUT_DIR):
	mkdir -p $(WASM_OUT_DIR)

clean:
	rm -rf build
