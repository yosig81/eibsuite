# EIBSuite

KNX/EIB home automation suite.

## Building

### Prerequisites

- CMake 3.14+
- C/C++ compiler (GCC, Clang, or Apple Clang)
- Google Test is fetched automatically via CMake FetchContent

### Configure and Build

```bash
# Configure (one-time, from repo root)
cmake -B build

# Build everything
cmake --build build

# Build a specific target
cmake --build build --target EIBServer
```

### Cross-Compile for Raspberry Pi

```bash
cmake -B build-pi --toolchain cmake/toolchains/raspberry-pi.cmake
cmake --build build-pi
```

### Running Tests

```bash
# Run all tests
cd build && ctest

# Run tests in parallel
cd build && ctest -j4

# Run a specific test binary directly
build/bin/eibserver_tests
build/bin/eibserver_integration_tests --gtest_filter="BusMonitorTest.*"
```

### Output Locations

Binaries are placed in `build/bin/`, libraries in `build/lib/`.

| Target | Binary |
|--------|--------|
| EIBServer | `build/bin/EIBServer` |
| Emulator-ng | `build/bin/Emulator-ng` |
| AMXServer | `build/bin/AMXServer` |
| SMSServer | `build/bin/SMSServer` |
| EIBRelay | `build/bin/EIBRelay` |
| EIBGenericTemplate | `build/bin/EIBGenericTemplate` |

| Library | Path |
|---------|------|
| jtc | `build/lib/libjtc.{so,dylib}` |
| EIBStdLib | `build/lib/libEIBStdLib.{so,dylib}` |
| GSM | `build/lib/libGSM.{so,dylib}` |

| Tests | Binary |
|-------|--------|
| jtc unit tests | `build/bin/jtc_tests` |
| EIBStdLib unit tests | `build/bin/eibstdlib_tests` |
| EIBServer unit tests | `build/bin/eibserver_tests` |
| EIBServer integration tests | `build/bin/eibserver_integration_tests` |

### Clean

```bash
cmake --build build --target clean
```
