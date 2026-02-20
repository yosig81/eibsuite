// WebApiAdminTest.cpp -- Admin API endpoint tests.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class WebApiAdminTest : public ::testing::Test {
protected:
    HttpTestClient http;
    CString admin_sid;

    void SetUp() override {
        admin_sid = http.Login("admin", "admin123");
        ASSERT_GT(admin_sid.GetLength(), 0) << "Admin login failed";
    }
};

TEST_F(WebApiAdminTest, GetUsersAsAdmin)
{
    HttpResponse resp = http.Get("/api/admin/users", admin_sid);
    EXPECT_EQ(resp.status_code, 200)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiAdminTest, GetInterfaceStatus)
{
    HttpResponse resp = http.Get("/api/admin/interface", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    // Should contain mode information
    EXPECT_GE(resp.body.Find("MODE"), 0)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiAdminTest, GetBusMonAddresses)
{
    HttpResponse resp = http.Get("/api/admin/busmon", admin_sid);
    EXPECT_EQ(resp.status_code, 200)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiAdminTest, UnknownApiEndpoint)
{
    HttpResponse resp = http.Get("/api/nonexistent", admin_sid);
    EXPECT_EQ(resp.status_code, 404);
}
