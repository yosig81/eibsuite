#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class SyncTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// JTCSynchronized with JTCMutex
// ---------------------------------------------------------------------------

TEST_F(SyncTest, SynchronizedWithMutex)
{
    JTCMutex m;
    {
        JTCSynchronized sync(m);
        // Mutex is now locked. Trylock from same thread would deadlock
        // on a non-recursive mutex, so just verify scope works.
    }
    // After scope, mutex should be unlocked
    EXPECT_TRUE(m.trylock());
    m.unlock();
}

// ---------------------------------------------------------------------------
// JTCSynchronized with JTCRecursiveMutex
// ---------------------------------------------------------------------------

TEST_F(SyncTest, SynchronizedWithRecursiveMutex)
{
    JTCRecursiveMutex m;
    {
        JTCSynchronized sync(m);
        EXPECT_EQ(m.count(), 1u);
        {
            JTCSynchronized sync2(m);
            EXPECT_EQ(m.count(), 2u);
        }
        EXPECT_EQ(m.count(), 1u);
    }
    EXPECT_EQ(m.count(), 0u);
}

// ---------------------------------------------------------------------------
// JTCSynchronized with JTCMonitor
// ---------------------------------------------------------------------------

TEST_F(SyncTest, SynchronizedWithMonitor)
{
    JTCMonitor mon;
    {
        JTCSynchronized sync(mon);
        // Monitor is locked; we can call notify/wait inside here
        mon.notify(); // should not throw
    }
    // After scope, monitor is unlocked
}

// ---------------------------------------------------------------------------
// JTCSynchronized with JTCRWMutex (read lock)
// ---------------------------------------------------------------------------

TEST_F(SyncTest, SynchronizedWithRWMutexReadLock)
{
    JTCRWMutex rw;
    {
        JTCSynchronized sync(rw, JTCSynchronized::read_lock);
        // Read lock is held; another read lock should be possible
        // (but we can't easily test from same thread without nesting)
    }
    // After scope, unlocked
    rw.write_lock();
    rw.unlock();
}

// ---------------------------------------------------------------------------
// JTCSynchronized with JTCRWMutex (write lock)
// ---------------------------------------------------------------------------

TEST_F(SyncTest, SynchronizedWithRWMutexWriteLock)
{
    JTCRWMutex rw;
    {
        JTCSynchronized sync(rw, JTCSynchronized::write_lock);
        // Write lock is held
    }
    // After scope, unlocked
    rw.read_lock();
    rw.unlock();
}

// ---------------------------------------------------------------------------
// Exception safety -- mutex unlocked after throw
// ---------------------------------------------------------------------------

TEST_F(SyncTest, ExceptionSafetyMutex)
{
    JTCMutex m;
    try {
        JTCSynchronized sync(m);
        throw std::runtime_error("test exception");
    } catch (...) {
        // swallow
    }
    // Mutex should be unlocked after exception
    EXPECT_TRUE(m.trylock());
    m.unlock();
}

TEST_F(SyncTest, ExceptionSafetyRecursiveMutex)
{
    JTCRecursiveMutex m;
    try {
        JTCSynchronized sync(m);
        throw std::runtime_error("test exception");
    } catch (...) {
        // swallow
    }
    EXPECT_EQ(m.count(), 0u);
}

// ---------------------------------------------------------------------------
// JTCSyncT with mutex, recursive mutex, monitor
// ---------------------------------------------------------------------------

TEST_F(SyncTest, SyncTWithMutex)
{
    JTCMutex m;
    {
        JTCSyncT<JTCMutex> sync(m);
    }
    EXPECT_TRUE(m.trylock());
    m.unlock();
}

TEST_F(SyncTest, SyncTWithRecursiveMutex)
{
    JTCRecursiveMutex m;
    {
        JTCSyncT<JTCRecursiveMutex> sync(m);
        EXPECT_EQ(m.count(), 1u);
    }
    EXPECT_EQ(m.count(), 0u);
}

TEST_F(SyncTest, SyncTWithMonitor)
{
    JTCMonitor mon;
    {
        JTCSyncT<JTCMonitor> sync(mon);
        mon.notify(); // should not throw
    }
}

// ---------------------------------------------------------------------------
// JTCReadLock / JTCWriteLock RAII
// ---------------------------------------------------------------------------

TEST_F(SyncTest, ReadLockRAII)
{
    JTCRWMutex rw;
    {
        JTCReadLock rl(rw);
        // Read lock is held
    }
    // After scope, verify by taking write lock
    rw.write_lock();
    rw.unlock();
}

TEST_F(SyncTest, WriteLockRAII)
{
    JTCRWMutex rw;
    {
        JTCWriteLock wl(rw);
        // Write lock is held
    }
    // After scope, verify by taking read lock
    rw.read_lock();
    rw.unlock();
}
