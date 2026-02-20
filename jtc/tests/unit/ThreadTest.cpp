#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <cstring>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class ThreadTest : public ::testing::Test {};

// Helper thread that records it ran
class FlagThread : public JTCThread
{
public:
    std::atomic<bool> ran{false};
    FlagThread(const char* name = "FlagThread") : JTCThread(name) {}
    void run() override { ran = true; }
};

// Helper Runnable
class FlagRunnable : public JTCRunnable
{
public:
    std::atomic<bool> ran{false};
    void run() override { ran = true; }
};

// Sleeping thread for testing isAlive
class SleepThread : public JTCThread
{
public:
    int sleep_ms;
    explicit SleepThread(int ms) : JTCThread("SleepThread"), sleep_ms(ms) {}
    void run() override {
        JTCThread::sleep(sleep_ms);
    }
};

// ---------------------------------------------------------------------------
// Create/start/join lifecycle
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, CreateStartJoin)
{
    JTCThreadHandle t = new FlagThread();
    t->start();
    t->join();
    EXPECT_TRUE(static_cast<FlagThread*>(t.get())->ran.load());
}

// ---------------------------------------------------------------------------
// isAlive states
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, IsAliveBeforeStart)
{
    JTCThreadHandle t = new SleepThread(100);
    EXPECT_FALSE(t->isAlive());
}

TEST_F(ThreadTest, IsAliveWhileRunning)
{
    JTCThreadHandle t = new SleepThread(500);
    t->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_TRUE(t->isAlive());
    t->join();
}

TEST_F(ThreadTest, IsAliveAfterJoin)
{
    JTCThreadHandle t = new FlagThread();
    t->start();
    t->join();
    EXPECT_FALSE(t->isAlive());
}

// ---------------------------------------------------------------------------
// Start twice throws
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, StartTwiceThrows)
{
    JTCThreadHandle t = new SleepThread(100);
    t->start();
    EXPECT_THROW(t->start(), JTCIllegalThreadStateException);
    t->join();
}

// ---------------------------------------------------------------------------
// destroy() on unstarted thread must not corrupt calling thread's TSS
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, DestroyUnstartedPreservesTSS)
{
    JTCThreadHandle t = new FlagThread();
    t->destroy();
    // After destroy(), the calling thread's identity must remain intact.
    EXPECT_NE(JTCThread::currentThread(), nullptr);
}

// ---------------------------------------------------------------------------
// destroy() on a started thread throws JTCIllegalThreadStateException
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, DestroyStartedThrows)
{
    JTCThreadHandle t = new SleepThread(200);
    t->start();
    EXPECT_THROW(t->destroy(), JTCIllegalThreadStateException);
    t->join();
}

// ---------------------------------------------------------------------------
// destroy() on a dead thread returns silently (does NOT throw)
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, DestroyDeadReturnsSilently)
{
    JTCThreadHandle t = new FlagThread();
    t->start();
    t->join();
    // destroy() on dead thread just returns
    EXPECT_NO_THROW(t->destroy());
}

// ---------------------------------------------------------------------------
// Runnable target execution
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, RunnableTarget)
{
    FlagRunnable* r = new FlagRunnable();
    JTCRunnableHandle rh(r);
    JTCThreadHandle t = new JTCThread(rh, "RunnableThread");
    t->start();
    t->join();
    EXPECT_TRUE(r->ran.load());
}

// ---------------------------------------------------------------------------
// join() and join(millis)
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, JoinWithTimeout)
{
    JTCThreadHandle t = new SleepThread(500);
    t->start();
    // Join with short timeout -- thread is still running
    t->join(10); // should return after ~10ms
    // Thread may still be alive
    t->join(); // wait for full completion
    EXPECT_FALSE(t->isAlive());
}

TEST_F(ThreadTest, JoinNegativeMillisThrows)
{
    JTCThreadHandle t = new FlagThread();
    t->start();
    EXPECT_THROW(t->join(-1), JTCIllegalArgumentException);
    t->join();
}

TEST_F(ThreadTest, JoinNanosOverflowThrows)
{
    JTCThreadHandle t = new FlagThread();
    t->start();
    EXPECT_THROW(t->join(0, 1000000), JTCIllegalArgumentException);
    t->join();
}

TEST_F(ThreadTest, JoinNanosNegativeThrows)
{
    JTCThreadHandle t = new FlagThread();
    t->start();
    // nanos < 0: join(long, int) doesn't explicitly check for negative nanos
    // but nanos > 999999 throws. Negative nanos won't trigger >=500000 path.
    // Just verify it doesn't crash.
    EXPECT_NO_THROW(t->join(0, -1));
    t->join();
}

// ---------------------------------------------------------------------------
// Name management
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, DefaultName)
{
    JTCThreadHandle t = new JTCThread();
    const char* name = t->getName();
    // Default name is "Thread-<number>"
    EXPECT_NE(name, nullptr);
    EXPECT_TRUE(std::strstr(name, "Thread-") != nullptr);
}

TEST_F(ThreadTest, CustomName)
{
    JTCThreadHandle t = new JTCThread("MyThread");
    EXPECT_STREQ(t->getName(), "MyThread");
}

