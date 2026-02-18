#include <gtest/gtest.h>
#include "TunnelRequest.h"
#include "CCemi_L_Data_Frame.h"
#include "EIBAddress.h"
#include "cEMI.h"
#include "CException.h"
#include "EIBNetIP.h"
#include "../fixtures/TestHelpers.h"
#include <cstring>
#include <vector>

using namespace EIBStdLibTest;
using namespace EibStack;

namespace {
std::vector<unsigned char> Serialize(CTunnelingRequest& packet) {
    std::vector<unsigned char> out(packet.GetTotalSize(), 0);
    packet.FillBuffer(out.data(), static_cast<int>(out.size()));
    return out;
}
}  // namespace

class TunnelRequestTest : public BaseTestFixture {};

TEST_F(TunnelRequestTest, RoundTrip_PreservesChannelSequenceAndCemiPayload) {
    unsigned char payload[] = {GROUP_WRITE, 0xAA, 0xBB};
    CCemi_L_Data_Frame cemi(
        L_DATA_REQ, CEibAddress("1.1.10"), CEibAddress("2/3/4"), payload, 3);

    CTunnelingRequest request(0x21, 0x34, cemi);
    std::vector<unsigned char> first = Serialize(request);

    unsigned char* ptr = first.data();
    CTunnelingRequest parsed(ptr);
    std::vector<unsigned char> second = Serialize(parsed);

    EXPECT_EQ(TUNNELLING_REQUEST, parsed.GetServiceType());
    EXPECT_EQ(0x21, parsed.GetChannelId());
    EXPECT_EQ(0x34, parsed.GetSequenceNumber());
    EXPECT_EQ(HEADER_SIZE_10 + 4 + cemi.GetTotalSize(), parsed.GetTotalSize());

    const CCemi_L_Data_Frame& parsed_cemi = parsed.GetcEMI();
    EXPECT_EQ(L_DATA_REQ, parsed_cemi.GetMessageCode());
    EXPECT_STREQ("1.1.10", parsed_cemi.GetSourceAddress().ToString().GetBuffer());
    EXPECT_STREQ("2/3/4", parsed_cemi.GetDestAddress().ToString().GetBuffer());
    EXPECT_EQ(3, parsed_cemi.GetValueLength());
    EXPECT_EQ(GROUP_WRITE, parsed_cemi.GetAPCI());
    ASSERT_NE(nullptr, parsed_cemi.GetAddilData());
    EXPECT_EQ(0xAA, parsed_cemi.GetAddilData()[0]);
    EXPECT_EQ(0xBB, parsed_cemi.GetAddilData()[1]);

    ASSERT_EQ(first.size(), second.size());
    EXPECT_EQ(0, std::memcmp(first.data(), second.data(), first.size()));
}

TEST_F(TunnelRequestTest, FillBuffer_ThrowsWhenTargetBufferTooSmall) {
    unsigned char payload[] = {GROUP_READ};
    CCemi_L_Data_Frame cemi(
        L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("1/2/3"), payload, 1);
    CTunnelingRequest request(1, 2, cemi);

    std::vector<unsigned char> too_small(request.GetTotalSize() - 1, 0);
    EXPECT_THROW(
        request.FillBuffer(too_small.data(), static_cast<int>(too_small.size())), CEIBException);
}
