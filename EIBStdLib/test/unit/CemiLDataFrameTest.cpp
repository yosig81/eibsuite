#include <gtest/gtest.h>
#include "CCemi_L_Data_Frame.h"
#include "EIBAddress.h"
#include "CException.h"
#include "EibNetwork.h"
#include "../fixtures/TestHelpers.h"
#include <vector>

using namespace EIBStdLibTest;
using namespace EibStack;

class CemiLDataFrameTest : public BaseTestFixture {};

TEST_F(CemiLDataFrameTest, Constructor_RejectsUnsupportedMessageCode) {
    unsigned char value[] = {GROUP_READ};
    EXPECT_THROW(
        CCemi_L_Data_Frame(0x55, CEibAddress("1.1.1"), CEibAddress("1/2/3"), value, 1),
        CEIBException);
}

TEST_F(CemiLDataFrameTest, Constructor_RejectsGroupSourceAddress) {
    unsigned char value[] = {GROUP_READ};
    EXPECT_THROW(
        CCemi_L_Data_Frame(L_DATA_REQ, CEibAddress("1/2/3"), CEibAddress("1/2/4"), value, 1),
        CEIBException);
}

TEST_F(CemiLDataFrameTest, Constructor_RejectsInvalidLength) {
    unsigned char value[] = {GROUP_READ};
    EXPECT_THROW(
        CCemi_L_Data_Frame(L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("1/2/3"), value, 0),
        CEIBException);

    std::vector<unsigned char> too_long(MAX_EIB_VALUE_LEN + 1, 0x11);
    EXPECT_THROW(
        CCemi_L_Data_Frame(
            L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("1/2/3"), too_long.data(),
            static_cast<int>(too_long.size())),
        CEIBException);
}

TEST_F(CemiLDataFrameTest, RoundTrip_PreservesAddressesFlagsAndPayload) {
    unsigned char payload[] = {GROUP_WRITE, 0x34, 0x56};
    CCemi_L_Data_Frame frame(
        L_DATA_REQ, CEibAddress("1.1.11"), CEibAddress("2/3/4"), payload, 3);
    frame.SetTPCI(0xA1);
    frame.SetAcknowledgeRequest(true);
    frame.SetRepeatFlag(false);
    frame.SetPriority(PRIORITY_LOW);

    std::vector<unsigned char> raw(frame.GetTotalSize(), 0);
    frame.FillBuffer(raw.data(), static_cast<int>(raw.size()));

    CCemi_L_Data_Frame parsed(raw.data());
    EXPECT_EQ(L_DATA_REQ, parsed.GetMessageCode());
    EXPECT_STREQ("1.1.11", parsed.GetSourceAddress().ToString().GetBuffer());
    EXPECT_STREQ("2/3/4", parsed.GetDestAddress().ToString().GetBuffer());
    EXPECT_TRUE(parsed.GetDestAddress().IsGroupAddress());
    EXPECT_EQ(3, parsed.GetValueLength());
    EXPECT_EQ(GROUP_WRITE, parsed.GetAPCI());
    ASSERT_NE(nullptr, parsed.GetAddilData());
    EXPECT_EQ(0x34, parsed.GetAddilData()[0]);
    EXPECT_EQ(0x56, parsed.GetAddilData()[1]);
    EXPECT_EQ(0xA1, parsed.GetTPCI());
    EXPECT_TRUE(parsed.GetAcknowledgeRequested());
    EXPECT_FALSE(parsed.IsRepeatedFrame());
    EXPECT_EQ(PRIORITY_LOW, parsed.GetPriority());
    EXPECT_EQ(6, parsed.GetHopCount());
    EXPECT_FALSE(parsed.IsExtendedFrame());
}

TEST_F(CemiLDataFrameTest, FillBufferWithFrameData_ReturnsTpduBytes) {
    unsigned char payload[] = {GROUP_RESPONSE, 0xAB, 0xCD};
    CCemi_L_Data_Frame frame(
        L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("0/1/2"), payload, 3);

    unsigned char tpdu[3] = {0, 0, 0};
    frame.FillBufferWithFrameData(tpdu, 3);

    EXPECT_EQ(GROUP_RESPONSE, tpdu[0]);
    EXPECT_EQ(0xAB, tpdu[1]);
    EXPECT_EQ(0xCD, tpdu[2]);
}

TEST_F(CemiLDataFrameTest, SetDestAddress_TogglesGroupFlag) {
    unsigned char payload[] = {GROUP_READ};
    CCemi_L_Data_Frame frame(
        L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("3.3.3"), payload, 1);
    EXPECT_FALSE(frame.GetDestAddress().IsGroupAddress());

    frame.SetDestAddress(CEibAddress("4/5/6"));
    EXPECT_TRUE(frame.GetDestAddress().IsGroupAddress());
    EXPECT_STREQ("4/5/6", frame.GetDestAddress().ToString().GetBuffer());

    frame.SetDestAddress(CEibAddress("6.7.8"));
    EXPECT_FALSE(frame.GetDestAddress().IsGroupAddress());
    EXPECT_STREQ("6.7.8", frame.GetDestAddress().ToString().GetBuffer());
}

TEST_F(CemiLDataFrameTest, Parse_RejectsAdditionalInfoLength) {
    unsigned char payload[] = {GROUP_READ};
    CCemi_L_Data_Frame frame(
        L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("1/2/3"), payload, 1);
    std::vector<unsigned char> raw(frame.GetTotalSize(), 0);
    frame.FillBuffer(raw.data(), static_cast<int>(raw.size()));

    raw[1] = 1;  // addil must be 0 for current implementation
    EXPECT_THROW(CCemi_L_Data_Frame parsed(raw.data()), CEIBException);
}

TEST_F(CemiLDataFrameTest, FillBuffer_ThrowsWhenBufferTooSmall) {
    unsigned char payload[] = {GROUP_RESPONSE, 0x10};
    CCemi_L_Data_Frame frame(
        L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("1/2/3"), payload, 2);
    std::vector<unsigned char> raw(frame.GetTotalSize() - 1, 0);
    EXPECT_THROW(frame.FillBuffer(raw.data(), static_cast<int>(raw.size())), CEIBException);
}

TEST_F(CemiLDataFrameTest, ConfirmationFlag_ReflectsControlBit) {
    unsigned char payload[] = {GROUP_RESPONSE};
    CCemi_L_Data_Frame frame(
        L_DATA_CON, CEibAddress("1.1.1"), CEibAddress("1/2/3"), payload, 1);
    EXPECT_TRUE(frame.IsPositiveConfirmation());

    frame.SetCtrl1(static_cast<unsigned char>(frame.GetCtrl1() | 0x01));
    EXPECT_FALSE(frame.IsPositiveConfirmation());
}
