# EIBStdLib Unit Tests

This directory contains Google Test-based unit tests for the EIBStdLib library.

## Overview

- **Test Framework**: Google Test (gtest)
- **Total Tests**: 47 passing tests
- **Coverage**: CString (42 tests), StringTokenizer (5 tests)

## Directory Structure

```
test/
├── Makefile                # Test build configuration
├── unit/                   # Unit test files
│   ├── CStringTest.cpp     # Tests for CString class (42 tests)
│   └── StringTokenizerTest.cpp  # Tests for StringTokenizer class (5 tests)
├── fixtures/               # Test helpers and utilities
│   └── TestHelpers.h       # Common test fixtures and helper functions
└── README.md               # This file
```

## Building and Running Tests

### From the test directory:
```bash
cd /home/ygabay/eibsuite/EIBStdLib/test
make           # Build tests
make run       # Build and run tests
make clean     # Clean build artifacts
```

### From the main EIBStdLib directory:
```bash
cd /home/ygabay/eibsuite/EIBStdLib/linux
make test      # Build library and run tests
```

### Running specific tests:
```bash
cd /home/ygabay/eibsuite/EIBStdLib/test
./run_tests --gtest_filter=CStringTest.*        # Run only CString tests
./run_tests --gtest_filter=*Constructor*        # Run constructor tests
./run_tests --gtest_list_tests                  # List all available tests
```

## Test Coverage

### CString Tests (42 tests)
Comprehensive coverage of the CString class including:
- **Constructors**: Default, char*, int, bool, double, copy constructors
- **Operators**: ==, !=, +, +=, [], assignment operators
- **String Operations**: ToLower, ToUpper, Trim, TrimStart, SubString, Erase, Clear
- **Search Operations**: Find, FindFirstOf
- **Conversions**: ToInt, ToUInt, ToDouble, ToBool, ToChar, ToShort, ToLong
- **Utilities**: GetLength, GetBuffer, IsEmpty, HashCode
- **Hex Operations**: ToHexFormat for various types
- **Edge Cases**: Empty strings, multiple operations

### StringTokenizer Tests (5 tests)
Basic coverage of StringTokenizer class:
- **Initialization**: Empty string handling
- **State Queries**: HasMoreTokens for various inputs
- **Data Access**: RemainingString functionality

## Known Issues and Limitations

### StringTokenizer Implementation Bugs

The StringTokenizer class has several bugs that prevent comprehensive testing:

1. **SubString Length Bug** (StringTokenizer.cpp:102)
   - Incorrect length calculation in NextToken()
   - Current: `_token_str.GetLength()-pos - 1`
   - Should be: `_token_str.GetLength()-pos - _delim.GetLength()`
   - **Impact**: Causes assertion failures when NextToken() is called

2. **CountTokens Hanging Issue**
   - CountTokens() appears to hang with certain non-empty inputs
   - **Impact**: Cannot test token counting with complex strings

Due to these bugs, the StringTokenizer test suite is limited to:
- Construction and basic state queries
- Empty string handling
- RemainingString() functionality

**Additional tests to add once bugs are fixed:**
- NextToken() tokenization tests
- NextIntToken(), NextFloatToken(), NextInt64Token() tests
- FilterNextToken() tests
- Comprehensive delimiter scenario tests
- Leading/trailing/consecutive delimiter handling

## Test Development

### Adding New Tests

1. Create test file in `unit/` directory:
```cpp
#include <gtest/gtest.h>
#include "YourClass.h"
#include "../fixtures/TestHelpers.h"

class YourClassTest : public EIBStdLibTest::BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(YourClassTest, YourMethod_Scenario) {
    // Test implementation
    EXPECT_EQ(expected, actual);
}
```

2. Rebuild and run:
```bash
make clean && make run
```

### Test Naming Convention

Use descriptive test names following the pattern:
- `TEST_F(ClassName, MethodName_Scenario)`
- Example: `TEST_F(CStringTest, ToLower_ConvertsToLowercase)`

### Helper Functions

The `TestHelpers.h` provides:
- `FloatEquals(a, b, epsilon)`: Compare floating point values with tolerance
- `BaseTestFixture`: Base class for test fixtures with common setup/teardown

## Dependencies

- **Google Test**: System-installed libgtest-dev
- **EIBStdLib**: Must be built before running tests
- **JTC Library**: Threading library dependency

## Integration

The test suite is integrated into the main build system:
- Tests are built against the shared library `libEIBStdLib.so`
- The main Makefile includes a `test` target
- Clean target removes test artifacts

## Future Improvements

1. **Fix StringTokenizer bugs** to enable comprehensive testing
2. **Expand coverage** to other classes:
   - Utils
   - CException
   - ConfigFile
   - CMutex
   - CTime
   - Socket
   - DataBuffer
3. **Add code coverage reporting** with gcov/lcov
4. **Integrate with CI/CD** pipeline
5. **Add performance benchmarks** for critical paths
6. **Mock external dependencies** for isolated unit testing

## Success Metrics

✅ All 47 tests passing
✅ Fast execution (< 1ms for full suite)
✅ Easy to run from command line
✅ Integrated with main build system
✅ Clear test organization and naming
✅ Documented known issues

## Troubleshooting

### Tests fail to build
- Ensure EIBStdLib is built: `cd ../linux && make`
- Check that gtest is installed: `ldconfig -p | grep gtest`

### Linking errors
- Verify jtc library exists: `ls -la ../../jtc/linux/libjtc.so`
- Check rpath settings in test Makefile

### Tests hang
- Known issue with StringTokenizer NextToken() calls
- Use `timeout` command: `timeout 10 ./run_tests`
