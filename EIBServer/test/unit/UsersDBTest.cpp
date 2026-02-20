#include <gtest/gtest.h>
#include "UsersDB.h"

// ---------------------------------------------------------------------------
// Test fixture -- provides a pre-populated CUsersDB for most tests
// ---------------------------------------------------------------------------

class UsersDBTest : public ::testing::Test {
protected:
    CUsersDB db;

    void SetUp() override
    {
        CUser alice;
        alice.SetName("alice");
        alice.SetPassword("secret");
        alice.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WEB_ACCESS);

        CUser bob;
        bob.SetName("bob");
        bob.SetPassword("hunter2");
        bob.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WRITE_ACCESS);

        db.AddRecord("alice", alice);
        db.AddRecord("bob", bob);
    }
};

// ---------------------------------------------------------------------------
// AuthenticateUser
// ---------------------------------------------------------------------------

TEST_F(UsersDBTest, AuthenticateUser_ValidCredentials)
{
    CUser user;
    EXPECT_TRUE(db.AuthenticateUser("alice", "secret", user));
    EXPECT_STREQ(user.GetName().GetBuffer(), "alice");
    EXPECT_TRUE(user.IsReadPolicyAllowed());
}

TEST_F(UsersDBTest, AuthenticateUser_WrongPassword)
{
    CUser user;
    EXPECT_FALSE(db.AuthenticateUser("alice", "wrong", user));
}

TEST_F(UsersDBTest, AuthenticateUser_UnknownUser)
{
    CUser user;
    EXPECT_FALSE(db.AuthenticateUser("charlie", "anything", user));
}

TEST_F(UsersDBTest, AuthenticateUser_EmptyDB)
{
    CUsersDB empty_db;
    CUser user;
    EXPECT_FALSE(empty_db.AuthenticateUser("alice", "secret", user));
}

// ---------------------------------------------------------------------------
// GetNumOfUsers
// ---------------------------------------------------------------------------

TEST(CUsersDB, GetNumOfUsers_Empty)
{
    CUsersDB db;
    EXPECT_EQ(db.GetNumOfUsers(), 0);
}

TEST_F(UsersDBTest, GetNumOfUsers_AfterAdd)
{
    EXPECT_EQ(db.GetNumOfUsers(), 2);
}

// ---------------------------------------------------------------------------
// OnReadParamComplete
// ---------------------------------------------------------------------------

TEST(CUsersDB, OnReadParamComplete_Password)
{
    CUsersDB db;
    CUser user;
    db.OnReadParamComplete(user, USER_PASSWORD_PARAM_NAME, "mypass");
    EXPECT_STREQ(user.GetPassword().GetBuffer(), "mypass");
}

TEST(CUsersDB, OnReadParamComplete_Privileges)
{
    CUsersDB db;
    CUser user;
    db.OnReadParamComplete(user, USER_PRIVILIGES_PARAM_NAME, "5");
    EXPECT_EQ(user.GetPriviliges(), 5u);
}

TEST(CUsersDB, OnReadParamComplete_SourceMask)
{
    CUsersDB db;
    CUser user;
    db.OnReadParamComplete(user, USER_ALLOWED_SOURCE_MASK, "0xFF00");
    EXPECT_EQ(user.GetSrcMask(), 0xFF00);
}

TEST(CUsersDB, OnReadParamComplete_DestMask)
{
    CUsersDB db;
    CUser user;
    db.OnReadParamComplete(user, USER_ALLOWED_DEST_MASK, "0x00FF");
    EXPECT_EQ(user.GetDstMask(), 0x00FF);
}

// ---------------------------------------------------------------------------
// OnReadRecordComplete / OnReadRecordNameComplete
// ---------------------------------------------------------------------------

TEST(CUsersDB, OnReadRecordComplete_AddsToMap)
{
    CUsersDB db;
    CUser user;
    db.OnReadRecordNameComplete(user, "testuser");
    user.SetPassword("pw");
    user.SetPriviliges(USER_POLICY_READ_ACCESS);
    db.OnReadRecordComplete(user);

    EXPECT_EQ(db.GetNumOfUsers(), 1);
    CUser found;
    EXPECT_TRUE(db.AuthenticateUser("testuser", "pw", found));
}

// ---------------------------------------------------------------------------
// OnSaveRecordStarted
// ---------------------------------------------------------------------------

TEST(CUsersDB, OnSaveRecordStarted_SerializesAllFields)
{
    CUsersDB db;
    CUser user;
    user.SetName("eve");
    user.SetPassword("pw");
    user.SetPriviliges(USER_POLICY_READ_ACCESS | USER_POLICY_WRITE_ACCESS |
                       USER_POLICY_WEB_ACCESS | USER_POLICY_ADMIN_ACCESS);
    user.SetAllowedSourceAddressMask(0xFF00);
    user.SetAllowedDestAddressMask(0x00FF);

    CString record_name;
    list<pair<CString,CString>> params;
    db.OnSaveRecordStarted(user, record_name, params);

    EXPECT_STREQ(record_name.GetBuffer(), "eve");
    EXPECT_EQ(params.size(), 4u);

    // Verify expected parameter names are present
    bool found_priv = false, found_pass = false, found_src = false, found_dst = false;
    for (auto& kv : params) {
        if (kv.first == USER_PRIVILIGES_PARAM_NAME) {
            found_priv = true;
            EXPECT_STREQ(kv.second.GetBuffer(), "15"); // 0xF = 15
        }
        if (kv.first == USER_PASSWORD_PARAM_NAME) {
            found_pass = true;
            EXPECT_STREQ(kv.second.GetBuffer(), "pw");
        }
        if (kv.first == USER_ALLOWED_SOURCE_MASK) found_src = true;
        if (kv.first == USER_ALLOWED_DEST_MASK) found_dst = true;
    }
    EXPECT_TRUE(found_priv);
    EXPECT_TRUE(found_pass);
    EXPECT_TRUE(found_src);
    EXPECT_TRUE(found_dst);
}
