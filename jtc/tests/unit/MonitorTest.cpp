#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <queue>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class MonitorTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Lock/unlock via JTCSynchronized
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, LockUnlockViaSynchronized)
{
    JTCMonitor mon;
    {
        JTCSynchronized sync(mon);
        // Monitor is locked
    }
    // Monitor is unlocked
    {
        JTCSynchronized sync(mon);
        // Can re-acquire
    }
}

// ---------------------------------------------------------------------------
// notify/notifyAll/wait without lock throws JTCIllegalMonitorStateException
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, NotifyWithoutLockThrows)
{
    JTCMonitor mon;
    EXPECT_THROW(mon.notify(), JTCIllegalMonitorStateException);
}

TEST_F(MonitorTest, NotifyAllWithoutLockThrows)
{
    JTCMonitor mon;
    EXPECT_THROW(mon.notifyAll(), JTCIllegalMonitorStateException);
}

TEST_F(MonitorTest, WaitWithoutLockThrows)
{
    JTCMonitor mon;
    EXPECT_THROW(mon.wait(), JTCIllegalMonitorStateException);
}

// ---------------------------------------------------------------------------
// SEMANTICS: notify() is delayed until unlock (Mesa monitor)
// The notification is not sent when notify() is called -- it's sent
// when the monitor's mutex is fully released (unlock or wait).
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, NotifyDelayedUntilUnlock)
{
    JTCMonitor mon;
    std::atomic<bool> waiter_ready{false};
    std::atomic<bool> waiter_woke{false};

    std::thread waiter([&]() {
        JTCAdoptCurrentThread adopt;
        JTCSynchronized sync(mon);
        waiter_ready = true;
        mon.wait();
        waiter_woke = true;
    });

    while (!waiter_ready.load()) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    {
        JTCSynchronized sync(mon);
        mon.notify();
        // At this point notify() has been called but the waiter should
        // NOT have woken yet because we still hold the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_FALSE(waiter_woke.load());
    }
    // Now the monitor is unlocked and the notification is sent

    waiter.join();
    EXPECT_TRUE(waiter_woke.load());
}

// ---------------------------------------------------------------------------
// notifyAll broadcasts to all waiters
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, NotifyAllBroadcasts)
{
    JTCMonitor mon;
    std::atomic<int> waiting{0};
    std::atomic<int> woken{0};
    const int N = 4;

    auto waiter_fn = [&]() {
        JTCAdoptCurrentThread adopt;
        JTCSynchronized sync(mon);
        ++waiting;
        mon.wait();
        ++woken;
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
        threads.emplace_back(waiter_fn);

    while (waiting.load() < N) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    {
        JTCSynchronized sync(mon);
        mon.notifyAll();
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(woken.load(), N);
}

// ---------------------------------------------------------------------------
// nnotify reset on lock acquisition
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, NnotifyResetOnLockAcquisition)
{
    // If we lock, call notify, then unlock (sending notification to nobody),
    // re-lock should reset the nnotify count so no spurious notification
    // is sent on the second unlock.
    JTCMonitor mon;
    {
        JTCSynchronized sync(mon);
        mon.notify(); // nnotify = 1
    }
    // Notification sent (to nobody). Re-lock should reset nnotify.
    {
        JTCSynchronized sync(mon);
        // nnotify should have been reset to 0
        // No notification on this unlock
    }
    // No crash, no spurious signal
}

// ---------------------------------------------------------------------------
// Recursive lock: notification only on final unlock
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, RecursiveLockNotificationOnFinalUnlock)
{
    JTCMonitor mon;
    std::atomic<bool> waiter_ready{false};
    std::atomic<bool> waiter_woke{false};

    std::thread waiter([&]() {
        JTCAdoptCurrentThread adopt;
        JTCSynchronized sync(mon);
        waiter_ready = true;
        mon.wait();
        waiter_woke = true;
    });

    while (!waiter_ready.load()) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    {
        JTCSynchronized outer(mon);
        {
            JTCSynchronized inner(mon); // recursive lock
            mon.notify();
        }
        // inner unlock -- but lock is still held by outer
        // Waiter should NOT be woken yet
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_FALSE(waiter_woke.load());
    }
    // outer unlock -- final release, notification fires

    waiter.join();
    EXPECT_TRUE(waiter_woke.load());
}

// ---------------------------------------------------------------------------
// Timed wait
// ---------------------------------------------------------------------------

TEST_F(MonitorTest, TimedWaitTimeout)
{
    JTCMonitor mon;
    JTCSynchronized sync(mon);
    auto start = std::chrono::steady_clock::now();
    mon.wait(50); // 50ms
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(elapsed, 30);
}

TEST_F(MonitorTest, TimedWaitSignalled)
{
    JTCMonitor mon;
    std::atomic<bool> waiter_ready{false};
    std::atomic<bool> waiter_done{false};

    std::thread waiter([&]() {
        JTCAdoptCurrentThread adopt;
        JTCSynchronized sync(mon);
        waiter_ready = true;
        mon.wait(5000); // long timeout
        waiter_done = true;
    });

    while (!waiter_ready.load()) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    {
        JTCSynchronized sync(mon);
        mon.notify();
    }

    waiter.join();
    EXPECT_TRUE(waiter_done.load());
}

// ---------------------------------------------------------------------------
// Producer-consumer pattern
// ---------------------------------------------------------------------------

class MonitorQueue : public JTCMonitor
{
public:
    std::queue<int> q;

    void produce(int val) {
        JTCSynchronized sync(*this);
        q.push(val);
        notify();
    }

    int consume() {
        JTCSynchronized sync(*this);
        while (q.empty())
            wait();
        int val = q.front();
        q.pop();
        return val;
    }
};

TEST_F(MonitorTest, ProducerConsumer)
{
    MonitorQueue mq;
    const int ITEMS = 100;
    std::vector<int> consumed;
    consumed.reserve(ITEMS);

    std::thread producer([&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITEMS; ++i) {
            mq.produce(i);
            std::this_thread::sleep_for(std::chrono::microseconds(50));
        }
    });

    std::thread consumer([&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITEMS; ++i) {
            consumed.push_back(mq.consume());
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ((int)consumed.size(), ITEMS);
    for (int i = 0; i < ITEMS; ++i)
        EXPECT_EQ(consumed[i], i);
}
