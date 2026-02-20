// EibCommunicationTest.cpp -- EIB interface and communication tests.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class EibCommunicationTest : public ::testing::Test {
protected:
    HttpTestClient http;
    CString admin_sid;

    void SetUp() override {
        admin_sid = http.Login("admin", "admin123");
        ASSERT_GT(admin_sid.GetLength(), 0) << "Admin login failed";
    }
};

TEST_F(EibCommunicationTest, InterfaceModeIsTunneling)
{
    CString mode = CEIBServer::GetInstance().GetEIBInterface().GetModeString();
    EXPECT_EQ(mode, CString("MODE_TUNNELING"));
}

TEST_F(EibCommunicationTest, SendCommandUpdatesStats)
{
    const EIBInterfaceStats& stats_before =
        CEIBServer::GetInstance().GetEIBInterface().GetInterfaceStats();
    int sent_before = stats_before._total_sent;

    // Send a command via the web API
    CString body = "{\"address\":\"0/0/1\",\"value\":\"0x01\"}";
    HttpResponse resp = http.Post("/api/eib/send", body, admin_sid);
    EXPECT_EQ(resp.status_code, 200);

    // Give the output handler time to process
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    const EIBInterfaceStats& stats_after =
        CEIBServer::GetInstance().GetEIBInterface().GetInterfaceStats();
    EXPECT_GT(stats_after._total_sent, sent_before);
}

TEST_F(EibCommunicationTest, MultipleCommands)
{
    const EIBInterfaceStats& stats_before =
        CEIBServer::GetInstance().GetEIBInterface().GetInterfaceStats();
    int sent_before = stats_before._total_sent;

    // Send 3 commands
    for (int i = 0; i < 3; ++i) {
        CString body = "{\"address\":\"0/0/1\",\"value\":\"0x01\"}";
        HttpResponse resp = http.Post("/api/eib/send", body, admin_sid);
        EXPECT_EQ(resp.status_code, 200);
        // Small delay between commands
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // Give the output handler time to process all
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    const EIBInterfaceStats& stats_after =
        CEIBServer::GetInstance().GetEIBInterface().GetInterfaceStats();
    EXPECT_GE(stats_after._total_sent, sent_before + 3);
}
