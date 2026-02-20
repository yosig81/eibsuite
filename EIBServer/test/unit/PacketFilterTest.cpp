#include <gtest/gtest.h>
#include "PacketFilter.h"
#include "CCemi_L_Data_Frame.h"

using namespace EibStack;

// Helper: build a minimal L_DATA_REQ frame with given source and dest addresses.
static CCemi_L_Data_Frame MakeFrame(unsigned short src, unsigned short dst)
{
    unsigned char val = 0x01;
    CCemi_L_Data_Frame frame(L_DATA_REQ,
                             CEibAddress(src, false),
                             CEibAddress(dst, true),
                             &val, 1);
    return frame;
}

// ---------------------------------------------------------------------------
// Constructor / ClearAllFilters
// ---------------------------------------------------------------------------

TEST(PacketFilter, DefaultConstructor_MasksAreFFFF)
{
    CPacketFilter filter;
    EXPECT_EQ(filter.GetSrcMask(), 0xFFFF);
    EXPECT_EQ(filter.GetDstMask(), 0xFFFF);
}

TEST(PacketFilter, ClearAllFilters_SetsToZero)
{
    CPacketFilter filter;
    filter.ClearAllFilters();
    EXPECT_EQ(filter.GetSrcMask(), 0);
    EXPECT_EQ(filter.GetDstMask(), 0);
}

// ---------------------------------------------------------------------------
// Mask getters/setters
// ---------------------------------------------------------------------------

TEST(PacketFilter, SetSrcMask_GetSrcMask)
{
    CPacketFilter filter;
    filter.SetAllowedSourceAddressMask(0x1234);
    EXPECT_EQ(filter.GetSrcMask(), 0x1234);
}

TEST(PacketFilter, SetDstMask_GetDstMask)
{
    CPacketFilter filter;
    filter.SetAllowedDestAddressMask(0xABCD);
    EXPECT_EQ(filter.GetDstMask(), 0xABCD);
}

// ---------------------------------------------------------------------------
// IsPacketAllowed -- default masks (0xFFFF) allow everything
// ---------------------------------------------------------------------------

TEST(PacketFilter, AllowAll_AnyPacketPasses)
{
    CPacketFilter filter; // masks default to 0xFFFF
    CCemi_L_Data_Frame frame = MakeFrame(0x1234, 0x5678);
    EXPECT_TRUE(filter.IsPacketAllowed(frame));
}

// ---------------------------------------------------------------------------
// IsPacketAllowed -- mask = 0 blocks all (when mask != 0xFFFF)
// ---------------------------------------------------------------------------

TEST(PacketFilter, SrcMaskZero_BlocksAll)
{
    CPacketFilter filter;
    filter.SetAllowedSourceAddressMask(0);
    // dst mask stays 0xFFFF
    CCemi_L_Data_Frame frame = MakeFrame(0x1234, 0x5678);
    EXPECT_FALSE(filter.IsPacketAllowed(frame));
}

TEST(PacketFilter, DstMaskZero_BlocksAll)
{
    CPacketFilter filter;
    filter.SetAllowedDestAddressMask(0);
    // src mask stays 0xFFFF
    CCemi_L_Data_Frame frame = MakeFrame(0x1234, 0x5678);
    EXPECT_FALSE(filter.IsPacketAllowed(frame));
}

// ---------------------------------------------------------------------------
// IsPacketAllowed -- partial masks
// ---------------------------------------------------------------------------

TEST(PacketFilter, SrcMask_MatchingAddress)
{
    CPacketFilter filter;
    // mask = 0xFF00, src addr high byte must have bits set
    filter.SetAllowedSourceAddressMask(0xFF00);
    CCemi_L_Data_Frame frame = MakeFrame(0x1200, 0x0001);
    // (0xFF00 & 0x1200) = 0x1200 != 0 -> allowed
    EXPECT_TRUE(filter.IsPacketAllowed(frame));
}

TEST(PacketFilter, SrcMask_NonMatchingAddress)
{
    CPacketFilter filter;
    filter.SetAllowedSourceAddressMask(0xFF00);
    CCemi_L_Data_Frame frame = MakeFrame(0x0012, 0x0001);
    // (0xFF00 & 0x0012) = 0 and mask != 0xFFFF -> blocked
    EXPECT_FALSE(filter.IsPacketAllowed(frame));
}

TEST(PacketFilter, DstMask_MatchingAddress)
{
    CPacketFilter filter;
    filter.SetAllowedDestAddressMask(0x00FF);
    CCemi_L_Data_Frame frame = MakeFrame(0x0001, 0x0034);
    // (0x00FF & 0x0034) = 0x0034 != 0 -> allowed
    EXPECT_TRUE(filter.IsPacketAllowed(frame));
}

TEST(PacketFilter, DstMask_NonMatchingAddress)
{
    CPacketFilter filter;
    filter.SetAllowedDestAddressMask(0x00FF);
    CCemi_L_Data_Frame frame = MakeFrame(0x0001, 0x1200);
    // (0x00FF & 0x1200) = 0 and mask != 0xFFFF -> blocked
    EXPECT_FALSE(filter.IsPacketAllowed(frame));
}

TEST(PacketFilter, BothMasks_BothMustPass)
{
    CPacketFilter filter;
    filter.SetAllowedSourceAddressMask(0xFF00);
    filter.SetAllowedDestAddressMask(0x00FF);

    // src passes, dst passes
    EXPECT_TRUE(filter.IsPacketAllowed(MakeFrame(0x1200, 0x0034)));
    // src fails, dst passes
    EXPECT_FALSE(filter.IsPacketAllowed(MakeFrame(0x0012, 0x0034)));
    // src passes, dst fails
    EXPECT_FALSE(filter.IsPacketAllowed(MakeFrame(0x1200, 0x3400)));
}
