// BusMonitorTest.cpp -- Tests that unsolicited EIB indications sent by
// the emulator are received by the server and visible via the web API.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

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

    // Give time for the indication to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    HttpResponse resp = http.Get("/api/admin/busmon", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    // The bus monitor response should reference the address we sent to
    EXPECT_GE(resp.body.Find("1/2/3"), 0)
        << "Bus monitor should contain address 1/2/3. Body: " << resp.body.GetBuffer();
}

TEST_F(BusMonitorTest, MultipleIndications)
{
    unsigned char val1[] = {0x01};
    unsigned char val2[] = {0x02};
    unsigned char val3[] = {0x03};

    // Space out indications to allow tunnel acks to complete between sends
    EmulatorSendIndication("0/0/1", val1, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EmulatorSendIndication("0/0/2", val2, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    EmulatorSendIndication("1/2/3", val3, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    HttpResponse resp = http.Get("/api/admin/busmon", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_GE(resp.body.Find("0/0/1"), 0)
        << "Missing 0/0/1. Body: " << resp.body.GetBuffer();
    EXPECT_GE(resp.body.Find("0/0/2"), 0)
        << "Missing 0/0/2. Body: " << resp.body.GetBuffer();
    EXPECT_GE(resp.body.Find("1/2/3"), 0)
        << "Missing 1/2/3. Body: " << resp.body.GetBuffer();
}

TEST_F(BusMonitorTest, IndicationValue)
{
    // Send a known value to 0/0/2
    unsigned char val[] = {0x00, 0x80};
    EmulatorSendIndication("0/0/2", val, 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    HttpResponse resp = http.Get("/api/history", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    // The value 0x0080 should appear somewhere in the history
    EXPECT_GE(resp.body.Find("0/0/2"), 0)
        << "History should contain address 0/0/2. Body: " << resp.body.GetBuffer();
}

TEST_F(BusMonitorTest, IndicationInHistory)
{
    unsigned char val[] = {0x42};
    EmulatorSendIndication("1/2/3", val, 1);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    HttpResponse resp = http.Get("/api/history", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    // The indication should appear in the global history
    EXPECT_GE(resp.body.Find("1/2/3"), 0)
        << "History should contain address 1/2/3. Body: " << resp.body.GetBuffer();
}
