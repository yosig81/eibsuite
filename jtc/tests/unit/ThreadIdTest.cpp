#include <JTC.h>
#include <gtest/gtest.h>
#include <cstring>

// ---------------------------------------------------------------------------
// JTCThreadId value semantics
// ---------------------------------------------------------------------------

TEST(JTCThreadId_Basic, DefaultCtorIsNull)
{
    JTCThreadId id;
    JTCThreadId null_id;
    EXPECT_EQ(id, null_id);
}

TEST(JTCThreadId_Basic, SelfIsNonNull)
{
    JTCThreadId self = JTCThreadId::self();
    JTCThreadId null_id;
    EXPECT_NE(self, null_id);
}

TEST(JTCThreadId_Basic, SelfEqualsSelf)
{
    JTCThreadId a = JTCThreadId::self();
    JTCThreadId b = JTCThreadId::self();
    EXPECT_EQ(a, b);
}

TEST(JTCThreadId_Basic, NullNotEqualSelf)
{
    JTCThreadId null_id;
    JTCThreadId self = JTCThreadId::self();
    EXPECT_NE(null_id, self);
}

TEST(JTCThreadId_Basic, CopySemantic)
{
    JTCThreadId orig = JTCThreadId::self();
    JTCThreadId copy(orig);
    EXPECT_EQ(orig, copy);
}

TEST(JTCThreadId_Basic, AssignmentSemantic)
{
    JTCThreadId a = JTCThreadId::self();
    JTCThreadId b;
    EXPECT_NE(a, b);
    b = a;
    EXPECT_EQ(a, b);
}

TEST(JTCThreadId_Basic, ToStringProducesOutput)
{
    JTCThreadId self = JTCThreadId::self();
    char buf[64] = {0};
    char* result = self.toString(buf, sizeof(buf));
    EXPECT_EQ(result, buf);
    EXPECT_GT(std::strlen(buf), 0u);
}

TEST(JTCThreadId_Basic, ToStringRespectsBufferSize)
{
    JTCThreadId self = JTCThreadId::self();
    char buf[4] = {0};
    self.toString(buf, sizeof(buf));
    // Must be null-terminated within buf size
    EXPECT_EQ(buf[3], '\0');
}
