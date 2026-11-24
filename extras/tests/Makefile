# Unit tests for the COBS library. Run `make test` or `make coverage` (requires gcovr).
CXX      ?= g++
CXXSTD   ?= c++11
WARNINGS := -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Werror
CXXFLAGS ?= -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all
BUILD    := build
ROOT     := ../..
DEPS     := unittest.cpp $(ROOT)/src/cobs.h

.PHONY: test coverage clean

test: $(BUILD)/unit_test
	./$<

$(BUILD)/unit_test: $(DEPS)
	@mkdir -p $(BUILD)
	$(CXX) -std=$(CXXSTD) $(WARNINGS) $(CXXFLAGS) unittest.cpp -o $@

# The library must have 100 % line and branch coverage
coverage: $(DEPS)
	@mkdir -p $(BUILD)/coverage
	$(CXX) -std=$(CXXSTD) $(WARNINGS) -O0 -g --coverage $(CURDIR)/unittest.cpp -o $(BUILD)/coverage/unit_test
	./$(BUILD)/coverage/unit_test
	gcovr --root $(ROOT) --filter '$(ROOT)/src/' --object-directory $(BUILD)/coverage \
	  --txt-metric branch --print-summary --fail-under-line 100 --fail-under-branch 100 \
	  --xml $(BUILD)/coverage/coverage.xml --html-details $(BUILD)/coverage/index.html

clean:
	rm -rf $(BUILD)
