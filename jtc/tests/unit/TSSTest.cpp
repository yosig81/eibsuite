#include <JTC.h>
#include <TSS.h>
#include <gtest/gtest.h>
#include <thread>
#include <atomic>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class TSSTest : public ::testing::Test {};

// ---------------------------------------------------------------------------
// Allocate / release
// ---------------------------------------------------------------------------

TEST_F(TSSTest, AllocateRelease)
{
    JTCThreadKey key = JTCTSS::allocate();
    JTCTSS::release(key);
}

// ---------------------------------------------------------------------------
// Set / get
// ---------------------------------------------------------------------------

TEST_F(TSSTest, SetGet)
{
    JTCThreadKey key = JTCTSS::allocate();
    int value = 42;
    JTCTSS::set(key, &value);
    void* got = JTCTSS::get(key);
    EXPECT_EQ(got, &value);
    EXPECT_EQ(*static_cast<int*>(got), 42);
    JTCTSS::release(key);
}

// ---------------------------------------------------------------------------
// Per-thread isolation
// ---------------------------------------------------------------------------

TEST_F(TSSTest, PerThreadIsolation)
{
    JTCThreadKey key = JTCTSS::allocate();
    int main_val = 100;
    JTCTSS::set(key, &main_val);

    int thread_val = 200;
    void* thread_got = nullptr;
    void* main_after = nullptr;

    std::thread t([&]() {
        JTCAdoptCurrentThread adopt;
        JTCTSS::set(key, &thread_val);
        thread_got = JTCTSS::get(key);
    });
    t.join();

    main_after = JTCTSS::get(key);

    // Each thread sees its own value
    EXPECT_EQ(thread_got, &thread_val);
    EXPECT_EQ(main_after, &main_val);

    JTCTSS::release(key);
}

// ---------------------------------------------------------------------------
// Cleanup function called on thread exit
// ---------------------------------------------------------------------------

static std::atomic<int> g_cleanup_count{0};
static void tss_cleanup(void* data)
{
    (void)data;
    ++g_cleanup_count;
}

TEST_F(TSSTest, CleanupCalledOnThreadExit)
{
    g_cleanup_count = 0;
    JTCThreadKey key = JTCTSS::allocate(tss_cleanup);

    // Run a JTC thread that sets TSS data -- cleanup should fire on exit
    class TSSThread : public JTCThread
    {
    public:
        JTCThreadKey key;
        int data;
        TSSThread(JTCThreadKey k) : JTCThread("TSSThread"), key(k), data(99) {}
        void run() override {
            JTCTSS::set(key, &data);
        }
    };

    JTCThreadHandle t = new TSSThread(key);
    t->start();
    t->join();

    // Cleanup should have been called
    EXPECT_EQ(g_cleanup_count.load(), 1);

    JTCTSS::release(key);
}

// ---------------------------------------------------------------------------
// Get before set returns null
// ---------------------------------------------------------------------------

TEST_F(TSSTest, GetBeforeSetReturnsNull)
{
    JTCThreadKey key = JTCTSS::allocate();
    void* val = JTCTSS::get(key);
    EXPECT_EQ(val, nullptr);
    JTCTSS::release(key);
}
