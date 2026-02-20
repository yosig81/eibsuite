#include "DescriptionResponse.h"
#include "SearchResponse.h"
#include "EIBNetIP.h"
#include "../fixtures/TestHelpers.h"
#include <vector>
#include <cstring>

using namespace EIBStdLibTest;
using namespace EibStack;

namespace {
unsigned long MakeAddressBytes(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    unsigned char bytes[4] = {a, b, c, d};
    unsigned long out = 0;
    std::memcpy(&out, bytes, sizeof(bytes));
    return out;
}
}  // namespace

class DescriptionSearchResponseTest : public BaseTestFixture {};

TEST_F(DescriptionSearchResponseTest, DescriptionResponse_StandaloneRoundTripPreservesFields) {
    const char serial[6] = {1, 2, 3, 4, 5, 6};
    const char mac[6] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15};
    const int services = SERVICE_CORE | SERVICE_TUNNELING | SERVICE_ROUTING;

    CDescriptionResponse response(
        MEDIUM_TP1,
        CEibAddress("1.2.3"),
        0x1234,
        serial,
        MakeAddressBytes(224, 0, 23, 12),
        mac,
        "Device Alpha",
        services);

    std::vector<unsigned char> bytes(response.GetTotalSize(), 0);
    response.FillBuffer(bytes.data(), static_cast<int>(bytes.size()), true);

    // Verify the EIBnet/IP header is present and not overwritten by payload bytes.
    EXPECT_EQ(HEADER_SIZE_10, bytes[0]);
    EXPECT_EQ(EIBNETIP_VERSION_10, bytes[1]);
    EXPECT_EQ(static_cast<unsigned char>((DESCRIPTION_RESPONSE >> 8) & 0xFF), bytes[2]);
    EXPECT_EQ(static_cast<unsigned char>(DESCRIPTION_RESPONSE & 0xFF), bytes[3]);

    CDescriptionResponse parsed(bytes.data(), static_cast<int>(bytes.size()));
    EXPECT_EQ(MEDIUM_TP1, parsed.GetMedium());
    EXPECT_STREQ("1.2.3", parsed.GetAddress().ToString().GetBuffer());
    EXPECT_EQ(0x1234, parsed.GetProjectInstallID());
    EXPECT_STREQ("01-02-03-04-05-06", parsed.GetSerialNum().GetBuffer());
    EXPECT_STREQ("10:11:12:13:14:15", parsed.GetMACAddress().GetBuffer());
    EXPECT_STREQ("224.0.23.12", parsed.GetMulticastAddress().GetBuffer());
    EXPECT_STREQ("Device Alpha", parsed.GetName().GetBuffer());
    EXPECT_EQ(services, parsed.GetSupportedServicesMask());
}

TEST_F(DescriptionSearchResponseTest, SearchResponse_RoundTripPreservesControlAndDescription) {
    const char serial[6] = {0x21, 0x22, 0x23, 0x24, 0x25, 0x26};
    const char mac[6] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36};
    const int services = SERVICE_CORE | SERVICE_DEV_MNGMT | SERVICE_TUNNELING;

    CSearchResponse response(
        "192.168.50.7",
        3671,
        MEDIUM_TP1,
        CEibAddress("2.3.4"),
        0x4567,
        serial,
        MakeAddressBytes(239, 255, 255, 250),
        mac,
        "SearchDevice",
        services);

    std::vector<unsigned char> bytes(response.GetTotalSize(), 0);
    response.FillBuffer(bytes.data(), static_cast<int>(bytes.size()));

    CSearchResponse parsed(bytes.data(), static_cast<int>(bytes.size()));
    EXPECT_STREQ("192.168.50.7", parsed.GetControlIPAddress().GetBuffer());
    EXPECT_EQ(3671, parsed.GetControlPort());

    const CDescriptionResponse& desc = parsed.GetDeviceDescription();
    EXPECT_EQ(MEDIUM_TP1, desc.GetMedium());
    EXPECT_STREQ("2.3.4", desc.GetAddress().ToString().GetBuffer());
    EXPECT_EQ(0x4567, desc.GetProjectInstallID());
    EXPECT_STREQ("SearchDevice", desc.GetName().GetBuffer());
    EXPECT_EQ(services, desc.GetSupportedServicesMask());
}

TEST_F(DescriptionSearchResponseTest, DescriptionResponse_StringConvertersCoverKnownAndUnknownValues) {
    CString medium;
    CDescriptionResponse::KNXMediumToString(MEDIUM_TP1, medium);
    EXPECT_STREQ("twisted pair 1 (9600 bit/s)", medium.GetBuffer());

    CDescriptionResponse::KNXMediumToString(static_cast<KNXMedium>(0x77), medium);
    EXPECT_EQ((size_t)0, medium.Find("Unknown"));

    CString services;
    CDescriptionResponse::SupportedServicesToString(
        SERVICE_CORE | SERVICE_ROUTING | SERVICE_OBJSRV, services);
    EXPECT_NE(CString(services).Find("Core"), string::npos);
    EXPECT_NE(CString(services).Find("Routing"), string::npos);
    EXPECT_NE(CString(services).Find("Object-Server"), string::npos);
}
