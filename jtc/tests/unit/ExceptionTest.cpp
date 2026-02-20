#include <JTC.h>
#include <Exception.h>
#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// JTCException basics
// ---------------------------------------------------------------------------

TEST(JTCException_Basic, DefaultConstructor)
{
    JTCException e;
    EXPECT_STREQ(e.getMessage(), "");
    EXPECT_EQ(e.getError(), 0);
    EXPECT_STREQ(e.getType(), "JTCException");
}

TEST(JTCException_Basic, MessageConstructor)
{
    JTCException e("something failed");
    EXPECT_STREQ(e.getMessage(), "something failed");
    EXPECT_EQ(e.getError(), 0);
}

TEST(JTCException_Basic, MessageAndErrorConstructor)
{
    JTCException e("syscall failed", 42);
    EXPECT_STREQ(e.getMessage(), "syscall failed");
    EXPECT_EQ(e.getError(), 42);
}

TEST(JTCException_Basic, CopyConstructor)
{
    JTCException orig("copy me", 7);
    JTCException copy(orig);
    EXPECT_STREQ(copy.getMessage(), "copy me");
    EXPECT_EQ(copy.getError(), 7);

    // Verify deep copy -- modifying orig's note shouldn't affect copy
    // (they each own their own buffer)
}

TEST(JTCException_Basic, SelfAssignment)
{
    JTCException e("safe");
    e = e; // must not crash or leak
    EXPECT_STREQ(e.getMessage(), "safe");
}

TEST(JTCException_Basic, AssignmentDoesNotLeak)
{
    JTCException a("first");
    JTCException b("second");
    a = b;
    EXPECT_STREQ(a.getMessage(), "second");
    // Under ASan/valgrind this would report a leak if operator= failed
    // to delete[] the old note_ buffer before re-allocating.
}

TEST(JTCException_Basic, AssignmentCopiesValues)
{
    JTCException a("first", 1);
    JTCException b("second", 2);
    a = b;
    EXPECT_STREQ(a.getMessage(), "second");
    EXPECT_EQ(a.getError(), 2);
}

// ---------------------------------------------------------------------------
// Subclass getType() values
// ---------------------------------------------------------------------------

TEST(JTCException_Subclasses, InterruptedException)
{
    JTCInterruptedException e("interrupted");
    EXPECT_STREQ(e.getType(), "JTCInterruptedException");
    EXPECT_STREQ(e.getMessage(), "interrupted");
}

TEST(JTCException_Subclasses, IllegalThreadStateException)
{
    JTCIllegalThreadStateException e("bad state");
    EXPECT_STREQ(e.getType(), "JTCIllegalThreadStateException");
}

TEST(JTCException_Subclasses, IllegalMonitorStateException)
{
    JTCIllegalMonitorStateException e("not locked");
    EXPECT_STREQ(e.getType(), "JTCIllegalMonitorStateException");
}

TEST(JTCException_Subclasses, IllegalArgumentException)
{
    JTCIllegalArgumentException e("bad arg");
    EXPECT_STREQ(e.getType(), "JTCIllegalArgumentException");
}

TEST(JTCException_Subclasses, SystemCallException)
{
    JTCSystemCallException e("open failed", ENOENT);
    EXPECT_STREQ(e.getType(), "JTCSystemCallException");
    EXPECT_EQ(e.getError(), ENOENT);
}

TEST(JTCException_Subclasses, OutOfMemoryError)
{
    JTCOutOfMemoryError e("oom");
    EXPECT_STREQ(e.getType(), "JTCOutOfMemoryError");
}

TEST(JTCException_Subclasses, UnknownThreadException)
{
    JTCUnknownThreadException e("who?");
    EXPECT_STREQ(e.getType(), "JTCUnknownThreadException");
}

TEST(JTCException_Subclasses, InitializeError)
{
    JTCInitializeError e("init failed");
    EXPECT_STREQ(e.getType(), "JTCInitializeError");
}

// ---------------------------------------------------------------------------
// JTCThreadDeath is NOT derived from JTCException
// ---------------------------------------------------------------------------

TEST(JTCException_Subclasses, ThreadDeathIsNotException)
{
    // JTCThreadDeath is its own class -- this is a compile-time check.
    // If someone changed the hierarchy, this static_assert would fail.
    static_assert(!std::is_base_of<JTCException, JTCThreadDeath>::value,
                  "JTCThreadDeath must not derive from JTCException");
}
