#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class JTCInitializeTest : public ::testing::Test {};

TEST_F(JTCInitializeTest, Initialized)
{
    EXPECT_TRUE(JTCInitialize::initialized());
}

TEST_F(JTCInitializeTest, MultipleInstancesRefCounted)
{
    // Create a second initializer -- should not crash
    {
        JTCInitialize second;
        EXPECT_TRUE(JTCInitialize::initialized());
    }
    // First (global) is still alive -- should still be initialized
    EXPECT_TRUE(JTCInitialize::initialized());
}

// When JTCInitialize is already active, the argv-constructor just
// ref-counts and skips argument parsing entirely. So unknown -JTC
// options do NOT throw in this case. We test the ref-count path.
TEST_F(JTCInitializeTest, ArgvConstructorRefCounts)
{
    const char* args[] = {"prog", "-JTCss", "65536"};
    int argc = 3;
    char** argv = const_cast<char**>(args);

    // Just verifies no crash -- args are not parsed when ref-counting
    JTCInitialize init(argc, argv);
    EXPECT_TRUE(JTCInitialize::initialized());
}

// The unknown-option check only fires on the FIRST JTCInitialize.
// Since our global JTCInitialize exists, subsequent constructors
// just ref-count. This test documents that behaviour.
TEST_F(JTCInitializeTest, UnknownJTCOptionSkippedWhenRefCounting)
{
    const char* args[] = {"prog", "-JTCgarbage"};
    int argc = 2;
    char** argv = const_cast<char**>(args);
    // Does NOT throw because the library is already initialized --
    // the constructor just copies state from the first instance
    EXPECT_NO_THROW({
        JTCInitialize init(argc, argv);
    });
}

// ---------------------------------------------------------------------------
// JTCAdoptCurrentThread -- wraps an external pthread for use with JTC
// ---------------------------------------------------------------------------

TEST_F(JTCInitializeTest, AdoptCurrentThread)
{
    // In the main thread (already a JTC thread), currentThread() is valid.
    JTCThread* ct = JTCThread::currentThread();
    EXPECT_NE(ct, nullptr);

    // A second thread created with std::thread is NOT a JTC thread.
    // JTCAdoptCurrentThread should make it one.
    bool adopted_ok = false;
    std::thread t([&]() {
        JTCAdoptCurrentThread adopt;
        JTCThread* inner = JTCThread::currentThread();
        adopted_ok = (inner != nullptr);
    });
    t.join();
    EXPECT_TRUE(adopted_ok);
}
