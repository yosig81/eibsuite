#include <gtest/gtest.h>
#include "CCemi_L_BusMon_Frame.h"
#include "CException.h"
#include "EIBNetIP.h"
#include "../fixtures/TestHelpers.h"
#include <vector>

using namespace EIBStdLibTest;
using namespace EibStack;

namespace {
std::vector<unsigned char> BuildBusMonPacket(
    unsigned char status,
    const std::vector<unsigned char>& raw_payload,
    const std::vector<unsigned char>& timestamp_bytes) {
    const unsigned char timestamp_type =
        (timestamp_bytes.size() == 4) ? 0x06 : 0x04;
    const unsigned char addinfo_len =
        static_cast<unsigned char>(3 + 2 + timestamp_bytes.size());
    const unsigned short payload_len =
        static_cast<unsigned short>(4 + 1 + 1 + addinfo_len + raw_payload.size());
    const unsigned short total_len = static_cast<unsigned short>(HEADER_SIZE_10 + payload_len);

    std::vector<unsigned char> packet(total_len, 0);
    packet[0] = HEADER_SIZE_10;
    packet[1] = EIBNETIP_VERSION_10;
    packet[2] = static_cast<unsigned char>((TUNNELLING_REQUEST >> 8) & 0xFF);
    packet[3] = static_cast<unsigned char>(TUNNELLING_REQUEST & 0xFF);
    packet[4] = static_cast<unsigned char>((total_len >> 8) & 0xFF);
    packet[5] = static_cast<unsigned char>(total_len & 0xFF);

    size_t i = HEADER_SIZE_10;
    packet[i++] = 4;              // CCH length
    packet[i++] = 1;              // channel id
    packet[i++] = 2;              // sequence counter
    packet[i++] = 0;              // type specific
    packet[i++] = L_BUSMON_IND;   // message code
    packet[i++] = addinfo_len;    // additional info length

    packet[i++] = 0x03;           // TYPEID_STATUSINFO
    packet[i++] = 1;              // status length
    packet[i++] = status;

    packet[i++] = timestamp_type; // TYPEID_TIMESTAMP/EXT
    packet[i++] = static_cast<unsigned char>(timestamp_bytes.size());
    for (size_t j = 0; j < timestamp_bytes.size(); ++j) {
        packet[i++] = timestamp_bytes[j];
    }

    for (size_t j = 0; j < raw_payload.size(); ++j) {
        packet[i++] = raw_payload[j];
    }

    return packet;
}
}  // namespace

class CemiLBusMonFrameTest : public BaseTestFixture {};

TEST_F(CemiLBusMonFrameTest, Parse_ExtractsStatusTimestampAndPayload) {
    std::vector<unsigned char> packet =
        BuildBusMonPacket(0xED, {0xAA, 0xBB}, {0x12, 0x34});

    CCemi_L_BusMon_Frame frame(packet.data(), static_cast<int>(packet.size()));

    EXPECT_EQ(L_BUSMON_IND, frame.GetMessageCode());
    EXPECT_EQ(5, frame.GetSequenceNumber());
    EXPECT_TRUE(frame.GetParityError());
    EXPECT_TRUE(frame.GetBitError());
    EXPECT_TRUE(frame.GetFrameError());
    EXPECT_TRUE(frame.GetLostFlag());
    EXPECT_EQ(0x1234, frame.GetTimeStamp().GetTime());

    ASSERT_EQ(2, frame.GetPayloadLength());
    ASSERT_NE(nullptr, frame.GetPayload());
    EXPECT_EQ(0xAA, frame.GetPayload()[0]);
    EXPECT_EQ(0xBB, frame.GetPayload()[1]);
    EXPECT_EQ(11, frame.GetStructLength());
}

TEST_F(CemiLBusMonFrameTest, Parse_RejectsWrongMessageCode) {
    std::vector<unsigned char> packet =
        BuildBusMonPacket(0x01, {0x11}, {0x12, 0x34});
    packet[10] = L_DATA_REQ;

    EXPECT_THROW(
        CCemi_L_BusMon_Frame frame(packet.data(), static_cast<int>(packet.size())),
        CEIBException);
}

TEST_F(CemiLBusMonFrameTest, Parse_RejectsTooShortAdditionalInfo) {
    std::vector<unsigned char> packet =
        BuildBusMonPacket(0x01, {0x11}, {0x12, 0x34});
    packet[11] = 6;

    EXPECT_THROW(
        CCemi_L_BusMon_Frame frame(packet.data(), static_cast<int>(packet.size())),
        CEIBException);
}

TEST_F(CemiLBusMonFrameTest, Parse_RejectsInvalidStatusLength) {
    std::vector<unsigned char> packet =
        BuildBusMonPacket(0x01, {0x11}, {0x12, 0x34});
    packet[13] = 2;

    EXPECT_THROW(
        CCemi_L_BusMon_Frame frame(packet.data(), static_cast<int>(packet.size())),
        CEIBException);
}

TEST_F(CemiLBusMonFrameTest, Parse_RejectsInvalidTimestampLength) {
    std::vector<unsigned char> packet =
        BuildBusMonPacket(0x01, {0x11}, {0x12, 0x34});
    packet[16] = 3;

    EXPECT_THROW(
        CCemi_L_BusMon_Frame frame(packet.data(), static_cast<int>(packet.size())),
        CEIBException);
}

TEST_F(CemiLBusMonFrameTest, Parse_RejectsTooShortFrame) {
    std::vector<unsigned char> packet =
        BuildBusMonPacket(0x01, {0x11}, {0x12, 0x34});

    EXPECT_THROW(CCemi_L_BusMon_Frame frame(packet.data(), 13), CEIBException);
}
