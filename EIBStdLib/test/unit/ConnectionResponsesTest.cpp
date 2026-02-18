#include "ConnectResponse.h"
#include "DisconnectResponse.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <vector>
#include <string>

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

class ConnectionResponsesTest : public BaseTestFixture {};

TEST_F(ConnectionResponsesTest, ConnectResponse_ConstructorRejectsOutOfRangeFields) {
    EXPECT_THROW(
        CConnectResponse(-1, E_NO_ERROR, "10.0.0.1", 3671, CConnectRequest::TunnelConnection),
        CEIBException);
    EXPECT_THROW(
        CConnectResponse(256, E_NO_ERROR, "10.0.0.1", 3671, CConnectRequest::TunnelConnection),
        CEIBException);
    EXPECT_THROW(
        CConnectResponse(1, -1, "10.0.0.1", 3671, CConnectRequest::TunnelConnection),
        CEIBException);
    EXPECT_THROW(
        CConnectResponse(1, 256, "10.0.0.1", 3671, CConnectRequest::TunnelConnection),
        CEIBException);
}

TEST_F(ConnectionResponsesTest, ConnectResponse_RoundTripPreservesSuccessFields) {
    CConnectResponse response(
        7, E_NO_ERROR, "172.16.0.8", 5000, CConnectRequest::TunnelConnection);
    std::vector<unsigned char> raw = Serialize(response);

    unsigned char* ptr = raw.data();
    CConnectResponse parsed(ptr);

    EXPECT_EQ(7, parsed.GetChannelID());
    EXPECT_EQ(E_NO_ERROR, parsed.GetStatus());
    EXPECT_STREQ("172.16.0.8", parsed.GetDataIPAddress().GetBuffer());
    EXPECT_EQ(5000, parsed.GetDataPort());
    EXPECT_EQ(0x0200, parsed.GetDevKNXAddress().ToByteArray());
}

TEST_F(ConnectionResponsesTest, ConnectResponse_ParseErrorStatusThrowsMappedMessage) {
    CConnectResponse ok(
        3, E_NO_ERROR, "192.168.10.20", 4000, CConnectRequest::TunnelConnection);
    std::vector<unsigned char> raw = Serialize(ok);

    raw[7] = E_CONNECTION_OPTION;  // status byte in payload
    unsigned char* ptr = raw.data();
    try {
        CConnectResponse parsed(ptr);
        FAIL() << "Expected CEIBException";
    } catch (const CEIBException& ex) {
        std::string msg(ex.what());
        EXPECT_NE(std::string::npos, msg.find("Connection option not supported"));
    }
}

TEST_F(ConnectionResponsesTest, ConnectResponse_StatusStringUnknownForUnexpectedCode) {
    CConnectResponse response(
        1, 0x99, "10.1.1.1", 1234, CConnectRequest::TunnelConnection);
    EXPECT_STREQ("unknown status", response.GetStatusString().GetBuffer());
}

TEST_F(ConnectionResponsesTest, DisconnectResponse_RoundTripAndStatusText) {
    CDisconnectResponse response(9, E_NO_ERROR);
    std::vector<unsigned char> raw = Serialize(response);

    unsigned char* ptr = raw.data();
    CDisconnectResponse parsed(ptr);

    EXPECT_EQ(9, parsed.GetChannelID());
    EXPECT_EQ(E_NO_ERROR, parsed.GetStatus());
    EXPECT_STREQ("Connection OK", parsed.GetStatusString().GetBuffer());
}

TEST_F(ConnectionResponsesTest, DisconnectResponse_StatusTextForKnownAndUnknownErrors) {
    CDisconnectResponse wrong_channel(5, E_CONNECTION_ID);
    EXPECT_STREQ("Wrong channel id", wrong_channel.GetStatusString().GetBuffer());

    CDisconnectResponse unknown(5, 0xAA);
    EXPECT_STREQ("unknown status", unknown.GetStatusString().GetBuffer());
}