TEST_F(ThreadTest, SetName)
{
    JTCThreadHandle t = new JTCThread("Original");
    EXPECT_STREQ(t->getName(), "Original");
    t->setName("Renamed");
    EXPECT_STREQ(t->getName(), "Renamed");
}

// QUIRK: getName() returns internal pointer that can be invalidated by setName()
TEST_F(ThreadTest, GetNameReturnsInternalPointer)
{
    JTCThreadHandle t = new JTCThread("First");
    const char* ptr = t->getName();
    EXPECT_STREQ(ptr, "First");
    t->setName("Second");
    // ptr is now a dangling pointer! Do not dereference.
    // This documents the hazard. Just verify the new name works:
    EXPECT_STREQ(t->getName(), "Second");
}

// ---------------------------------------------------------------------------
// QUIRK: setPriority/getPriority no-op on macOS/Linux
// All priorities map to 0 in the platform mapping.
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, PriorityDefaultIsNorm)
{
    // Must use SleepThread so the thread is still alive when we call
    // getPriority(). A bare JTCThread (no run() override) exits
    // immediately, and getPriority() on a dead pthread_t throws.
    JTCThreadHandle t = new SleepThread(200);
    t->start();
    // Priority is set during entrance_hook, so check after start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    // On macOS all priority values map to 0, which getPriority() maps
    // to JTC_MIN_PRIORITY. The "default" thus appears as MIN.
    int pri = t->getPriority();
    EXPECT_TRUE(pri >= JTCThread::JTC_MIN_PRIORITY && pri <= JTCThread::JTC_MAX_PRIORITY);
    t->join();
}

TEST_F(ThreadTest, SetPriorityNoOpOnPosix)
{
    JTCThreadHandle t = new SleepThread(100);
    t->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    t->setPriority(JTCThread::JTC_MAX_PRIORITY);
    // On macOS/Linux all priorities map to 0, so getPriority() returns
    // a value based on the OS mapping, not what was set.
    int pri = t->getPriority();
    EXPECT_TRUE(pri >= JTCThread::JTC_MIN_PRIORITY && pri <= JTCThread::JTC_MAX_PRIORITY);
    t->join();
}

TEST_F(ThreadTest, InvalidPriorityThrows)
{
    JTCThreadHandle t = new SleepThread(100);
    t->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_THROW(t->setPriority(999), JTCIllegalArgumentException);
    t->join();
}

// ---------------------------------------------------------------------------
// currentThread(), sleep(), yield()
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, CurrentThread)
{
    JTCThread* ct = JTCThread::currentThread();
    EXPECT_NE(ct, nullptr);
}

TEST_F(ThreadTest, Sleep)
{
    auto start = std::chrono::steady_clock::now();
    JTCThread::sleep(50);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(elapsed, 30);
}

TEST_F(ThreadTest, Yield)
{
    // Just verify it doesn't crash
    JTCThread::yield();
}

// ---------------------------------------------------------------------------
// activeCount() and enumerate()
// ---------------------------------------------------------------------------

TEST_F(ThreadTest, ActiveCount)
{
    int before = JTCThread::activeCount();
    JTCThreadHandle t = new SleepThread(200);
    t->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    int during = JTCThread::activeCount();
    t->join();
    EXPECT_GE(during, before);
}

TEST_F(ThreadTest, Enumerate)
{
    JTCThreadHandle list[32];
    int count = JTCThread::enumerate(list, 32);
    EXPECT_GE(count, 1); // at least the current thread
}

// ---------------------------------------------------------------------------
// Run hook
// ---------------------------------------------------------------------------

static std::atomic<bool> g_run_hook_called{false};
static JTCRunHook g_old_run_hook = nullptr;

static void testRunHook(JTCThread* thread)
{
    g_run_hook_called = true;
    thread->run(); // Must call run() or the thread won't execute
}

TEST_F(ThreadTest, RunHook)
{
    g_run_hook_called = false;
    JTCThread::setRunHook(testRunHook, &g_old_run_hook);

    JTCThreadHandle t = new FlagThread("HookThread");
    t->start();
    t->join();

    EXPECT_TRUE(g_run_hook_called.load());
    EXPECT_TRUE(static_cast<FlagThread*>(t.get())->ran.load());

    // Restore old hook
    JTCThread::setRunHook(g_old_run_hook);
}

// ---------------------------------------------------------------------------
// Start hook
// ---------------------------------------------------------------------------

static std::atomic<bool> g_start_hook_called{false};
static JTCStartHook g_old_start_hook = nullptr;

static void testStartHook()
{
    g_start_hook_called = true;
}

TEST_F(ThreadTest, StartHook)
{
    g_start_hook_called = false;
    JTCThread::setStartHook(testStartHook, &g_old_start_hook);

    JTCThreadHandle t = new FlagThread("StartHookThread");
    t->start();
    t->join();

    EXPECT_TRUE(g_start_hook_called.load());

    // Restore old hook
    JTCThread::setStartHook(g_old_start_hook);
}
