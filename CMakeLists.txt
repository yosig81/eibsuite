cmake_minimum_required(VERSION 3.20)
project(eibsuite VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)

# Note: C++ standard is NOT set globally because the library code (jtc,
# EIBStdLib) predates C++17 and uses identifiers (e.g. 'byte') that
# conflict with std::byte when compiled with C++17 + 'using namespace std'.
# Test targets set CXX_STANDARD 17 explicitly.

# Shared libraries get proper rpath automatically on all platforms
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# Place all build outputs together so executables can find libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build options (replaces Kconfig CONFIG_* flags)
option(BUILD_GSM            "Build GSM library"               OFF)
option(BUILD_EIB_SERVER     "Build EIBServer"                 ON)
option(BUILD_AMX_SERVER     "Build AMXServer"                 OFF)
option(BUILD_SMS_SERVER     "Build SMSServer"                 OFF)
option(BUILD_RELAY_SERVER   "Build EIBRelay"                  OFF)
option(BUILD_GENERIC_TEMPLATE "Build EIBGenericTemplate"      OFF)
option(BUILD_EMULATOR       "Build Emulator-ng"               ON)
option(BUILD_TESTS          "Build unit and integration tests" ON)

# Architecture define — auto-detect from target processor
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
    add_compile_definitions(PPC)
else()
    add_compile_definitions(X86)
endif()

# pthreads — works on Linux, macOS, and cross-compile
find_package(Threads REQUIRED)

# GTest via FetchContent — downloaded and built automatically
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.17.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    include(GoogleTest)
endif()

# Libraries (order matters — dependencies first)
add_subdirectory(jtc)
add_subdirectory(EIBStdLib)

if(BUILD_GSM)
    add_subdirectory(GSM)
endif()

# Executables
if(BUILD_EIB_SERVER)
    add_subdirectory(EIBServer)
endif()
if(BUILD_AMX_SERVER)
    add_subdirectory(AMXServer)
endif()
if(BUILD_SMS_SERVER)
    if(NOT BUILD_GSM)
        message(FATAL_ERROR "BUILD_SMS_SERVER requires BUILD_GSM=ON")
    endif()
    add_subdirectory(SMSServer)
endif()
if(BUILD_RELAY_SERVER)
    add_subdirectory(EIBRelay)
endif()
if(BUILD_GENERIC_TEMPLATE)
    add_subdirectory(EIBGenericTemplate)
endif()
if(BUILD_EMULATOR)
    add_subdirectory(Emulator-ng)
endif()
