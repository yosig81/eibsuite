// WebApiAdminTest.cpp -- Admin API endpoint tests.

#include "IntegrationHelpers.h"
#include "UsersDB.h"
#include <fstream>

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
    EXPECT_NE(resp.body.Find("MODE"), string::npos)
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

TEST_F(WebApiAdminTest, SetUsersAddsNewUser)
{
    // Build XML payload that keeps the existing admin user and adds a new one
    CString xml =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<Root><EIB_SERVER_USERS_LIST>"
        "<EIB_SERVER_USER>"
            "<EIB_SERVER_USER_IP_ADDRESS>0.0.0.0</EIB_SERVER_USER_IP_ADDRESS>"
            "<EIB_SERVER_USER_NAME>admin</EIB_SERVER_USER_NAME>"
            "<EIB_SERVER_USER_PASSWORD>admin123</EIB_SERVER_USER_PASSWORD>"
            "<EIB_SERVER_USER_IS_CONNECTED>false</EIB_SERVER_USER_IS_CONNECTED>"
            "<EIB_SERVER_USER_SESSION_ID>0</EIB_SERVER_USER_SESSION_ID>"
            "<EIB_SERVER_USER_PRIVILIGES>15</EIB_SERVER_USER_PRIVILIGES>"
            "<EIB_SERVER_USER_SOURCE_ADDR_MASK>65535</EIB_SERVER_USER_SOURCE_ADDR_MASK>"
            "<EIB_SERVER_USER_DST_ADDR_MASK>65535</EIB_SERVER_USER_DST_ADDR_MASK>"
        "</EIB_SERVER_USER>"
        "<EIB_SERVER_USER>"
            "<EIB_SERVER_USER_IP_ADDRESS>0.0.0.0</EIB_SERVER_USER_IP_ADDRESS>"
            "<EIB_SERVER_USER_NAME>newuser</EIB_SERVER_USER_NAME>"
            "<EIB_SERVER_USER_PASSWORD>newpass</EIB_SERVER_USER_PASSWORD>"
            "<EIB_SERVER_USER_IS_CONNECTED>false</EIB_SERVER_USER_IS_CONNECTED>"
            "<EIB_SERVER_USER_SESSION_ID>0</EIB_SERVER_USER_SESSION_ID>"
            "<EIB_SERVER_USER_PRIVILIGES>7</EIB_SERVER_USER_PRIVILIGES>"
            "<EIB_SERVER_USER_SOURCE_ADDR_MASK>65535</EIB_SERVER_USER_SOURCE_ADDR_MASK>"
            "<EIB_SERVER_USER_DST_ADDR_MASK>65535</EIB_SERVER_USER_DST_ADDR_MASK>"
        "</EIB_SERVER_USER>"
        "</EIB_SERVER_USERS_LIST></Root>";

    HttpResponse resp = http.PostXml("/api/admin/users", xml, admin_sid);
    EXPECT_EQ(resp.status_code, 200)
        << "Body: " << resp.body.GetBuffer();
    EXPECT_NE(resp.body.Find("ok"), string::npos)
        << "Expected ok in response, got: " << resp.body.GetBuffer();

    // Verify Users.db on disk contains the new user
    std::ifstream db_file("conf/Users.db");
    ASSERT_TRUE(db_file.is_open()) << "Could not open conf/Users.db";
    std::string content((std::istreambuf_iterator<char>(db_file)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("[newuser]"), std::string::npos)
        << "Users.db does not contain [newuser]. Content:\n" << content;
    EXPECT_NE(content.find("PASSWORD = newpass"), std::string::npos)
        << "Users.db does not contain newuser's password. Content:\n" << content;
}
