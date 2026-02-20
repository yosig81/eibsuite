#ifndef EIBSTDLIB_TEST_HELPERS_H
#define EIBSTDLIB_TEST_HELPERS_H

#include <gtest/gtest.h>
#include <string>

// GTEST_SKIP was added in Google Test 1.10. Provide a fallback for older versions.
#ifndef GTEST_SKIP
#define GTEST_SKIP() GTEST_LOG_(INFO) << "Skipped: "
#endif

/**
 * Common test utilities and fixtures for EIBStdLib tests
 */

namespace EIBStdLibTest {

/**
 * Helper function to compare floating point values with tolerance
 */
inline bool FloatEquals(double a, double b, double epsilon = 0.0001) {
    return std::abs(a - b) < epsilon;
}

/**
 * Base fixture class for common test setup/teardown
 * Derived test fixtures can extend this for shared functionality
 */
class BaseTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Common setup for all tests
    }

    void TearDown() override {
        // Common cleanup for all tests
    }
};

} // namespace EIBStdLibTest

#endif // EIBSTDLIB_TEST_HELPERS_H
