# Test Makefile for JTC (JThreads/C++)
-include ../../Makefile.rules

# Compiler and flags
CPP_COMPILER ?= g++

# Google Test: try Homebrew first, then user-built location
GTEST_BREW := $(shell brew --prefix googletest 2>/dev/null)
ifdef GTEST_BREW
  GTEST_INCLUDE ?= $(GTEST_BREW)/include
  GTEST_LIB_DIR ?= $(GTEST_BREW)/lib
else
  GTEST_ROOT ?= $(HOME)/Projects/googletest-v1.17.0
  GTEST_INCLUDE ?= $(GTEST_ROOT)/googletest/include
  GTEST_LIB_DIR ?= $(GTEST_ROOT)/manual-arm64
endif

# Include paths
CPPFLAGS := -I../include \
            -I$(GTEST_INCLUDE)
CXXFLAGS := -O0 -g3 -Wall -fmessage-length=0 -std=c++17 -Wno-dynamic-exception-spec
LDFLAGS := -L../linux -L$(GTEST_LIB_DIR) \
           -Wl,-rpath,../linux
LIBS := -ljtc -lgtest -lpthread

# Directories
UNIT_DIR := unit
BUILD_DIR := build

# Test source files
TEST_SOURCES := $(wildcard $(UNIT_DIR)/*.cpp)
TEST_OBJECTS := $(TEST_SOURCES:$(UNIT_DIR)/%.cpp=$(BUILD_DIR)/%.o)

TEST_EXECUTABLE := run_tests

# Targets
.PHONY: all clean run help

all: $(TEST_EXECUTABLE)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile test files
$(BUILD_DIR)/%.o: $(UNIT_DIR)/%.cpp | $(BUILD_DIR)
	@echo 'Building test file: $<'
	$(CPP_COMPILER) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	@echo 'Building test executable: $@'
	$(CPP_COMPILER) $(LDFLAGS) -o "$@" $(TEST_OBJECTS) $(LIBS)
	@echo 'Finished building: $@'
	@echo

run: $(TEST_EXECUTABLE)
	@echo '========================================='
	@echo 'Running JTC Unit Tests'
	@echo '========================================='
	@./$(TEST_EXECUTABLE); \
	EXIT_CODE=$$?; \
	echo; \
	if [ $$EXIT_CODE -eq 0 ]; then \
		echo 'Test run completed.'; \
	else \
		echo 'Test run completed with failures.'; \
		exit $$EXIT_CODE; \
	fi

clean:
	-$(RM) -rf $(BUILD_DIR) $(TEST_EXECUTABLE)
	@echo 'Test artifacts cleaned.'

help:
	@echo 'JTC Test Makefile'
	@echo ''
	@echo 'Available targets:'
	@echo '  all    - Build test executable (default)'
	@echo '  run    - Build and run all tests'
	@echo '  clean  - Remove all build artifacts'
	@echo '  help   - Show this help message'
	@echo ''
	@echo 'Example usage:'
	@echo '  make         # Build tests'
	@echo '  make run     # Build and run tests'
	@echo '  make clean   # Clean build artifacts'
	@echo ''
	@echo 'Run specific test suites:'
	@echo '  ./run_tests --gtest_filter=JTCMutex_Basic.*'

# Include dependency files
-include $(BUILD_DIR)/*.d
