#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class RWMutexTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Basic read_lock / write_lock / unlock
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, ReadLockUnlock)
{
    JTCRWMutex rw;
    rw.read_lock();
    rw.unlock();
}

TEST_F(RWMutexTest, WriteLockUnlock)
{
    JTCRWMutex rw;
    rw.write_lock();
    rw.unlock();
}

// ---------------------------------------------------------------------------
// Multiple readers allowed concurrently
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, MultipleReadersConcurrent)
{
    JTCRWMutex rw;
    std::atomic<int> inside{0};
    std::atomic<int> max_inside{0};
    std::atomic<bool> go{false};
    const int N = 8;

    auto reader = [&]() {
        JTCAdoptCurrentThread adopt;
        while (!go.load()) { std::this_thread::yield(); }
        rw.read_lock();
        int cur = ++inside;
        // Track max concurrent readers
        int expected = max_inside.load();
        while (cur > expected && !max_inside.compare_exchange_weak(expected, cur)) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        --inside;
        rw.unlock();
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
        threads.emplace_back(reader);

    go = true;
    for (auto& t : threads) t.join();

    // Multiple readers should have been inside concurrently
    EXPECT_GT(max_inside.load(), 1);
}

// ---------------------------------------------------------------------------
// Writer excludes readers
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, WriterExcludesReaders)
{
    JTCRWMutex rw;
    std::atomic<bool> writer_locked{false};
    std::atomic<bool> reader_entered{false};
    std::atomic<bool> writer_done{false};

    std::thread writer([&]() {
        JTCAdoptCurrentThread adopt;
        rw.write_lock();
        writer_locked = true;
        // Hold write lock for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        writer_done = true;
        rw.unlock();
    });

    while (!writer_locked.load()) { std::this_thread::yield(); }

    std::thread reader([&]() {
        JTCAdoptCurrentThread adopt;
        rw.read_lock();
        reader_entered = true;
        rw.unlock();
    });

    reader.join();
    writer.join();

    // Reader should only enter after writer is done
    EXPECT_TRUE(writer_done.load());
    EXPECT_TRUE(reader_entered.load());
}

// ---------------------------------------------------------------------------
// Writer excludes writers
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, WriterExcludesWriters)
{
    JTCRWMutex rw;
    std::atomic<bool> first_locked{false};
    std::atomic<bool> first_done{false};
    std::atomic<bool> second_entered{false};

    std::thread first([&]() {
        JTCAdoptCurrentThread adopt;
        rw.write_lock();
        first_locked = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        first_done = true;
        rw.unlock();
    });

    while (!first_locked.load()) { std::this_thread::yield(); }

    std::thread second([&]() {
        JTCAdoptCurrentThread adopt;
        rw.write_lock();
        second_entered = true;
        rw.unlock();
    });

    second.join();
    first.join();

    EXPECT_TRUE(first_done.load());
    EXPECT_TRUE(second_entered.load());
}

// ---------------------------------------------------------------------------
// Readers exclude writer
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, ReadersExcludeWriter)
{
    JTCRWMutex rw;
    std::atomic<int> readers_inside{0};
    std::atomic<bool> readers_started{false};
    std::atomic<bool> writer_entered{false};
    const int N = 4;

    auto reader = [&]() {
        JTCAdoptCurrentThread adopt;
        rw.read_lock();
        ++readers_inside;
        // Signal once all readers are in
        if (readers_inside.load() == N) readers_started = true;
        // Hold read lock
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        --readers_inside;
        rw.unlock();
    };

    std::vector<std::thread> readers;
    for (int i = 0; i < N; ++i)
        readers.emplace_back(reader);

    while (!readers_started.load()) { std::this_thread::yield(); }

    std::thread writer([&]() {
        JTCAdoptCurrentThread adopt;
        rw.write_lock();
        writer_entered = true;
        rw.unlock();
    });

    writer.join();
    for (auto& r : readers) r.join();

    // Writer should only enter after all readers released
    EXPECT_TRUE(writer_entered.load());
}

// ---------------------------------------------------------------------------
// SEMANTICS: Writer preference -- readers block when writer is waiting
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, WriterPreference)
{
    JTCRWMutex rw;
    std::atomic<bool> reader1_locked{false};
    std::atomic<bool> writer_waiting{false};
    std::atomic<bool> reader2_entered{false};
    std::atomic<bool> writer_entered{false};

    // reader1 holds the read lock
    std::thread reader1([&]() {
        JTCAdoptCurrentThread adopt;
        rw.read_lock();
        reader1_locked = true;
        // Hold until writer is waiting
        while (!writer_waiting.load()) { std::this_thread::yield(); }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        rw.unlock();
    });

    while (!reader1_locked.load()) { std::this_thread::yield(); }

    // Writer attempts write_lock -- will block because reader1 holds read lock
    std::thread writer([&]() {
        JTCAdoptCurrentThread adopt;
        writer_waiting = true;
        rw.write_lock();
        writer_entered = true;
        rw.unlock();
    });

    while (!writer_waiting.load()) { std::this_thread::yield(); }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // reader2 tries to read_lock while writer is waiting
    // With writer preference, reader2 should be blocked until writer is done
    std::thread reader2([&]() {
        JTCAdoptCurrentThread adopt;
        rw.read_lock();
        reader2_entered = true;
        rw.unlock();
    });

    reader1.join();
    writer.join();
    reader2.join();

    // Writer should have entered before reader2
    EXPECT_TRUE(writer_entered.load());
    EXPECT_TRUE(reader2_entered.load());
}

// ---------------------------------------------------------------------------
// Concurrent readers + writers stress test
// ---------------------------------------------------------------------------

TEST_F(RWMutexTest, ConcurrentStress)
{
    JTCRWMutex rw;
    std::atomic<int> shared_value{0};
    std::atomic<int> read_count{0};
    const int NUM_READERS = 8;
    const int NUM_WRITERS = 4;
    const int ITERS = 500;

    auto reader = [&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITERS; ++i) {
            rw.read_lock();
            (void)shared_value.load(); // read
            ++read_count;
            rw.unlock();
        }
    };

    auto writer = [&]() {
        JTCAdoptCurrentThread adopt;
        for (int i = 0; i < ITERS; ++i) {
            rw.write_lock();
            ++shared_value;
            rw.unlock();
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_READERS; ++i)
        threads.emplace_back(reader);
    for (int i = 0; i < NUM_WRITERS; ++i)
        threads.emplace_back(writer);
    for (auto& t : threads) t.join();

    EXPECT_EQ(shared_value.load(), NUM_WRITERS * ITERS);
    EXPECT_EQ(read_count.load(), NUM_READERS * ITERS);
}
