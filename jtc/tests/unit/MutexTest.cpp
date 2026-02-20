#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class MutexTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Basic lock / unlock
// ---------------------------------------------------------------------------

TEST_F(MutexTest, LockUnlock)
{
    JTCMutex m;
    bool locked = m.lock();
    EXPECT_TRUE(locked);
    bool unlocked = m.unlock();
    EXPECT_TRUE(unlocked);
}

TEST_F(MutexTest, TrylockWhenFree)
{
    JTCMutex m;
    EXPECT_TRUE(m.trylock());
    m.unlock();
}

// ---------------------------------------------------------------------------
// QUIRK: get_owner() always returns self()
// Unlike JTCRecursiveMutex, JTCMutex::get_owner() returns
// JTCThreadId::self() regardless of who holds the lock.
// ---------------------------------------------------------------------------

TEST_F(MutexTest, GetOwnerAlwaysReturnsSelf)
{
    JTCMutex m;
    // Before locking, get_owner() still returns self()
    JTCThreadId owner = m.get_owner();
    JTCThreadId self = JTCThreadId::self();
    EXPECT_EQ(owner, self);

    m.lock();
    EXPECT_EQ(m.get_owner(), JTCThreadId::self());
    m.unlock();

    // Even after unlock, still returns self()
    EXPECT_EQ(m.get_owner(), JTCThreadId::self());
}

// ---------------------------------------------------------------------------
// QUIRK: count() always returns 1
// ---------------------------------------------------------------------------

TEST_F(MutexTest, CountAlwaysReturnsOne)
{
    JTCMutex m;
    EXPECT_EQ(m.count(), 1u);
    m.lock();
    EXPECT_EQ(m.count(), 1u);
    m.unlock();
    EXPECT_EQ(m.count(), 1u);
}

// ---------------------------------------------------------------------------
// QUIRK: lock()/unlock() return value always true
// ---------------------------------------------------------------------------

TEST_F(MutexTest, LockReturnAlwaysTrue)
{
    JTCMutex m;
    EXPECT_TRUE(m.lock());
    m.unlock();
}

TEST_F(MutexTest, UnlockReturnAlwaysTrue)
{
    JTCMutex m;
    m.lock();
    EXPECT_TRUE(m.unlock());
}

// ---------------------------------------------------------------------------
// Threaded: trylock fails when held by another thread
// ---------------------------------------------------------------------------

TEST_F(MutexTest, TrylockFailsWhenHeldByAnother)
{
    JTCMutex m;
    m.lock();

    bool other_trylock_result = true;
    std::thread t([&]() {
        JTCAdoptCurrentThread adopt;
        other_trylock_result = m.trylock();
    });
    t.join();

    EXPECT_FALSE(other_trylock_result);
    m.unlock();
}

// ---------------------------------------------------------------------------
// Mutual exclusion stress test
// ---------------------------------------------------------------------------

TEST_F(MutexTest, MutualExclusionStress)
{
    JTCMutex m;
    int counter = 0;
    const int N = 10;
    const int ITERS = 10000;

    auto worker = [&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITERS; ++i) {
            m.lock();
            ++counter;
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
// Trylock under contention -- not a race, just verify behaviour
// ---------------------------------------------------------------------------

TEST_F(MutexTest, TrylockUnderContention)
{
    JTCMutex m;
    std::atomic<bool> holder_ready{false};
    std::atomic<bool> release{false};

    std::thread holder([&]() {
        JTCAdoptCurrentThread adopt;
        m.lock();
        holder_ready = true;
        while (!release.load()) { std::this_thread::yield(); }
        m.unlock();
    });

    while (!holder_ready.load()) { std::this_thread::yield(); }

    // Mutex is held by another thread -- trylock should fail
    EXPECT_FALSE(m.trylock());

    release = true;
    holder.join();

    // Now it should succeed
    EXPECT_TRUE(m.trylock());
    m.unlock();
}
