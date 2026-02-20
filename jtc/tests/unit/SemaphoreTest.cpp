#include <JTC.h>
#include <JTCSemaphore.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class SemaphoreTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Initial count 0 blocks, count 1 allows
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, InitialCountZeroBlocks)
{
    JTCSemaphore sem(0);
    // Timed wait should timeout immediately since count is 0
    bool result = sem.wait(10); // 10ms
    EXPECT_FALSE(result);
}

TEST_F(SemaphoreTest, InitialCountOneAllows)
{
    JTCSemaphore sem(1);
    bool result = sem.wait(100);
    EXPECT_TRUE(result);
}

// ---------------------------------------------------------------------------
// Post then wait
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, PostThenWait)
{
    JTCSemaphore sem(0);
    sem.post();
    bool result = sem.wait(100);
    EXPECT_TRUE(result);
}

// ---------------------------------------------------------------------------
// Multiple posts / waits
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, MultiplePostsWaits)
{
    JTCSemaphore sem(0);
    sem.post();
    sem.post();
    sem.post();
    EXPECT_TRUE(sem.wait(100));
    EXPECT_TRUE(sem.wait(100));
    EXPECT_TRUE(sem.wait(100));
    EXPECT_FALSE(sem.wait(10)); // no more
}

// ---------------------------------------------------------------------------
// Timed wait timeout / signal
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, TimedWaitTimeout)
{
    JTCSemaphore sem(0);
    auto start = std::chrono::steady_clock::now();
    bool result = sem.wait(50); // 50ms
    auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_FALSE(result);
    // Should have waited at least ~50ms
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count(), 30);
}

TEST_F(SemaphoreTest, TimedWaitSignalled)
{
    JTCSemaphore sem(0);
    std::atomic<bool> posted{false};

    std::thread poster([&]() {
        JTCAdoptCurrentThread adopt;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        sem.post();
        posted = true;
    });

    bool result = sem.wait(5000); // long timeout
    EXPECT_TRUE(result);
    poster.join();
}

// ---------------------------------------------------------------------------
// Negative timeout means infinite wait
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, NegativeTimeoutInfiniteWait)
{
    JTCSemaphore sem(0);
    std::thread poster([&]() {
        JTCAdoptCurrentThread adopt;
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        sem.post();
    });

    // timeout=-1 should wait indefinitely
    bool result = sem.wait(-1);
    EXPECT_TRUE(result);
    poster.join();
}

// ---------------------------------------------------------------------------
// post(count) increments by count
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, PostCount)
{
    JTCSemaphore sem(0);
    sem.post(5); // Should increment count by 5
    EXPECT_TRUE(sem.wait(100));
    EXPECT_TRUE(sem.wait(100));
    EXPECT_TRUE(sem.wait(100));
    EXPECT_TRUE(sem.wait(100));
    EXPECT_TRUE(sem.wait(100));
    EXPECT_FALSE(sem.wait(10)); // no more
}

// ---------------------------------------------------------------------------
// Threaded producer-consumer
// ---------------------------------------------------------------------------

TEST_F(SemaphoreTest, ProducerConsumer)
{
    JTCSemaphore sem(0);
    std::atomic<int> consumed{0};
    const int ITEMS = 50;

    std::thread producer([&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITEMS; ++i) {
            sem.post();
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    std::thread consumer([&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITEMS; ++i) {
            sem.wait();
            ++consumed;
        }
    });

    producer.join();
    consumer.join();
    EXPECT_EQ(consumed.load(), ITEMS);
}
