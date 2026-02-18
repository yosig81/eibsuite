#include "EibStdLib.h"
#include "cEMI.h"
#include "ConnectRequest.h"
#include "ConnectResponse.h"
#include "ConnectionStateRequest.h"
#include "ConnectionStateResponse.h"
#include "DisconnectRequest.h"
#include "DisconnectResponse.h"
#include "DescriptionRequest.h"
#include "SearchRequest.h"
#include "TunnelAck.h"
#include "HPAI.h"
#include "CRI_CRD.h"
#include "../fixtures/TestHelpers.h"
#include <cstring>
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

class ProtocolPacketRoundTripTest : public BaseTestFixture {};

TEST_F(ProtocolPacketRoundTripTest, HPAI_RoundTripFromBytes) {
    unsigned char raw[8] = {8, IPV4_UDP, 192, 168, 10, 20, 0x0E, 0x57};
    CHPAI parsed(raw);

    EXPECT_STREQ("192.168.10.20", parsed.GetAddress().GetBuffer());
    EXPECT_EQ(3671, parsed.GetPort());
    EXPECT_EQ(IPV4_UDP, parsed.GetHostProtocol());
}

TEST_F(ProtocolPacketRoundTripTest, CRI_CRD_RoundTripFromBytes) {
    unsigned char options[2] = {0x12, 0x34};
    CCRI_CRD crd(TUNNEL_CONNECTION, options);
    CCRI_CRD parsed(crd.ToByteArray());

    EXPECT_EQ(static_cast<unsigned short>(TUNNEL_CONNECTION), parsed.GetConnectionTypeCode());
    EXPECT_EQ(0x12, parsed.GetProtocolIndependentData());
    EXPECT_EQ(0x34, parsed.GetProtocolDependentData());
    EXPECT_EQ(4, parsed.GetLength());
}

TEST_F(ProtocolPacketRoundTripTest, ConnectRequest_RoundTripPreservesFields) {
    CConnectRequest request(CConnectRequest::TunnelConnection,
                            CConnectRequest::TunnelLinkLayer,
                            3671,
                            "10.1.2.3");
    std::vector<unsigned char> raw = Serialize(request);

    unsigned char* ptr = raw.data();
    CConnectRequest parsed(ptr);

    EXPECT_EQ(CConnectRequest::TunnelConnection, parsed.GetConnectionType());
    EXPECT_STREQ("10.1.2.3", parsed.GetControlAddress().GetBuffer());
    EXPECT_EQ(3671, parsed.GetControlPort());
    EXPECT_STREQ("10.1.2.3", parsed.GetDataAddress().GetBuffer());
    EXPECT_EQ(3671, parsed.GetDataPort());
}

TEST_F(ProtocolPacketRoundTripTest, ConnectResponse_RoundTripPreservesFields) {
    CConnectResponse response(7, E_NO_ERROR, "172.16.1.9", 4001, CConnectRequest::TunnelConnection);
    std::vector<unsigned char> raw = Serialize(response);

    unsigned char* ptr = raw.data();
    CConnectResponse parsed(ptr);

    EXPECT_EQ(7, parsed.GetChannelID());
    EXPECT_EQ(E_NO_ERROR, parsed.GetStatus());
    EXPECT_STREQ("172.16.1.9", parsed.GetDataIPAddress().GetBuffer());
    EXPECT_EQ(4001, parsed.GetDataPort());
}

TEST_F(ProtocolPacketRoundTripTest, SearchRequest_RoundTripPreservesEndpoint) {
    CSearchRequest request(5555, "192.168.1.44");
    std::vector<unsigned char> raw = Serialize(request);

    unsigned char* ptr = raw.data();
    CSearchRequest parsed(ptr);

    EXPECT_STREQ("192.168.1.44", parsed.GetRemoteIPAddress().GetBuffer());
    EXPECT_EQ(5555, parsed.GetRemotePort());
}

TEST_F(ProtocolPacketRoundTripTest, DescriptionRequest_RoundTripPreservesEndpoint) {
    CDescriptionRequest request("192.168.1.55", 6000);
    std::vector<unsigned char> raw = Serialize(request);

    unsigned char* ptr = raw.data();
    CDescriptionRequest parsed(ptr);

    EXPECT_STREQ("192.168.1.55", parsed.GetAddress().GetBuffer());
    EXPECT_EQ(6000, parsed.GetPort());
}

TEST_F(ProtocolPacketRoundTripTest, ConnectionStateRequest_RoundTripProducesStableBytes) {
    CConnectionStateRequest request(9, 5000, "10.0.0.7");
    std::vector<unsigned char> first = Serialize(request);

    unsigned char* ptr = first.data();
    CConnectionStateRequest parsed(ptr);
    std::vector<unsigned char> second = Serialize(parsed);

    ASSERT_EQ(first.size(), second.size());
    EXPECT_EQ(0, std::memcmp(first.data(), second.data(), first.size()));
    EXPECT_EQ(9, parsed.GetChannelId());
}

TEST_F(ProtocolPacketRoundTripTest, ConnectionStateResponse_RoundTripPreservesFields) {
    CConnectionStateResponse response(3, E_NO_ERROR);
    std::vector<unsigned char> raw = Serialize(response);

    unsigned char* ptr = raw.data();
    CConnectionStateResponse parsed(ptr);

    EXPECT_EQ(3, parsed.GetChannelID());
    EXPECT_EQ(E_NO_ERROR, parsed.GetStatus());
    EXPECT_STREQ("Connection OK", parsed.GetStatusString().GetBuffer());
}

TEST_F(ProtocolPacketRoundTripTest, DisconnectRequest_RoundTripPreservesFields) {
    CDisconnectRequest request(4, 7777, "192.168.33.4");
    std::vector<unsigned char> raw = Serialize(request);

    unsigned char* ptr = raw.data();
    CDisconnectRequest parsed(ptr);

    EXPECT_EQ(4, parsed.GetChannelID());
    EXPECT_STREQ("192.168.33.4", parsed.GetControlAddr().GetBuffer());
    EXPECT_EQ(7777, parsed.GetControlPort());
}

TEST_F(ProtocolPacketRoundTripTest, DisconnectResponse_RoundTripPreservesFields) {
    CDisconnectResponse response(6, E_NO_ERROR);
    std::vector<unsigned char> raw = Serialize(response);

    unsigned char* ptr = raw.data();
    CDisconnectResponse parsed(ptr);

    EXPECT_EQ(6, parsed.GetChannelID());
    EXPECT_EQ(E_NO_ERROR, parsed.GetStatus());
}

TEST_F(ProtocolPacketRoundTripTest, TunnelingAck_RoundTripPreservesStatusAndSequence) {
    CTunnelingAck ack(7, 8, E_NO_ERROR);
    std::vector<unsigned char> raw = Serialize(ack);

    unsigned char* ptr = raw.data();
    CTunnelingAck parsed(ptr);

    EXPECT_EQ(7, parsed.GetChannelId());
    EXPECT_EQ(8, parsed.GetSequenceNumber());
    EXPECT_EQ(E_NO_ERROR, parsed.GetStatus());
    EXPECT_STREQ("No error", parsed.GetStatusString().GetBuffer());
}
