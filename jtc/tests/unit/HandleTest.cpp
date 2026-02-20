#include <JTC.h>
#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// A simple ref-counted test object
// ---------------------------------------------------------------------------

static std::atomic<int> g_alive_count{0};

class TestRefObj : public virtual JTCRefCount
{
public:
    int value;

    explicit TestRefObj(int v = 0) : value(v) { ++g_alive_count; }
    ~TestRefObj() override { --g_alive_count; }
};

typedef JTCHandleT<TestRefObj> TestHandle;

class HandleTest : public ::testing::Test
{
protected:
    void SetUp() override { g_alive_count = 0; }
    void TearDown() override { EXPECT_EQ(g_alive_count.load(), 0); }
};

// ---------------------------------------------------------------------------
// QUIRK: m_refcount and m_ref_mutex are public
// ---------------------------------------------------------------------------

TEST_F(HandleTest, RefCountIsPublic)
{
    // This compiles only because m_refcount is public.
    TestRefObj* obj = new TestRefObj(1);
    EXPECT_EQ(obj->m_refcount, 0);
    TestHandle h(obj);
    EXPECT_EQ(obj->m_refcount, 1);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TEST_F(HandleTest, DefaultHandleIsNull)
{
    TestHandle h;
    EXPECT_FALSE(h);
    EXPECT_TRUE(!h);
    EXPECT_EQ(h.get(), nullptr);
}

TEST_F(HandleTest, ConstructFromPointerIncrementsRefcount)
{
    TestRefObj* raw = new TestRefObj(42);
    EXPECT_EQ(raw->m_refcount, 0);
    {
        TestHandle h(raw);
        EXPECT_EQ(raw->m_refcount, 1);
        EXPECT_TRUE(h);
        EXPECT_EQ(h.get(), raw);
    }
    // Destructor should have deleted the object
    EXPECT_EQ(g_alive_count.load(), 0);
}

TEST_F(HandleTest, DestructorDeletesWhenRefcountReachesZero)
{
    {
        TestHandle h(new TestRefObj(1));
        EXPECT_EQ(g_alive_count.load(), 1);
    }
    EXPECT_EQ(g_alive_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Copy semantics
// ---------------------------------------------------------------------------

TEST_F(HandleTest, CopyCtorIncrementsRefcount)
{
    TestRefObj* raw = new TestRefObj(10);
    TestHandle h1(raw);
    EXPECT_EQ(raw->m_refcount, 1);
    {
        TestHandle h2(h1);
        EXPECT_EQ(raw->m_refcount, 2);
        EXPECT_EQ(h1.get(), h2.get());
    }
    EXPECT_EQ(raw->m_refcount, 1);
    EXPECT_EQ(g_alive_count.load(), 1);
}

TEST_F(HandleTest, AssignmentIncrementsRefcount)
{
    TestRefObj* a = new TestRefObj(1);
    TestRefObj* b = new TestRefObj(2);
    TestHandle ha(a);
    TestHandle hb(b);
    EXPECT_EQ(a->m_refcount, 1);
    EXPECT_EQ(b->m_refcount, 1);

    ha = hb; // releases a, increments b's refcount
    EXPECT_EQ(g_alive_count.load(), 1); // 'a' deleted
    EXPECT_EQ(b->m_refcount, 2);
    EXPECT_EQ(ha.get(), b);
}

TEST_F(HandleTest, SelfAssignmentIsSafe)
{
    TestHandle h(new TestRefObj(99));
    TestRefObj* raw = h.get();
    h = h;
    EXPECT_EQ(h.get(), raw);
    EXPECT_EQ(raw->m_refcount, 1);
}

// ---------------------------------------------------------------------------
// Assignment from raw pointer
// ---------------------------------------------------------------------------

TEST_F(HandleTest, AssignFromRawPointer)
{
    TestHandle h(new TestRefObj(1));
    TestRefObj* raw2 = new TestRefObj(2);
    h = raw2;
    EXPECT_EQ(h.get(), raw2);
    EXPECT_EQ(raw2->m_refcount, 1);
    EXPECT_EQ(g_alive_count.load(), 1);
}

TEST_F(HandleTest, AssignNull)
{
    TestHandle h(new TestRefObj(1));
    EXPECT_TRUE(h);
    h = (TestRefObj*)0;
    EXPECT_FALSE(h);
    EXPECT_EQ(g_alive_count.load(), 0);
}

// ---------------------------------------------------------------------------
// Equality and dereference
// ---------------------------------------------------------------------------

TEST_F(HandleTest, EqualityOperators)
{
    TestRefObj* raw = new TestRefObj(1);
    TestHandle h1(raw);
    TestHandle h2(h1);
    TestHandle h3(new TestRefObj(2));

    EXPECT_TRUE(h1 == h2);
    EXPECT_FALSE(h1 != h2);
    EXPECT_TRUE(h1 != h3);
    EXPECT_FALSE(h1 == h3);
}

TEST_F(HandleTest, DereferenceOperators)
{
    TestHandle h(new TestRefObj(77));
    EXPECT_EQ(h->value, 77);
    EXPECT_EQ((*h).value, 77);
}

TEST_F(HandleTest, MultipleHandlesToSameObject)
{
    TestRefObj* raw = new TestRefObj(5);
    TestHandle h1(raw);
    TestHandle h2(raw);
    TestHandle h3(h1);
    EXPECT_EQ(raw->m_refcount, 3);
    h1 = (TestRefObj*)0;
    EXPECT_EQ(raw->m_refcount, 2);
    h2 = (TestRefObj*)0;
    EXPECT_EQ(raw->m_refcount, 1);
    EXPECT_EQ(g_alive_count.load(), 1);
}

// ---------------------------------------------------------------------------
// Concurrent copy/destroy (no double-free)
// ---------------------------------------------------------------------------

TEST_F(HandleTest, ConcurrentCopyDestroy)
{
    TestHandle shared(new TestRefObj(42));
    const int N = 100;

    auto worker = [&]() {
        for (int i = 0; i < 1000; ++i) {
            TestHandle local(shared);
            (void)local->value;
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i)
        threads.emplace_back(worker);

    for (auto& t : threads)
        t.join();

    EXPECT_EQ(shared.get()->m_refcount, 1);
    EXPECT_EQ(g_alive_count.load(), 1);
}
