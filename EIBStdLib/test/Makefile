# Test Makefile for EIBStdLib
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
  UNAME_S := $(shell uname -s)
  ifeq ($(UNAME_S),Darwin)
    GTEST_LIB_DIR ?= $(GTEST_ROOT)/manual-arm64
  else
    GTEST_LIB_DIR ?= $(GTEST_ROOT)/build/lib
  endif
endif

# Platform-specific rpath: @loader_path on macOS, $ORIGIN on Linux
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  RPATH_ORIGIN := @loader_path
else
  RPATH_ORIGIN := $$ORIGIN
endif

CPPFLAGS := -I../include -I../include/xml -I../../jtc/include -I/usr/include -I$(GTEST_INCLUDE)
CXXFLAGS := -O0 -g3 -Wall -fmessage-length=0 -std=c++17
LDFLAGS := -L../linux -L../../jtc/linux -L$(GTEST_LIB_DIR) -Wl,-rpath,'$(RPATH_ORIGIN)/../linux' -Wl,-rpath,'$(RPATH_ORIGIN)/../../jtc/linux' -Wl,-rpath,'$(GTEST_LIB_DIR)'
LIBS := -lEIBStdLib -ljtc -lgtest -lgtest_main -lpthread

# Directories
UNIT_DIR := unit
FIXTURES_DIR := fixtures
BUILD_DIR := build

# Source files
TEST_SOURCES := $(wildcard $(UNIT_DIR)/*.cpp)
TEST_OBJECTS := $(TEST_SOURCES:$(UNIT_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TEST_DEPS := $(TEST_OBJECTS:%.o=%.d)

# Test executable
TEST_EXECUTABLE := run_tests

# Targets
.PHONY: all clean run help

all: $(TEST_EXECUTABLE)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(UNIT_DIR)/%.cpp | $(BUILD_DIR)
	@echo 'Building test file: $<'
	$(CPP_COMPILER) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo

$(TEST_EXECUTABLE): $(TEST_OBJECTS)
	@echo 'Building test executable: $@'
	$(CPP_COMPILER) $(LDFLAGS) -o "$(TEST_EXECUTABLE)" $(TEST_OBJECTS) $(LIBS)
	@echo 'Finished building: $@'
	@echo

run: $(TEST_EXECUTABLE)
	@echo '========================================='
	@echo 'Running EIBStdLib Unit Tests'
	@echo '========================================='
	@DYLD_LIBRARY_PATH=../linux:../../jtc/linux ./$(TEST_EXECUTABLE); \
	EXIT_CODE=$$?; \
	echo; \
	if [ $$EXIT_CODE -eq 0 ]; then \
		echo 'Test run completed.'; \
	else \
		echo 'Test run completed with failures.'; \
		exit $$EXIT_CODE; \
	fi

clean:
	-$(RM) $(BUILD_DIR) $(TEST_EXECUTABLE)
	@echo 'Test artifacts cleaned.'

help:
	@echo 'EIBStdLib Test Makefile'
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

# Include dependency files
-include $(TEST_DEPS)
