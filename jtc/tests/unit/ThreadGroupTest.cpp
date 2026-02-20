#include <JTC.h>
#include <gtest/gtest.h>
#include <thread>
#include <cstring>

// JTCInitialize is provided by TestMain.cpp for the entire test run.

class ThreadGroupTest : public ::testing::Test {};

// Helper thread that sleeps
class TGSleepThread : public JTCThread
{
public:
    TGSleepThread(JTCThreadGroupHandle& g, const char* name, int ms)
        : JTCThread(g, name), sleep_ms(ms) {}
    void run() override { JTCThread::sleep(sleep_ms); }
private:
    int sleep_ms;
};

// ---------------------------------------------------------------------------
// Named group
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, NamedGroup)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("TestGroup");
    EXPECT_STREQ(g->getName(), "TestGroup");
}

// ---------------------------------------------------------------------------
// Parent/child hierarchy
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, ParentChildHierarchy)
{
    JTCThreadGroupHandle parent = new JTCThreadGroup("Parent");
    JTCThreadGroupHandle child = new JTCThreadGroup(parent, "Child");

    EXPECT_EQ(child->getParent().get(), parent.get());
    EXPECT_TRUE(parent->parentOf(child));
    EXPECT_FALSE(child->parentOf(parent));
}

// ---------------------------------------------------------------------------
// activeCount
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, ActiveCount)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("CountGroup");
    EXPECT_EQ(g->activeCount(), 0);

    JTCThreadHandle t1 = new TGSleepThread(g, "T1", 200);
    JTCThreadHandle t2 = new TGSleepThread(g, "T2", 200);
    t1->start();
    t2->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_EQ(g->activeCount(), 2);

    t1->join();
    t2->join();
}

// ---------------------------------------------------------------------------
// activeGroupCount
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, ActiveGroupCount)
{
    JTCThreadGroupHandle parent = new JTCThreadGroup("Parent2");
    EXPECT_EQ(parent->activeGroupCount(), 0);

    JTCThreadGroupHandle child1 = new JTCThreadGroup(parent, "Child1");
    JTCThreadGroupHandle child2 = new JTCThreadGroup(parent, "Child2");

    EXPECT_EQ(parent->activeGroupCount(), 2);
}

// ---------------------------------------------------------------------------
// MaxPriority get/set
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, MaxPriorityGetSet)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("PriGroup");
    EXPECT_EQ(g->getMaxPriority(), JTCThread::JTC_MAX_PRIORITY);

    g->setMaxPriority(JTCThread::JTC_MIN_PRIORITY);
    EXPECT_EQ(g->getMaxPriority(), JTCThread::JTC_MIN_PRIORITY);
}

// ---------------------------------------------------------------------------
// Daemon flag
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, DaemonFlag)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("DaemonGroup");
    EXPECT_FALSE(g->isDaemon());
    g->setDaemon(true);
    EXPECT_TRUE(g->isDaemon());
    g->setDaemon(false);
    EXPECT_FALSE(g->isDaemon());
}

// ---------------------------------------------------------------------------
// parentOf
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, ParentOf)
{
    JTCThreadGroupHandle g1 = new JTCThreadGroup("G1");
    JTCThreadGroupHandle g2 = new JTCThreadGroup(g1, "G2");
    JTCThreadGroupHandle g3 = new JTCThreadGroup(g2, "G3");

    EXPECT_TRUE(g1->parentOf(g2));
    EXPECT_TRUE(g1->parentOf(g3));
    EXPECT_TRUE(g2->parentOf(g3));
    EXPECT_FALSE(g3->parentOf(g1));
}

// ---------------------------------------------------------------------------
// Destroy empty group
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, DestroyEmptyGroup)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("EmptyGroup");
    EXPECT_FALSE(g->isDestroyed());
    EXPECT_NO_THROW(g->destroy());
    EXPECT_TRUE(g->isDestroyed());
}

// ---------------------------------------------------------------------------
// Destroy with active threads throws
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, DestroyWithThreadsThrows)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("ActiveGroup");
    JTCThreadHandle t = new TGSleepThread(g, "Active", 300);
    t->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_THROW(g->destroy(), JTCIllegalThreadStateException);

    t->join();
}

// ---------------------------------------------------------------------------
// Enumerate threads
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, EnumerateThreads)
{
    JTCThreadGroupHandle g = new JTCThreadGroup("EnumGroup");
    JTCThreadHandle t1 = new TGSleepThread(g, "ET1", 200);
    JTCThreadHandle t2 = new TGSleepThread(g, "ET2", 200);
    t1->start();
    t2->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    JTCThreadHandle list[10];
    int count = g->enumerate(list, 10);
    EXPECT_EQ(count, 2);

    t1->join();
    t2->join();
}

// ---------------------------------------------------------------------------
// Enumerate groups
// ---------------------------------------------------------------------------

TEST_F(ThreadGroupTest, EnumerateGroups)
{
    JTCThreadGroupHandle parent = new JTCThreadGroup("EnumParent");
    JTCThreadGroupHandle c1 = new JTCThreadGroup(parent, "EC1");
    JTCThreadGroupHandle c2 = new JTCThreadGroup(parent, "EC2");
    JTCThreadGroupHandle c3 = new JTCThreadGroup(parent, "EC3");

    JTCThreadGroupHandle list[10];
    int count = parent->enumerate(list, 10, false); // non-recursive
    EXPECT_EQ(count, 3);
}
