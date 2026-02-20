#include <gtest/gtest.h>
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdynamic-exception-spec"
#endif
#include "Socket.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#include "Utils.h"
#include "../fixtures/TestHelpers.h"
#include <algorithm>
#include <cstring>
#include <map>

// GTEST_SKIP was added in Google Test 1.10. Provide a fallback for older versions.
#ifndef GTEST_SKIP
#define GTEST_SKIP() GTEST_LOG_(INFO) << "Skipped: "
#endif

using namespace EIBStdLibTest;

/**
 * These tests reproduce the multicast receive issue observed when running
 * EIBServer + Emulator-ng on the same macOS host.
 *
 * The emulator binds its UDP socket to a specific interface IP (e.g. 192.168.x.x)
 * and joins a multicast group.  On macOS, a socket bound to a unicast address
 * will NOT receive multicast traffic even if it has joined the group.
 * The socket must be bound to INADDR_ANY (0.0.0.0) to receive multicast.
 *
 * Test 1 (BoundToSpecificIP_CannotReceiveMulticast):
 *   Mimics the current emulator code — expected to FAIL to receive (timeout).
 *
 * Test 2 (BoundToAny_CanReceiveMulticast):
 *   Mimics the proposed fix — expected to SUCCEED.
 */

namespace {

// Use a link-local multicast address in the "administratively scoped" range
// to avoid interfering with real KNX traffic.
static const char* TEST_MCAST_GROUP = "239.255.77.1";

std::string ToLowerStr(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool IsEnvironmentRestricted(const SocketException& ex) {
    const std::string msg = ToLowerStr(ex.what());
    return msg.find("operation not permitted") != std::string::npos ||
           msg.find("permission denied") != std::string::npos ||
           msg.find("not supported") != std::string::npos ||
           msg.find("cannot assign requested address") != std::string::npos ||
           msg.find("network is unreachable") != std::string::npos;
}

// Get the first non-loopback interface IP via CUtils::EnumNics
CString GetLocalInterfaceAddress() {
    std::map<CString,CString> nics;
    if (!CUtils::EnumNics(nics) || nics.empty()) {
        return CString();
    }
    // Return the first NIC name (key) — we need its IP.
    // The EnumNics value contains "name (ip)", extract the IP.
    for (auto it = nics.begin(); it != nics.end(); ++it) {
        const CString& desc = it->second;
        // Format is "en0 (192.168.x.x)"
        std::string s = desc.GetBuffer();
        auto open = s.find('(');
        auto close = s.find(')');
        if (open != std::string::npos && close != std::string::npos && close > open + 1) {
            std::string ip = s.substr(open + 1, close - open - 1);
            if (ip != "127.0.0.1" && ip.find(':') == std::string::npos) {
                return CString(ip.c_str());
            }
        }
    }
    return CString();
}

} // namespace

class MulticastBindTest : public BaseTestFixture {};

/**
 * Current emulator behavior: bind to specific IP, then JoinGroup.
 * On macOS this socket will NOT receive multicast datagrams.
 */
TEST_F(MulticastBindTest, BoundToSpecificIP_CannotReceiveMulticast) {
    CString local_ip = GetLocalInterfaceAddress();
    if (local_ip.IsEmpty()) {
        GTEST_SKIP() << "No non-loopback network interface available.";
    }

    int recv_port = 0;
    try {
        // Receiver: bind to specific IP on an ephemeral port, then join multicast group
        UDPSocket receiver(local_ip, 0);
        recv_port = receiver.GetLocalPort();
        ASSERT_GT(recv_port, 0);
        receiver.JoinGroup(local_ip, TEST_MCAST_GROUP);

        // Sender: set outgoing interface and send to the multicast group
        UDPSocket sender;
        sender.SetMulticastInterface(local_ip);
        sender.SetMulticastLoopBack(true);
        const char payload[] = "mcast-test";
        sender.SendTo(payload, static_cast<int>(sizeof(payload) - 1),
                      TEST_MCAST_GROUP, recv_port);

        // Try to receive — on macOS this should time out because the socket
        // is bound to a unicast address, not INADDR_ANY.
        char recv_buf[64] = {0};
        CString source_address;
        int source_port = 0;
        int got = receiver.RecvFrom(recv_buf, sizeof(recv_buf),
                                     source_address, source_port, 2000);

        // We EXPECT this to be 0 (timeout) on macOS, proving the bug.
        EXPECT_EQ(0, got) << "Socket bound to specific IP should NOT receive multicast on macOS";

    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Multicast not available: " << ex.what();
        }
        throw;
    }
}

/**
 * Proposed fix: bind to INADDR_ANY (port only), then JoinGroup on the interface.
 * This socket SHOULD receive multicast datagrams.
 */
TEST_F(MulticastBindTest, BoundToAny_CanReceiveMulticast) {
    CString local_ip = GetLocalInterfaceAddress();
    if (local_ip.IsEmpty()) {
        GTEST_SKIP() << "No non-loopback network interface available.";
    }

    int recv_port = 0;
    try {
        // Receiver: bind to INADDR_ANY on ephemeral port, then join multicast group
        UDPSocket receiver(0);  // binds to 0.0.0.0:ephemeral
        recv_port = receiver.GetLocalPort();
        ASSERT_GT(recv_port, 0);
        receiver.JoinGroup(local_ip, TEST_MCAST_GROUP);

        // Sender: set outgoing interface and enable loopback, then send
        UDPSocket sender;
        sender.SetMulticastInterface(local_ip);
        sender.SetMulticastLoopBack(true);
        const char payload[] = "mcast-test";
        sender.SendTo(payload, static_cast<int>(sizeof(payload) - 1),
                      TEST_MCAST_GROUP, recv_port);

        // Try to receive — this should succeed.
        char recv_buf[64] = {0};
        CString source_address;
        int source_port = 0;
        int got = receiver.RecvFrom(recv_buf, sizeof(recv_buf),
                                     source_address, source_port, 2000);

        ASSERT_GT(got, 0) << "Socket bound to INADDR_ANY should receive multicast";
        EXPECT_EQ(static_cast<int>(sizeof(payload) - 1), got);
        EXPECT_EQ(0, std::memcmp(payload, recv_buf, sizeof(payload) - 1));

    } catch (const SocketException& ex) {
        if (IsEnvironmentRestricted(ex)) {
            GTEST_SKIP() << "Multicast not available: " << ex.what();
        }
        throw;
    }
}
