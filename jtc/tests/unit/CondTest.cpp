#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class CondTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Signal wakes one waiter (with JTCMutex)
// ---------------------------------------------------------------------------

TEST_F(CondTest, SignalWakesOneWaiter)
{
    JTCMutex m;
    JTCCond cv;
    std::atomic<bool> ready{false};
    std::atomic<bool> done{false};

    std::thread waiter([&]() {
        JTCAdoptCurrentThread adopt;
        m.lock();
        ready = true;
        cv.wait(m);
        done = true;
        m.unlock();
    });

    // Wait for waiter to be ready
    while (!ready.load()) { std::this_thread::yield(); }
    // Give waiter time to enter wait()
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    m.lock();
    cv.signal();
    m.unlock();

    waiter.join();
    EXPECT_TRUE(done.load());
}

// ---------------------------------------------------------------------------
// Broadcast wakes all waiters
// ---------------------------------------------------------------------------

TEST_F(CondTest, BroadcastWakesAll)
{
    JTCMutex m;
    JTCCond cv;
    std::atomic<int> waiting{0};
    std::atomic<int> woken{0};
    const int N = 4;

    auto waiter_fn = [&]() {
        JTCAdoptCurrentThread adopt;
        m.lock();
        ++waiting;
        cv.wait(m);
        ++woken;
        m.unlock();
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
        threads.emplace_back(waiter_fn);

    while (waiting.load() < N) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    m.lock();
    cv.broadcast();
    m.unlock();

    for (auto& t : threads) t.join();
    EXPECT_EQ(woken.load(), N);
}

// ---------------------------------------------------------------------------
// Timed wait returns false on timeout
// ---------------------------------------------------------------------------

TEST_F(CondTest, TimedWaitTimeout)
{
    JTCMutex m;
    JTCCond cv;
    m.lock();
    bool result = cv.wait(m, 10); // 10ms timeout, no signal
    EXPECT_FALSE(result);
    m.unlock();
}

// ---------------------------------------------------------------------------
// Timed wait returns true on signal
// ---------------------------------------------------------------------------

TEST_F(CondTest, TimedWaitSignalled)
{
    JTCMutex m;
    JTCCond cv;
    std::atomic<bool> ready{false};
    bool wait_result = false;

    std::thread waiter([&]() {
        JTCAdoptCurrentThread adopt;
        m.lock();
        ready = true;
        wait_result = cv.wait(m, 5000); // long timeout
        m.unlock();
    });

    while (!ready.load()) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    m.lock();
    cv.signal();
    m.unlock();

    waiter.join();
    EXPECT_TRUE(wait_result);
}

// ---------------------------------------------------------------------------
// Negative timeout throws JTCIllegalArgumentException
// ---------------------------------------------------------------------------

TEST_F(CondTest, NegativeTimeoutThrows)
{
    JTCMutex m;
    JTCCond cv;
    m.lock();
    EXPECT_THROW(cv.wait(m, -1), JTCIllegalArgumentException);
    m.unlock();
}

// ---------------------------------------------------------------------------
// With recursive mutex: signal restores lock count
// ---------------------------------------------------------------------------

TEST_F(CondTest, RecursiveMutexSignalRestoresLockCount)
{
    JTCRecursiveMutex rm;
    JTCCond cv;
    std::atomic<bool> ready{false};

    std::thread waiter([&]() {
        JTCAdoptCurrentThread adopt;
        rm.lock();
        rm.lock(); // count = 2
        ready = true;
        cv.wait(rm);
        // After waking, lock count should be restored to 2
        EXPECT_EQ(rm.count(), 2u);
        rm.unlock();
        rm.unlock();
    });

    while (!ready.load()) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    rm.lock();
    cv.signal();
    rm.unlock();

    waiter.join();
}

// ---------------------------------------------------------------------------
// With recursive mutex: timed wait timeout
// ---------------------------------------------------------------------------

TEST_F(CondTest, RecursiveMutexTimedWaitTimeout)
{
    JTCRecursiveMutex rm;
    JTCCond cv;
    rm.lock();
    rm.lock(); // count = 2
    bool result = cv.wait(rm, 10);
    EXPECT_FALSE(result);
    // Lock count should be restored after timed wait
    EXPECT_EQ(rm.count(), 2u);
    rm.unlock();
    rm.unlock();
}
