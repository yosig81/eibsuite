// ServerLifecycleTest.cpp -- Basic server startup and configuration tests.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class ServerLifecycleTest : public ::testing::Test {
protected:
    HttpTestClient http;
};

TEST_F(ServerLifecycleTest, ServerIsRunning)
{
    EXPECT_TRUE(CEIBServer::GetInstance().GetConfig().GetLoadOK());
}

TEST_F(ServerLifecycleTest, ConfigValuesCorrect)
{
    CServerConfig& conf = CEIBServer::GetInstance().GetConfig();
    EXPECT_EQ(conf.GetListeningPort(), 15000);
    EXPECT_EQ(conf.GetWEBServerPort(), 18080);
    EXPECT_EQ(conf.GetEibDeviceAddress(), CString("127.0.0.1"));
}

TEST_F(ServerLifecycleTest, UsersDBLoaded)
{
    EXPECT_EQ(CEIBServer::GetInstance().GetUsersDB().GetNumOfUsers(), 6);
}

TEST_F(ServerLifecycleTest, EibInterfaceConnected)
{
    EIB_DEVICE_MODE mode = CEIBServer::GetInstance().GetEIBInterface().GetMode();
    EXPECT_NE(mode, UNDEFINED_MODE);
}

TEST_F(ServerLifecycleTest, WebServerListening)
{
    // A simple TCP connect to port 18080 should succeed
    try {
        TCPSocket sock("127.0.0.1", 18080);
        SUCCEED();
    } catch (SocketException& e) {
        FAIL() << "Cannot connect to web server: " << e.what();
    }
}

TEST_F(ServerLifecycleTest, ServeIndexHtml)
{
    HttpResponse resp = http.Get("/");
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_GE(resp.body.Find("EIBServer Test"), 0)
        << "Body: " << resp.body.GetBuffer();
}
