#include <gtest/gtest.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdynamic-exception-spec"
#endif
#include "Socket.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include "../fixtures/TestHelpers.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <dirent.h>
#include <memory>
#include <vector>

using namespace EIBStdLibTest;

class SocketNetworkTest : public BaseTestFixture {};

namespace {
std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool IsEnvironmentRestricted(const SocketException& ex) {
    const std::string msg = ToLower(ex.what());
    return msg.find("operation not permitted") != std::string::npos ||
           msg.find("permission denied") != std::string::npos ||
           msg.find("not supported") != std::string::npos ||
           msg.find("cannot assign requested address") != std::string::npos ||
           msg.find("network is unreachable") != std::string::npos;
}

bool IsPermissionRestricted(const SocketException& ex) {
    std::string msg = ex.what();
    std::transform(msg.begin(), msg.end(), msg.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return msg.find("operation not permitted") != std::string::npos ||
           msg.find("permission denied") != std::string::npos;
}

int CountOpenFDs() {
    DIR* dir = opendir("/dev/fd");
    if (dir == NULL) {
        return -1;
    }
    int count = 0;
    while (readdir(dir) != NULL) {
        ++count;
    }
    closedir(dir);
    return count;
}
}  // namespace

TEST_F(SocketNetworkTest, TcpLoopback_SendAndReceiveBidirectional) {
    try {
        TCPServerSocket server(0);
        int server_port = server.GetLocalPort();
        ASSERT_GT(server_port, 0);

        TCPSocket client("127.0.0.1", static_cast<unsigned short>(server_port));

        std::unique_ptr<TCPSocket> accepted(server.Accept(1000));
        ASSERT_NE(nullptr, accepted);

        const char request[] = "ping";
        client.Send(request, 4);

        char server_buf[16] = {0};
        int server_read = accepted->Recv(server_buf, sizeof(server_buf), 1000);
        ASSERT_EQ(4, server_read);
        EXPECT_EQ(0, std::memcmp(request, server_buf, 4));

        const char reply[] = "ok";
        accepted->Send(reply, 2);

        char client_buf[16] = {0};
        int client_read = client.Recv(client_buf, sizeof(client_buf), 1000);
        ASSERT_EQ(2, client_read);
        EXPECT_EQ(0, std::memcmp(reply, client_buf, 2));

        EXPECT_EQ(server_port, client.GetForeignPort());
        EXPECT_STREQ("127.0.0.1", client.GetForeignAddress().GetBuffer());
    } catch (const SocketException& ex) {
        if (IsPermissionRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, TcpServerAccept_TimesOutWithoutIncomingConnection) {
    try {
        TCPServerSocket server(0);
        std::unique_ptr<TCPSocket> accepted(server.Accept(20));
        EXPECT_EQ(nullptr, accepted);
    } catch (const SocketException& ex) {
        if (IsPermissionRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, UdpLoopback_SendToAndRecvFrom) {
    try {
        UDPSocket receiver("127.0.0.1", 0);
        int receiver_port = receiver.GetLocalPort();
        ASSERT_GT(receiver_port, 0);

        UDPSocket sender;

        const char payload[] = "hello";
        sender.SendTo(payload, 5, "127.0.0.1", receiver_port);

        char recv_buf[32] = {0};
        CString source_address;
        int source_port = 0;

        int got = receiver.RecvFrom(recv_buf, sizeof(recv_buf), source_address, source_port, 1000);
        ASSERT_EQ(5, got);
        EXPECT_EQ(0, std::memcmp(payload, recv_buf, 5));
        EXPECT_FALSE(source_address.IsEmpty());
        EXPECT_GT(source_port, 0);
    } catch (const SocketException& ex) {
        if (IsPermissionRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, UdpRecvFrom_TimeoutReturnsZero) {
    try {
        UDPSocket receiver("127.0.0.1", 0);

        char recv_buf[8] = {0};
        CString source_address;
        int source_port = 0;

        int got = receiver.RecvFrom(recv_buf, sizeof(recv_buf), source_address, source_port, 20);
        EXPECT_EQ(0, got);
    } catch (const SocketException& ex) {
        if (IsPermissionRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, ResolveService_NumericStringReturnsPortValue) {
    EXPECT_EQ(3671, Socket::ResolveService("3671"));
}

TEST_F(SocketNetworkTest, ResolveService_ByNameAndUnknownString) {
    EXPECT_GT(Socket::ResolveService("http"), 0);
    EXPECT_EQ(0, Socket::ResolveService("not-a-real-service-name"));
}

TEST_F(SocketNetworkTest, TcpConnectToClosedPort_Throws) {
    int closed_port = 0;
    try {
        TCPServerSocket server(0);
        closed_port = server.GetLocalPort();
    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }

    EXPECT_GT(closed_port, 0);
    EXPECT_THROW(
        {
            TCPSocket client("127.0.0.1", static_cast<unsigned short>(closed_port));
            (void)client;
        },
        SocketException);
}

TEST_F(SocketNetworkTest, TcpRecvReturnsZeroWhenPeerClosed) {
    try {
        TCPServerSocket server(0);
        int server_port = server.GetLocalPort();
        TCPSocket client("127.0.0.1", static_cast<unsigned short>(server_port));

        std::unique_ptr<TCPSocket> accepted(server.Accept(1000));
        ASSERT_NE(nullptr, accepted);
        accepted.reset();

        char buf[8] = {0};
        int read = client.Recv(buf, sizeof(buf), 1000);
        EXPECT_EQ(0, read);
    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, UdpSendToTimeoutOverload_DeliversDatagram) {
    try {
        UDPSocket receiver("127.0.0.1", 0);
        int receiver_port = receiver.GetLocalPort();
        UDPSocket sender;

        const char payload[] = "with-timeout";
        sender.SendTo(payload, static_cast<int>(sizeof(payload) - 1), "127.0.0.1", receiver_port, 1000);

        char recv_buf[32] = {0};
        CString source_address;
        int source_port = 0;
        int got = receiver.RecvFrom(recv_buf, sizeof(recv_buf), source_address, source_port, 1000);
        ASSERT_EQ(static_cast<int>(sizeof(payload) - 1), got);
        EXPECT_EQ(0, std::memcmp(payload, recv_buf, sizeof(payload) - 1));
    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, UdpDisconnect_AllowsSubsequentSendTo) {
    try {
        UDPSocket receiver("127.0.0.1", 0);
        int receiver_port = receiver.GetLocalPort();

        UDPSocket sender;
        sender.Connect("127.0.0.1", static_cast<unsigned short>(receiver_port));
        sender.Disconnect();

        const char payload[] = "after-disconnect";
        sender.SendTo(payload, static_cast<int>(sizeof(payload) - 1), "127.0.0.1", receiver_port);

        char recv_buf[64] = {0};
        CString source_address;
        int source_port = 0;
        int got = receiver.RecvFrom(recv_buf, sizeof(recv_buf), source_address, source_port, 1000);
        ASSERT_EQ(static_cast<int>(sizeof(payload) - 1), got);
        EXPECT_EQ(0, std::memcmp(payload, recv_buf, sizeof(payload) - 1));
    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, UdpMulticastOptions_AndJoinLeave) {
    try {
        UDPSocket socket("0.0.0.0", 0);
        EXPECT_NO_THROW(socket.SetMulticastTTL(2));
        EXPECT_NO_THROW(socket.SetMulticastLoopBack(true));
        EXPECT_NO_THROW(socket.JoinGroup("0.0.0.0", "239.255.0.1"));
        EXPECT_NO_THROW(socket.LeaveGroup("239.255.0.1"));
    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Network multicast not available in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, SetNonBlocking_MakesImmediateRecvFailWithoutData) {
    try {
        UDPSocket receiver("127.0.0.1", 0);
        receiver.SetNonBlocking();

        char recv_buf[8] = {0};
        CString source_address;
        int source_port = 0;
        EXPECT_THROW(receiver.RecvFrom(recv_buf, sizeof(recv_buf), source_address, source_port), SocketException);
    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}

TEST_F(SocketNetworkTest, LocalAddress_InvalidInterfaceDoesNotLeakFileDescriptors) {
    int before = CountOpenFDs();
    if (before < 0) {
        GTEST_SKIP() << "Cannot count open file descriptors on this environment.";
    }

    for (int i = 0; i < 64; ++i) {
        try {
            (void)Socket::LocalAddress("definitely-not-a-valid-interface");
            FAIL() << "Expected Socket::LocalAddress to throw for invalid interface name";
        } catch (const SocketException& ex) {
            if (IsEnvironmentRestricted(ex)) {
                GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
            }
        }
    }

    int after = CountOpenFDs();
    ASSERT_GE(after, 0);
    EXPECT_LE(after - before, 4);
}

TEST_F(SocketNetworkTest, LocalAddress_ReturnsForKnownLoopbackInterface) {
    const std::vector<const char*> candidates = {"lo0", "lo", "loopback"};

    for (size_t i = 0; i < candidates.size(); ++i) {
        try {
            CString addr = Socket::LocalAddress(candidates[i]);
            if (!addr.IsEmpty()) {
                return;
            }
        } catch (const SocketException& ex) {
            if (IsEnvironmentRestricted(ex)) {
                GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
            }
        }
    }

    GTEST_SKIP() << "No known loopback interface alias matched on this host.";
}

TEST_F(SocketNetworkTest, TcpServerSocket_ReuseAddrAllowsImmediateRebind) {
    // Verify that SO_REUSEADDR is set on TCPServerSocket so that a port
    // in TIME_WAIT from a previous process can be rebound immediately.
    try {
        int port = 0;
        {
            TCPServerSocket first(0);
            port = first.GetLocalPort();
            ASSERT_GT(port, 0);

            // Connect a client so the port enters TIME_WAIT on close
            TCPSocket client("127.0.0.1", static_cast<unsigned short>(port));
            std::unique_ptr<TCPSocket> accepted(first.Accept(1000));
            ASSERT_NE(nullptr, accepted);
        }
        // first is now closed -- port may be in TIME_WAIT.
        // Without SO_REUSEADDR the following bind would fail.
        EXPECT_NO_THROW({
            TCPServerSocket second(port);
            EXPECT_EQ(port, second.GetLocalPort());
        });
    } catch (const SocketException& ex) {
        if (IsPermissionRestricted(ex)) {
            GTEST_SKIP() << "Socket operations restricted in this environment: " << ex.what();
        }
        throw;
    }
}
