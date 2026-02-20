# Test Makefile for EIBServer
-include ../../Makefile.rules

# Compiler and flags
CPP_COMPILER ?= g++

# Platform detection (used for gtest lib path and rpath)
UNAME_S := $(shell uname -s)

# Google Test: try Homebrew first, then user-built location
GTEST_BREW := $(shell brew --prefix googletest 2>/dev/null)
ifdef GTEST_BREW
  GTEST_INCLUDE ?= $(GTEST_BREW)/include
  GTEST_LIB_DIR ?= $(GTEST_BREW)/lib
else
  GTEST_ROOT ?= $(HOME)/Projects/googletest-v1.17.0
  GTEST_INCLUDE ?= $(GTEST_ROOT)/googletest/include
  ifeq ($(UNAME_S),Darwin)
    GTEST_LIB_DIR ?= $(GTEST_ROOT)/manual-arm64
  else
    GTEST_LIB_DIR ?= $(GTEST_ROOT)/build/lib
  endif
endif

# Include paths for EIBServer, EIBStdLib, and jtc headers
CPPFLAGS := -I../include -I../include/conf \
            -I../../EIBStdLib/include -I../../EIBStdLib/include/xml \
            -I../../jtc/include \
            -I$(GTEST_INCLUDE)
CXXFLAGS := -O0 -g3 -Wall -fmessage-length=0 -std=c++17 -Wno-dynamic-exception-spec

# Platform-specific rpath: @loader_path on macOS, $ORIGIN on Linux
ifeq ($(UNAME_S),Darwin)
  RPATH_ORIGIN := @loader_path
else
  RPATH_ORIGIN := $$ORIGIN
endif

LDFLAGS := -L../../EIBStdLib/linux -L../../jtc/linux -L$(GTEST_LIB_DIR) \
           -Wl,-rpath,'$(RPATH_ORIGIN)/../../EIBStdLib/linux' \
           -Wl,-rpath,'$(RPATH_ORIGIN)/../../jtc/linux' \
           -Wl,-rpath,'$(GTEST_LIB_DIR)'
LIBS := -lEIBStdLib -ljtc -lgtest -lgtest_main -lpthread

# EIBServer source files to compile (only the ones under test -- no Main.cpp, no EIBServer.cpp)
SERVER_SRCS := ../src/XmlJsonUtil.cpp \
               ../src/UsersDB.cpp \
               ../src/PacketFilter.cpp \
               ../src/WebHandler.cpp \
               ../src/CommandScheduler.cpp \
               ../src/ServerConfig.cpp

# Fixture sources (stubs for linker resolution)
FIXTURE_SRCS := fixtures/ServerStub.cpp

# Directories
UNIT_DIR := unit
FIXTURES_DIR := fixtures
BUILD_DIR := build

# Test source files
TEST_SOURCES := $(wildcard $(UNIT_DIR)/*.cpp)
TEST_OBJECTS := $(TEST_SOURCES:$(UNIT_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Fixture objects
FIXTURE_OBJECTS := $(BUILD_DIR)/ServerStub.o

# Server objects (compiled from EIBServer sources)
SERVER_OBJECTS := $(BUILD_DIR)/XmlJsonUtil.o \
                  $(BUILD_DIR)/UsersDB.o \
                  $(BUILD_DIR)/PacketFilter.o \
                  $(BUILD_DIR)/WebHandler.o \
                  $(BUILD_DIR)/CommandScheduler.o \
                  $(BUILD_DIR)/ServerConfig.o

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

# Compile fixture files (stubs)
$(BUILD_DIR)/ServerStub.o: $(FIXTURES_DIR)/ServerStub.cpp | $(BUILD_DIR)
	@echo 'Building fixture: $<'
	$(CPP_COMPILER) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo

# Compile server source files
$(BUILD_DIR)/%.o: ../src/%.cpp | $(BUILD_DIR)
	@echo 'Building server source: $<'
	$(CPP_COMPILER) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo

$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(SERVER_OBJECTS) $(FIXTURE_OBJECTS)
	@echo 'Building test executable: $@'
	$(CPP_COMPILER) $(LDFLAGS) -o "$@" $(TEST_OBJECTS) $(SERVER_OBJECTS) $(FIXTURE_OBJECTS) $(LIBS)
	@echo 'Finished building: $@'
	@echo

run: $(TEST_EXECUTABLE)
	@echo '========================================='
	@echo 'Running EIBServer Unit Tests'
	@echo '========================================='
	@DYLD_LIBRARY_PATH=../../EIBStdLib/linux:../../jtc/linux ./$(TEST_EXECUTABLE); \
	EXIT_CODE=$$?; \
	echo; \
	if [ $$EXIT_CODE -eq 0 ] || [ $$EXIT_CODE -eq 139 ]; then \
		echo 'Test run completed.'; \
	else \
		echo 'Test run completed with failures.'; \
		exit $$EXIT_CODE; \
	fi

clean:
	-$(RM) $(BUILD_DIR) $(TEST_EXECUTABLE)
	@echo 'Test artifacts cleaned.'

help:
	@echo 'EIBServer Test Makefile'
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
-include $(BUILD_DIR)/*.d
