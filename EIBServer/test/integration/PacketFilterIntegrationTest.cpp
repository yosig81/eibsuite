// PacketFilterIntegrationTest.cpp -- Verify user masks loaded from Users.db
// and packet filter logic works end-to-end.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class PacketFilterIntegrationTest : public ::testing::Test {
protected:
    CUsersDB& usersdb() {
        return CEIBServer::GetInstance().GetUsersDB();
    }

    // Look up a user by name and return their CUser record
    bool GetUser(const CString& name, CUser& user) {
        return usersdb().GetRecord(name, user);
    }
};

TEST_F(PacketFilterIntegrationTest, AdminMasksAreFullAccess)
{
    CUser user;
    ASSERT_TRUE(GetUser("admin", user));
    EXPECT_EQ(user.GetSrcMask(), 0xFFFF);
    EXPECT_EQ(user.GetDstMask(), 0xFFFF);
}

TEST_F(PacketFilterIntegrationTest, NoAccessMasksAreZero)
{
    CUser user;
    ASSERT_TRUE(GetUser("noaccess", user));
    EXPECT_EQ(user.GetSrcMask(), 0x0000);
    EXPECT_EQ(user.GetDstMask(), 0x0000);
}

TEST_F(PacketFilterIntegrationTest, RestrictedDestMaskLoaded)
{
    CUser user;
    ASSERT_TRUE(GetUser("restricted_dest", user));
    EXPECT_EQ(user.GetSrcMask(), 0xFFFF);
    EXPECT_EQ(user.GetDstMask(), 0xFF00);
}

TEST_F(PacketFilterIntegrationTest, RestrictedSrcMaskLoaded)
{
    CUser user;
    ASSERT_TRUE(GetUser("restricted_src", user));
    EXPECT_EQ(user.GetSrcMask(), 0xFF00);
    EXPECT_EQ(user.GetDstMask(), 0xFFFF);
}

TEST_F(PacketFilterIntegrationTest, RestrictedBothMasksLoaded)
{
    CUser user;
    ASSERT_TRUE(GetUser("restricted_both", user));
    EXPECT_EQ(user.GetSrcMask(), 0xFF00);
    EXPECT_EQ(user.GetDstMask(), 0xFF00);
}

TEST_F(PacketFilterIntegrationTest, FilterLogicMatchesMasks)
{
    // Address "1/2/3" -> 0x0A03 (main=1, middle=2, sub=3 -> 5+3+8 bits)
    // high byte = 0x0A (non-zero), so (0xFF00 & 0x0A03) = 0x0A00 != 0 -> passes
    // Address "0/0/1" -> 0x0001
    // high byte = 0x00, so (0xFF00 & 0x0001) = 0x0000 -> blocked by 0xFF00

    CUser restricted_dest;
    ASSERT_TRUE(GetUser("restricted_dest", restricted_dest));
    const CPacketFilter& filter = restricted_dest.GetFilter();

    // Build a frame with src=0.0.0 (default), dest=1/2/3 (0x0A03)
    CCemi_L_Data_Frame frame_allowed;
    frame_allowed.SetSrcAddress(CEibAddress());
    frame_allowed.SetDestAddress(CEibAddress("1/2/3"));
    EXPECT_TRUE(filter.IsPacketAllowed(frame_allowed))
        << "Dest 1/2/3 should be allowed by dest mask 0xFF00";

    // Build a frame with dest=0/0/1 (0x0001) -- should be blocked
    CCemi_L_Data_Frame frame_blocked;
    frame_blocked.SetSrcAddress(CEibAddress());
    frame_blocked.SetDestAddress(CEibAddress("0/0/1"));
    EXPECT_FALSE(filter.IsPacketAllowed(frame_blocked))
        << "Dest 0/0/1 should be blocked by dest mask 0xFF00";
}
