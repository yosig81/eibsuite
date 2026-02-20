#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class RecursiveMutexTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// First lock returns true, recursive returns false
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, FirstLockReturnsTrue)
{
    JTCRecursiveMutex m;
    EXPECT_TRUE(m.lock());
    m.unlock();
}

TEST_F(RecursiveMutexTest, RecursiveLockReturnsFalse)
{
    JTCRecursiveMutex m;
    EXPECT_TRUE(m.lock());   // first
    EXPECT_FALSE(m.lock());  // recursive
    m.unlock();
    m.unlock();
}

// ---------------------------------------------------------------------------
// Unlock returns false when still held, true when fully released
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, UnlockReturnsFalseWhenStillHeld)
{
    JTCRecursiveMutex m;
    m.lock();
    m.lock();
    EXPECT_FALSE(m.unlock()); // still held once
    EXPECT_TRUE(m.unlock());  // fully released
}

// ---------------------------------------------------------------------------
// count() tracks recursion depth
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, CountTracksDepth)
{
    JTCRecursiveMutex m;
    EXPECT_EQ(m.count(), 0u);
    m.lock();
    EXPECT_EQ(m.count(), 1u);
    m.lock();
    EXPECT_EQ(m.count(), 2u);
    m.lock();
    EXPECT_EQ(m.count(), 3u);
    m.unlock();
    EXPECT_EQ(m.count(), 2u);
    m.unlock();
    EXPECT_EQ(m.count(), 1u);
    m.unlock();
    EXPECT_EQ(m.count(), 0u);
}

// ---------------------------------------------------------------------------
// get_owner() works correctly (unlike JTCMutex)
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, GetOwnerCorrect)
{
    JTCRecursiveMutex m;
    // Before lock, owner should be null
    JTCThreadId null_id;
    EXPECT_EQ(m.get_owner(), null_id);

    m.lock();
    EXPECT_EQ(m.get_owner(), JTCThreadId::self());
    m.unlock();

    // After full unlock, owner should be null again
    EXPECT_EQ(m.get_owner(), null_id);
}

// ---------------------------------------------------------------------------
// Trylock
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, TrylockWhenFree)
{
    JTCRecursiveMutex m;
    EXPECT_TRUE(m.trylock());
    EXPECT_EQ(m.count(), 1u);
    m.unlock();
}

TEST_F(RecursiveMutexTest, TrylockRecursive)
{
    JTCRecursiveMutex m;
    m.lock();
    EXPECT_TRUE(m.trylock()); // recursive trylock succeeds
    EXPECT_EQ(m.count(), 2u);
    m.unlock();
    m.unlock();
}

TEST_F(RecursiveMutexTest, TrylockContended)
{
    JTCRecursiveMutex m;
    m.lock();

    bool other_result = true;
    std::thread t([&]() {
        JTCAdoptCurrentThread adopt;
        other_result = m.trylock();
    });
    t.join();
    EXPECT_FALSE(other_result);

    m.unlock();
}

// ---------------------------------------------------------------------------
// Threaded: cross-thread get_owner()
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, CrossThreadGetOwner)
{
    JTCRecursiveMutex m;
    JTCThreadId main_id = JTCThreadId::self();

    m.lock();
    EXPECT_EQ(m.get_owner(), main_id);

    JTCThreadId observed_owner;
    std::thread t([&]() {
        JTCAdoptCurrentThread adopt;
        // Another thread sees the owner as the main thread
        observed_owner = m.get_owner();
    });
    t.join();
    EXPECT_EQ(observed_owner, main_id);

    m.unlock();
}

// ---------------------------------------------------------------------------
// Mutual exclusion with recursion
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, MutualExclusionWithRecursion)
{
    JTCRecursiveMutex m;
    int counter = 0;
    const int N = 8;
    const int ITERS = 5000;

    auto worker = [&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITERS; ++i) {
            m.lock();
            m.lock(); // recursive
            ++counter;
            m.unlock();
            m.unlock();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
        threads.emplace_back(worker);
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(counter, N * ITERS);
}

// ---------------------------------------------------------------------------
// Deep recursion
// ---------------------------------------------------------------------------

TEST_F(RecursiveMutexTest, DeepRecursion)
{
    JTCRecursiveMutex m;
    const int DEPTH = 100;
    for (int i = 0; i < DEPTH; ++i)
        m.lock();
    EXPECT_EQ(m.count(), (unsigned int)DEPTH);
    for (int i = 0; i < DEPTH; ++i)
        m.unlock();
    EXPECT_EQ(m.count(), 0u);
}
