// BusMonitorTest.cpp -- Tests that unsolicited EIB indications sent by
// the emulator are received by the server and visible via the web API.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

// Poll an HTTP endpoint until the response body contains the expected string.
static bool WaitForBodyContains(HttpTestClient& http, const CString& uri,
                                const CString& session, const CString& needle,
                                int max_ms = 2000) {
    for (int elapsed = 0; elapsed < max_ms; elapsed += 20) {
        HttpResponse r = http.Get(uri, session);
        if (r.status_code == 200 && r.body.Find(needle) != string::npos)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

class BusMonitorTest : public ::testing::Test {
protected:
    HttpTestClient http;
    CString admin_sid;

    void SetUp() override {
        admin_sid = http.Login("admin", "admin123");
        ASSERT_GT(admin_sid.GetLength(), 0) << "Admin login failed";
    }
};

TEST_F(BusMonitorTest, UnsolicitedIndicationRecorded)
{
    unsigned char val[] = {0x01};
    EmulatorSendIndication("1/2/3", val, 1);

    EXPECT_TRUE(WaitForBodyContains(http, "/api/admin/busmon", admin_sid, "1/2/3"))
        << "Bus monitor should contain address 1/2/3";
}

TEST_F(BusMonitorTest, MultipleIndications)
{
    unsigned char val1[] = {0x01};
    unsigned char val2[] = {0x02};
    unsigned char val3[] = {0x03};

    // Delay between sends for tunnel ACK round-trip
    EmulatorSendIndication("0/0/1", val1, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EmulatorSendIndication("0/0/2", val2, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EmulatorSendIndication("1/2/3", val3, 1);

    EXPECT_TRUE(WaitForBodyContains(http, "/api/admin/busmon", admin_sid, "0/0/1"))
        << "Missing 0/0/1";
    EXPECT_TRUE(WaitForBodyContains(http, "/api/admin/busmon", admin_sid, "0/0/2"))
        << "Missing 0/0/2";
    EXPECT_TRUE(WaitForBodyContains(http, "/api/admin/busmon", admin_sid, "1/2/3"))
        << "Missing 1/2/3";
}

TEST_F(BusMonitorTest, IndicationValue)
{
    unsigned char val[] = {0x00, 0x80};
    EmulatorSendIndication("0/0/2", val, 2);

    EXPECT_TRUE(WaitForBodyContains(http, "/api/history", admin_sid, "0/0/2"))
        << "History should contain address 0/0/2";
}

TEST_F(BusMonitorTest, IndicationInHistory)
{
    unsigned char val[] = {0x42};
    EmulatorSendIndication("1/2/3", val, 1);

    EXPECT_TRUE(WaitForBodyContains(http, "/api/history", admin_sid, "1/2/3"))
        << "History should contain address 1/2/3";
}
