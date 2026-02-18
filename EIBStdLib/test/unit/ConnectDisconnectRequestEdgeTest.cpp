#include "ConnectRequest.h"
#include "DisconnectRequest.h"
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

class ConnectDisconnectRequestEdgeTest : public BaseTestFixture {};

TEST_F(ConnectDisconnectRequestEdgeTest, ConnectRequest_ParseRejectsInvalidFirstHpaiLength) {
    CConnectRequest request(
        CConnectRequest::TunnelConnection, CConnectRequest::TunnelLinkLayer, 3671, "10.1.1.1");
    std::vector<unsigned char> raw = Serialize(request);

    raw[6] = 7;  // First HPAI length must be 8.
    unsigned char* ptr = raw.data();
    EXPECT_THROW(CConnectRequest parsed(ptr), CEIBException);
}

TEST_F(ConnectDisconnectRequestEdgeTest, ConnectRequest_ParseRejectsInvalidSecondHpaiProtocol) {
    CConnectRequest request(
        CConnectRequest::TunnelConnection, CConnectRequest::TunnelLinkLayer, 3671, "10.1.1.1");
    std::vector<unsigned char> raw = Serialize(request);

    raw[15] = IPV4_TCP;  // Second HPAI protocol must be UDP.
    unsigned char* ptr = raw.data();
    EXPECT_THROW(CConnectRequest parsed(ptr), CEIBException);
}

TEST_F(ConnectDisconnectRequestEdgeTest, ConnectRequest_ParseRejectsInvalidCriLength) {
    CConnectRequest request(
        CConnectRequest::TunnelConnection, CConnectRequest::TunnelLinkLayer, 3671, "10.1.1.1");
    std::vector<unsigned char> raw = Serialize(request);

    raw[22] = 3;  // CRI length must be 4.
    unsigned char* ptr = raw.data();
    EXPECT_THROW(CConnectRequest parsed(ptr), CEIBException);
}

TEST_F(ConnectDisconnectRequestEdgeTest, ConnectRequest_FillBufferThrowsWhenTooSmall) {
    CConnectRequest request(
        CConnectRequest::TunnelConnection, CConnectRequest::TunnelLinkLayer, 3671, "10.1.1.1");
    std::vector<unsigned char> too_small(request.GetTotalSize() - 1, 0);

    EXPECT_THROW(
        request.FillBuffer(too_small.data(), static_cast<int>(too_small.size())), CEIBException);
}

TEST_F(ConnectDisconnectRequestEdgeTest, DisconnectRequest_RoundTripPreservesReservedAndEndpoint) {
    CDisconnectRequest request(12, 4321, "192.168.2.33");
    std::vector<unsigned char> raw = Serialize(request);

    // Mutate reserved byte and ensure parser + getter reflect it.
    raw[7] = 0xAB;
    unsigned char* ptr = raw.data();
    CDisconnectRequest parsed(ptr);

    EXPECT_EQ(12, parsed.GetChannelID());
    EXPECT_EQ(0xAB, parsed.GetReserved());
    EXPECT_STREQ("192.168.2.33", parsed.GetControlAddr().GetBuffer());
    EXPECT_EQ(4321, parsed.GetControlPort());
}

TEST_F(ConnectDisconnectRequestEdgeTest, DisconnectRequest_ParseRejectsInvalidHpaiFields) {
    CDisconnectRequest request(12, 4321, "192.168.2.33");
    std::vector<unsigned char> raw = Serialize(request);

    raw[8] = 7;  // HPAI length must be 8.
    unsigned char* ptr1 = raw.data();
    EXPECT_THROW(CDisconnectRequest parsed(ptr1), CEIBException);

    raw = Serialize(request);
    raw[9] = IPV4_TCP;  // HPAI host protocol must be UDP.
    unsigned char* ptr2 = raw.data();
    EXPECT_THROW(CDisconnectRequest parsed(ptr2), CEIBException);
}
