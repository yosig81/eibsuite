// WebApiDataTest.cpp -- Data API endpoint tests (send, history, schedule).

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class WebApiDataTest : public ::testing::Test {
protected:
    HttpTestClient http;
    CString admin_sid;

    void SetUp() override {
        admin_sid = http.Login("admin", "admin123");
        ASSERT_GT(admin_sid.GetLength(), 0) << "Admin login failed";
    }
};

TEST_F(WebApiDataTest, SendEibCommand)
{
    CString body = "{\"address\":\"0/0/1\",\"value\":\"0x01\"}";
    HttpResponse resp = http.Post("/api/eib/send", body, admin_sid);
    EXPECT_EQ(resp.status_code, 200)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiDataTest, SendEibCommandReadOnly)
{
    // readonly user has privileges=5 (READ + WEB, no WRITE)
    CString ro_sid = http.Login("readonly", "readonly123");
    ASSERT_GT(ro_sid.GetLength(), 0) << "Readonly login failed";

    CString body = "{\"address\":\"0/0/1\",\"value\":\"0x01\"}";
    HttpResponse resp = http.Post("/api/eib/send", body, ro_sid);
    EXPECT_EQ(resp.status_code, 403);
}

TEST_F(WebApiDataTest, GetHistory)
{
    HttpResponse resp = http.Get("/api/history", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    // Should return a JSON object with "records" array
    EXPECT_GE(resp.body.Find("\"records\""), 0)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiDataTest, ProtectedEndpointNoAuth)
{
    // GET /api/history without authentication should be rejected
    HttpResponse resp = http.Get("/api/history");
    EXPECT_EQ(resp.status_code, 401);
}

TEST_F(WebApiDataTest, ScheduleCommand)
{
    // Schedule a command for a future time (use a date well in the future)
    CString body("{\"address\":\"0/0/1\",\"value\":\"0x01\",\"datetime\":\"Wed Jan 01 00:00:00 2031\"}");
    HttpResponse resp = http.Post("/api/eib/schedule", body, admin_sid);
    EXPECT_EQ(resp.status_code, 200)
        << "Body: " << resp.body.GetBuffer();
}
