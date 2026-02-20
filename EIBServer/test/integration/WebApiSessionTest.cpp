// WebApiSessionTest.cpp -- Login, logout, and session management tests.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class WebApiSessionTest : public ::testing::Test {
protected:
    HttpTestClient http;
};

TEST_F(WebApiSessionTest, LoginValidCredentials)
{
    CString body = "{\"user\":\"admin\",\"pass\":\"admin123\"}";
    HttpResponse resp = http.Post("/api/login", body);
    EXPECT_EQ(resp.status_code, 200);
    // Should set a session cookie
    EXPECT_GE(resp.set_cookie.Find("WEBSESSIONID="), 0)
        << "Set-Cookie: " << resp.set_cookie.GetBuffer();
    // Body should contain "ok"
    EXPECT_GE(resp.body.Find("\"status\":\"ok\""), 0)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiSessionTest, LoginInvalidPassword)
{
    CString body = "{\"user\":\"admin\",\"pass\":\"wrongpassword\"}";
    HttpResponse resp = http.Post("/api/login", body);
    EXPECT_EQ(resp.status_code, 401);
}

TEST_F(WebApiSessionTest, LoginUnknownUser)
{
    CString body = "{\"user\":\"nonexistent\",\"pass\":\"whatever\"}";
    HttpResponse resp = http.Post("/api/login", body);
    EXPECT_EQ(resp.status_code, 401);
}

TEST_F(WebApiSessionTest, LoginNoWebAccess)
{
    // noaccess user has privileges=0 (no WEB_ACCESS bit)
    CString body = "{\"user\":\"noaccess\",\"pass\":\"noaccess123\"}";
    HttpResponse resp = http.Post("/api/login", body);
    EXPECT_EQ(resp.status_code, 403);
}

TEST_F(WebApiSessionTest, SessionCheckAfterLogin)
{
    CString sid = http.Login("admin", "admin123");
    ASSERT_GT(sid.GetLength(), 0) << "Login failed";

    HttpResponse resp = http.Get("/api/session", sid);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_GE(resp.body.Find("\"authenticated\":true"), 0)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiSessionTest, SessionCheckWithoutLogin)
{
    HttpResponse resp = http.Get("/api/session");
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_GE(resp.body.Find("\"authenticated\":false"), 0)
        << "Body: " << resp.body.GetBuffer();
}

TEST_F(WebApiSessionTest, LogoutClearsSession)
{
    CString sid = http.Login("admin", "admin123");
    ASSERT_GT(sid.GetLength(), 0) << "Login failed";

    // Logout
    HttpResponse logout_resp = http.Post("/api/logout", "", sid);
    EXPECT_EQ(logout_resp.status_code, 200);

    // Session should now be invalid
    HttpResponse check_resp = http.Get("/api/session", sid);
    EXPECT_EQ(check_resp.status_code, 200);
    EXPECT_GE(check_resp.body.Find("\"authenticated\":false"), 0)
        << "Body: " << check_resp.body.GetBuffer();
}
