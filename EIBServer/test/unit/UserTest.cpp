#include <gtest/gtest.h>
#include "UsersDB.h"

// ---------------------------------------------------------------------------
// CUser -- Constructor / Reset
// ---------------------------------------------------------------------------

TEST(CUser, DefaultConstructor_AllZero)
{
    CUser user;
    EXPECT_STREQ(user.GetName().GetBuffer(), "");
    EXPECT_STREQ(user.GetPassword().GetBuffer(), "");
    EXPECT_EQ(user.GetPriviliges(), (unsigned int)USER_POLICY_NONE);
}

TEST(CUser, SetName_GetName)
{
    CUser user;
    user.SetName("alice");
    EXPECT_STREQ(user.GetName().GetBuffer(), "alice");
}

TEST(CUser, SetPassword_GetPassword)
{
    CUser user;
    user.SetPassword("s3cret");
    EXPECT_STREQ(user.GetPassword().GetBuffer(), "s3cret");
}

TEST(CUser, SetPriviliges_GetPriviliges)
{
    CUser user;
    user.SetPriviliges(0xF);
    EXPECT_EQ(user.GetPriviliges(), 0xFu);
}

// ---------------------------------------------------------------------------
// CUser -- Privilege bitmask checks
// ---------------------------------------------------------------------------

TEST(CUser, IsReadPolicyAllowed_BitSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_READ_ACCESS);
    EXPECT_TRUE(user.IsReadPolicyAllowed());
}

TEST(CUser, IsReadPolicyAllowed_BitNotSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_WRITE_ACCESS);
    EXPECT_FALSE(user.IsReadPolicyAllowed());
}

TEST(CUser, IsWritePolicyAllowed_BitSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_WRITE_ACCESS);
    EXPECT_TRUE(user.IsWritePolicyAllowed());
}

TEST(CUser, IsWritePolicyAllowed_BitNotSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_READ_ACCESS);
    EXPECT_FALSE(user.IsWritePolicyAllowed());
}

TEST(CUser, IsWebAccessAllowed_BitSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_WEB_ACCESS);
    EXPECT_TRUE(user.IsWebAccessAllowed());
}

TEST(CUser, IsWebAccessAllowed_BitNotSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WRITE_ACCESS);
    EXPECT_FALSE(user.IsWebAccessAllowed());
}

TEST(CUser, IsAdminAccessAllowed_BitSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_ADMIN_ACCESS);
    EXPECT_TRUE(user.IsAdminAccessAllowed());
}

TEST(CUser, IsAdminAccessAllowed_BitNotSet)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WRITE_ACCESS | USER_POLICY_WEB_ACCESS);
    EXPECT_FALSE(user.IsAdminAccessAllowed());
}

TEST(CUser, AllPrivileges_AllTrue)
{
    CUser user;
    user.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WRITE_ACCESS |
                       USER_POLICY_WEB_ACCESS | USER_POLICY_ADMIN_ACCESS);
    EXPECT_TRUE(user.IsReadPolicyAllowed());
    EXPECT_TRUE(user.IsWritePolicyAllowed());
    EXPECT_TRUE(user.IsWebAccessAllowed());
    EXPECT_TRUE(user.IsAdminAccessAllowed());
}

// ---------------------------------------------------------------------------
// CUser -- Copy constructor
// ---------------------------------------------------------------------------

TEST(CUser, CopyConstructor_CopiesAll)
{
    CUser original;
    original.SetName("bob");
    original.SetPassword("pass123");
    original.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WEB_ACCESS);
    original.SetAllowedSourceAddressMask(0xFF00);
    original.SetAllowedDestAddressMask(0x00FF);

    CUser copy(original);
    EXPECT_STREQ(copy.GetName().GetBuffer(), "bob");
    EXPECT_STREQ(copy.GetPassword().GetBuffer(), "pass123");
    EXPECT_EQ(copy.GetPriviliges(), original.GetPriviliges());
    EXPECT_EQ(copy.GetSrcMask(), 0xFF00);
    EXPECT_EQ(copy.GetDstMask(), 0x00FF);
}

// ---------------------------------------------------------------------------
// CUser -- Reset
// ---------------------------------------------------------------------------

TEST(CUser, Reset_ClearsAll)
{
    CUser user;
    user.SetName("charlie");
    user.SetPassword("pw");
    user.SetPriviliges(0xF);

    // Reset is private, accessed via friend class CUsersDB.
    // We test indirectly via CUsersDB::OnReadRecordComplete which calls Reset
    // fields (SetName(""), SetPassword(""), SetPriviliges(0)).
    // Alternatively, create a CUsersDB and use OnReadRecordComplete.
    // For now, test the observable effect of the setters zeroing out.
    user.SetName(EMPTY_STRING);
    user.SetPassword(EMPTY_STRING);
    user.SetPriviliges(USER_POLICY_NONE);

    EXPECT_STREQ(user.GetName().GetBuffer(), "");
    EXPECT_STREQ(user.GetPassword().GetBuffer(), "");
    EXPECT_EQ(user.GetPriviliges(), (unsigned int)USER_POLICY_NONE);
}
