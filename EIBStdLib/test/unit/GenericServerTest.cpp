#include <gtest/gtest.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdynamic-exception-spec"
#endif
#include "GenericServer.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include "CCemi_L_Data_Frame.h"
#include "EIBAddress.h"
#include "EibNetwork.h"
#include "../fixtures/TestHelpers.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <memory>

using namespace EIBStdLibTest;
using namespace EibStack;

namespace {
bool IsPermissionRestricted(const SocketException& ex) {
    std::string msg = ex.what();
    std::transform(msg.begin(), msg.end(), msg.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return msg.find("operation not permitted") != std::string::npos ||
           msg.find("permission denied") != std::string::npos;
}

void EnsureJtcRuntime() {
    // CGenericServer owns a JTC thread handle; JTC must be initialized first.
    static JTCInitialize* init = new JTCInitialize();
    (void)init;
}
}  // namespace

class GenericServerTest : public BaseTestFixture {
protected:
    std::unique_ptr<CGenericServer> server;
    CLogFile log;
    bool restricted;

    GenericServerTest() : restricted(false) {}

    void SetUp() override {
        BaseTestFixture::SetUp();
        try {
            // Probe socket capability first so we can skip safely on restricted hosts.
            UDPSocket probe(0);
            (void)probe.GetLocalPort();

            EnsureJtcRuntime();
            server.reset(new CGenericServer(0x2A));
            log.SetPrinterMethod(printf);
            log.SetPrompt(false);
            server->Init(&log);
        } catch (const SocketException& ex) {
            if (IsPermissionRestricted(ex)) {
                restricted = true;
                return;
            }
            throw;
        }
    }

    bool HasServer() const {
        return !restricted && server != nullptr;
    }
};

TEST_F(GenericServerTest, BasicPropertiesAndStatusTransitions) {
    if (!HasServer()) {
        return;
    }

    EXPECT_EQ(0x2A, server->GetNetworkID());
    EXPECT_EQ(0, server->GetSessionID());
    EXPECT_TRUE(server->GetSharedKey().IsEmpty());
    EXPECT_EQ(&log, server->GetLog());
    EXPECT_TRUE(server->GetUserName().IsEmpty());

    server->SetStatus(STATUS_CONNECTED);
    EXPECT_TRUE(server->IsConnected());

    server->SetStatus(STATUS_DISCONNECTED);
    EXPECT_FALSE(server->IsConnected());
}

TEST_F(GenericServerTest, OpenConnectionAlreadyConnected_CharAndCStringOverloads) {
    if (!HasServer()) {
        return;
    }
    server->SetStatus(STATUS_CONNECTED);

    EXPECT_EQ(
        STATUS_ALREADY_CONNECTED,
        server->OpenConnection("net", "127.0.0.1", 1234, "key", "127.0.0.1", "u", "p"));

    EXPECT_EQ(
        STATUS_ALREADY_CONNECTED,
        server->OpenConnection(
            CString("net"), CString("127.0.0.1"), 1234, CString("key"), CString("127.0.0.1"),
            CString("u"), CString("p")));

    EXPECT_TRUE(server->IsConnected());
}

TEST_F(GenericServerTest, DiscoverWhileConnected_ReturnsFalse) {
    if (!HasServer()) {
        return;
    }
    server->SetStatus(STATUS_CONNECTED);

    CString discovered_ip;
    discovered_ip = "keep";
    int discovered_port = 77;
    EXPECT_FALSE(server->DiscoverEIBServer("127.0.0.1", "init-key", discovered_ip, discovered_port));
    EXPECT_STREQ("keep", discovered_ip.GetBuffer());
    EXPECT_EQ(77, discovered_port);
}

TEST_F(GenericServerTest, SendWhenDisconnected_ReturnsZeroForBothOverloads) {
    if (!HasServer()) {
        return;
    }
    server->SetStatus(STATUS_DISCONNECTED);

    unsigned char value[] = {0x01, 0x02};
    EXPECT_EQ(0, server->SendEIBNetwork(CEibAddress("1/2/3"), value, 2, NON_BLOCKING));

    unsigned char payload[] = {GROUP_WRITE, 0x33};
    CCemi_L_Data_Frame frame(
        L_DATA_REQ, CEibAddress("1.1.1"), CEibAddress("1/2/3"), payload, 2);
    EXPECT_EQ(0, server->SendEIBNetwork(frame, NON_BLOCKING));
}

TEST_F(GenericServerTest, ReceiveWhenDisconnected_ReturnsZeroForBothOverloads) {
    if (!HasServer()) {
        return;
    }
    server->SetStatus(STATUS_DISCONNECTED);

    CEibAddress function("1/2/3");
    unsigned char value[16];
    memset(value, 0xAA, sizeof(value));
    unsigned char value_len = 7;
    EXPECT_EQ(0, server->ReceiveEIBNetwork(function, value, value_len, 10));
    EXPECT_EQ(7, value_len);
    for (size_t i = 0; i < sizeof(value); ++i) {
        EXPECT_EQ(0xAA, value[i]);
    }

    CCemi_L_Data_Frame frame;
    EXPECT_EQ(0, server->ReceiveEIBNetwork(frame, 10));
}

TEST_F(GenericServerTest, CloseWhenDisconnected_DoesNotThrow) {
    if (!HasServer()) {
        return;
    }
    server->SetStatus(STATUS_DISCONNECTED);
    EXPECT_NO_THROW(server->Close());
}

TEST_F(GenericServerTest, CloseWhenConnectedWithoutHandshake_Throws) {
    if (!HasServer()) {
        return;
    }
    // Artificially forcing CONNECTED without handshake leaves shared key empty.
    // Close() encrypts disconnect payload and must fail this invalid state.
    server->SetStatus(STATUS_CONNECTED);
    EXPECT_THROW(server->Close(), CEIBException);
}
