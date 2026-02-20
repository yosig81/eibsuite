// IntegrationMain.cpp -- Custom main() for integration tests.
//
// Registers the IntegrationEnvironment exactly once and runs all tests.
// This replaces gtest_main to avoid multiple registrations from the
// header being included in every test .cpp file.

#include "IntegrationHelpers.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new IntegrationTest::IntegrationEnvironment);
    return RUN_ALL_TESTS();
}
