#ifndef EIBSERVER_TEST_HELPERS_H
#define EIBSERVER_TEST_HELPERS_H

#include <gtest/gtest.h>
#include "JTC.h"

namespace EIBServerTest {

// Google Test global environment that initializes JTC before any tests run.
// This is critical because JTCThread/JTCMonitor objects (CWebHandler,
// CCommandScheduler) crash if JTC is not yet initialized when they are
// constructed -- including when they appear as test-fixture member variables.
class JTCEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // JTCInitialize must live for the entire process lifetime
        static JTCInitialize jtc_init;
    }
};

// Register the environment.  The static variable ensures registration
// happens exactly once, before main() enters RUN_ALL_TESTS().
static ::testing::Environment* const jtc_env_ [[maybe_unused]] =
    ::testing::AddGlobalTestEnvironment(new JTCEnvironment);

} // namespace EIBServerTest

#endif // EIBSERVER_TEST_HELPERS_H
