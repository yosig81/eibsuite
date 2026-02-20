#include <JTC.h>
#include <gtest/gtest.h>

// Provide a single JTCInitialize for the entire test run.
// This avoids issues with repeated init/teardown cycles corrupting
// internal state (TSS keys, thread groups, etc.).

int main(int argc, char** argv)
{
    JTCInitialize jtc;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
