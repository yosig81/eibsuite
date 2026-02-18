#include "RoutingIndication.h"
#include "DescriptionRequest.h"
#include "CCemi_L_Data_Frame.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <vector>

using namespace EIBStdLibTest;
using namespace EibStack;

namespace {
template <typename Packet>
std::vector<unsigned char> Serialize(Packet& packet) {
    std::vector<unsigned char> out(packet.GetTotalSize(), 0);
    packet.FillBuffer(out.data(), static_cast<int>(out.size()));
    return out;
}
}  // namespace

class RoutingIndicationDescriptionRequestTest : public BaseTestFixture {};

TEST_F(
    RoutingIndicationDescriptionRequestTest,
    RoutingIndication_RoundTripPreservesCemiFields) {
    unsigned char value[] = {GROUP_WRITE, 0x2A, 0x7B};
    CCemi_L_Data_Frame cemi(
        L_DATA_IND, CEibAddress("1.1.10"), CEibAddress("3/4/5"), value, 3);
    cemi.SetTPCI(0x51);
    cemi.SetPriority(PRIORITY_URGENT);
    cemi.SetRepeatFlag(false);
    cemi.SetAcknowledgeRequest(true);

    CRoutingIndication indication(cemi);
    std::vector<unsigned char> raw = Serialize(indication);

    unsigned char* ptr = raw.data();
    CRoutingIndication parsed(ptr);
    const CCemi_L_Data_Frame& parsed_cemi = parsed.GetCemiFrame();

    EXPECT_EQ(L_DATA_IND, parsed_cemi.GetMessageCode());
    EXPECT_STREQ("1.1.10", parsed_cemi.GetSourceAddress().ToString().GetBuffer());
    EXPECT_STREQ("3/4/5", parsed_cemi.GetDestAddress().ToString().GetBuffer());
    EXPECT_EQ(3, parsed_cemi.GetValueLength());
    EXPECT_EQ(GROUP_WRITE, parsed_cemi.GetAPCI());
    EXPECT_EQ(0x51, parsed_cemi.GetTPCI());
    ASSERT_NE(nullptr, parsed_cemi.GetAddilData());
    EXPECT_EQ(0x2A, parsed_cemi.GetAddilData()[0]);
    EXPECT_EQ(0x7B, parsed_cemi.GetAddilData()[1]);
    EXPECT_EQ(PRIORITY_URGENT, parsed_cemi.GetPriority());
    EXPECT_FALSE(parsed_cemi.IsRepeatedFrame());
    EXPECT_TRUE(parsed_cemi.GetAcknowledgeRequested());
}

TEST_F(
    RoutingIndicationDescriptionRequestTest,
    RoutingIndication_FillBufferThrowsOnTooSmallBuffer) {
    unsigned char value[] = {GROUP_READ};
    CCemi_L_Data_Frame cemi(
        L_DATA_IND, CEibAddress("1.1.1"), CEibAddress("1/2/3"), value, 1);
    CRoutingIndication indication(cemi);

    std::vector<unsigned char> too_small(indication.GetTotalSize() - 1, 0);
    EXPECT_THROW(
        indication.FillBuffer(too_small.data(), static_cast<int>(too_small.size())),
        CEIBException);
}

TEST_F(
    RoutingIndicationDescriptionRequestTest,
    DescriptionRequest_RoundTripPreservesAddressPortAndHeader) {
    CDescriptionRequest request("10.20.30.40", 3671);
    std::vector<unsigned char> raw = Serialize(request);

    EXPECT_EQ(HEADER_SIZE_10, raw[0]);
    EXPECT_EQ(EIBNETIP_VERSION_10, raw[1]);
    EXPECT_EQ(static_cast<unsigned char>((DESCRIPTION_REQUEST >> 8) & 0xFF), raw[2]);
    EXPECT_EQ(static_cast<unsigned char>(DESCRIPTION_REQUEST & 0xFF), raw[3]);
    EXPECT_EQ(0x0E, raw[12]);  // network-order port high byte (3671)
    EXPECT_EQ(0x57, raw[13]);  // network-order port low byte

    unsigned char* ptr = raw.data();
    CDescriptionRequest parsed(ptr);
    EXPECT_STREQ("10.20.30.40", parsed.GetAddress().GetBuffer());
    EXPECT_EQ(3671, parsed.GetPort());
}

TEST_F(
    RoutingIndicationDescriptionRequestTest,
    DescriptionRequest_ParseRejectsInvalidHpaiLength) {
    CDescriptionRequest request("192.168.1.10", 4000);
    std::vector<unsigned char> raw = Serialize(request);

    raw[6] = 7;  // HPAI struct length must be 8
    unsigned char* ptr = raw.data();
    EXPECT_THROW(CDescriptionRequest parsed(ptr), CEIBException);
}

TEST_F(
    RoutingIndicationDescriptionRequestTest,
    DescriptionRequest_ParseRejectsUnsupportedHostProtocol) {
    CDescriptionRequest request("192.168.1.10", 4000);
    std::vector<unsigned char> raw = Serialize(request);

    raw[7] = IPV4_TCP;  // parser supports only IPV4_UDP
    unsigned char* ptr = raw.data();
    EXPECT_THROW(CDescriptionRequest parsed(ptr), CEIBException);
}
