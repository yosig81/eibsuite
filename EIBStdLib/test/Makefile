# Test Makefile for EIBStdLib
-include ../../Makefile.rules

# Compiler and flags
CPP_COMPILER ?= g++
CPPFLAGS := -I../include -I../include/xml -I../../jtc/include -I/usr/include
CXXFLAGS := -O0 -g3 -Wall -fmessage-length=0 -std=c++11
LDFLAGS := -L../linux -L../../jtc/linux -Wl,-rpath,../linux -Wl,-rpath,../../jtc/linux
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
	@./$(TEST_EXECUTABLE)
	@echo
	@echo 'Test run completed.'

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
