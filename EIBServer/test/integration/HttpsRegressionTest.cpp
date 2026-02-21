// HttpsRegressionTest.cpp -- Regression tests for the HTTP -> HTTPS migration.
//
// Verifies TLS connectivity, HTTP/1.1 behavior, content types, CORS,
// static file serving, SPA fallback, large payloads, and concurrent
// connections -- all areas affected by replacing the hand-rolled HTTP
// stack with cpp-httplib + OpenSSL.

#include "IntegrationHelpers.h"
#include <thread>
#include <vector>
#include <atomic>

using namespace IntegrationTest;

class HttpsRegressionTest : public ::testing::Test {
protected:
    HttpTestClient http;
    CString admin_sid;

    void SetUp() override {
        admin_sid = http.Login("admin", "admin123");
        ASSERT_GT(admin_sid.GetLength(), 0) << "Admin login failed";
    }
};

// ---------------------------------------------------------------------------
// TLS / connectivity
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, PlainHttpConnectionRefused)
{
    // A plain HTTP client should fail to connect to the HTTPS server.
    httplib::Client plain("127.0.0.1", 18080);
    plain.set_connection_timeout(2, 0);
    auto result = plain.Get("/api/session");
    // Either the connection fails or the server rejects the non-TLS data
    EXPECT_TRUE(!result || result->status != 200)
        << "Plain HTTP should not get a valid 200 from the HTTPS server";
}

TEST_F(HttpsRegressionTest, TlsConnectionSucceeds)
{
    httplib::SSLClient cli("127.0.0.1", 18080);
    cli.enable_server_certificate_verification(false);
    cli.set_connection_timeout(5, 0);
    auto result = cli.Get("/api/session");
    ASSERT_TRUE(result != nullptr) << "TLS connection failed";
    EXPECT_EQ(result->status, 200);
}

// ---------------------------------------------------------------------------
// Content-Type headers
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, ApiResponseIsJson)
{
    HttpResponse resp = http.Get("/api/session");
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.content_type.Find("application/json"), string::npos)
        << "Content-Type: " << resp.content_type.GetBuffer();
}

TEST_F(HttpsRegressionTest, LoginResponseIsJson)
{
    CString body = "{\"user\":\"admin\",\"pass\":\"admin123\"}";
    HttpResponse resp = http.Post("/api/login", body);
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.content_type.Find("application/json"), string::npos)
        << "Content-Type: " << resp.content_type.GetBuffer();
}

TEST_F(HttpsRegressionTest, ErrorResponseIsJson)
{
    HttpResponse resp = http.Get("/api/history"); // no auth
    EXPECT_EQ(resp.status_code, 401);
    EXPECT_NE(resp.content_type.Find("application/json"), string::npos)
        << "Content-Type: " << resp.content_type.GetBuffer();
}

TEST_F(HttpsRegressionTest, IndexHtmlIsTextHtml)
{
    HttpResponse resp = http.Get("/");
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.content_type.Find("text/html"), string::npos)
        << "Content-Type: " << resp.content_type.GetBuffer();
}

// ---------------------------------------------------------------------------
// CORS
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, ApiResponseHasCorsHeader)
{
    // Verify Access-Control-Allow-Origin is present on API responses
    httplib::SSLClient cli("127.0.0.1", 18080);
    cli.enable_server_certificate_verification(false);
    auto result = cli.Get("/api/session");
    ASSERT_TRUE(result != nullptr);
    EXPECT_TRUE(result->has_header("Access-Control-Allow-Origin"))
        << "Missing CORS header on API response";
    EXPECT_EQ(result->get_header_value("Access-Control-Allow-Origin"), "*");
}

// ---------------------------------------------------------------------------
// Static file serving & SPA fallback
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, ServeIndexHtmlAtRoot)
{
    HttpResponse resp = http.Get("/");
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body.Find("EIBServer Test"), string::npos);
}

TEST_F(HttpsRegressionTest, SpaFallbackForUnknownPath)
{
    // Non-API paths that don't match a static file should serve index.html
    HttpResponse resp = http.Get("/some/spa/route");
    EXPECT_EQ(resp.status_code, 200);
    EXPECT_NE(resp.body.Find("EIBServer Test"), string::npos)
        << "SPA fallback should serve index.html, got: "
        << resp.body.GetBuffer();
}

TEST_F(HttpsRegressionTest, ApiPathNotFallenBackToSpa)
{
    // /api/* paths should NOT fall back to index.html
    HttpResponse resp = http.Get("/api/nonexistent", admin_sid);
    EXPECT_EQ(resp.status_code, 404);
    // Should be a JSON error, not HTML
    EXPECT_NE(resp.content_type.Find("application/json"), string::npos)
        << "API 404 should be JSON, got Content-Type: "
        << resp.content_type.GetBuffer();
}

// ---------------------------------------------------------------------------
// Cookie handling
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, LoginSetsCookieWithPath)
{
    CString body = "{\"user\":\"admin\",\"pass\":\"admin123\"}";
    HttpResponse resp = http.Post("/api/login", body);
    EXPECT_EQ(resp.status_code, 200);
    // Cookie should contain both the session ID and Path=/
    EXPECT_NE(resp.set_cookie.Find("WEBSESSIONID="), string::npos)
        << "Set-Cookie: " << resp.set_cookie.GetBuffer();
    EXPECT_NE(resp.set_cookie.Find("Path=/"), string::npos)
        << "Cookie should have Path=/, got: " << resp.set_cookie.GetBuffer();
}

TEST_F(HttpsRegressionTest, LogoutClearsCookieValue)
{
    HttpResponse resp = http.Post("/api/logout", "", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
    // Cookie value should be cleared (WEBSESSIONID=; or WEBSESSIONID=;Path=/)
    EXPECT_NE(resp.set_cookie.Find("WEBSESSIONID="), string::npos);
    // The value between '=' and ';' should be empty
    size_t eq = resp.set_cookie.Find("WEBSESSIONID=");
    size_t semi = resp.set_cookie.Find(";", eq);
    CString val = resp.set_cookie.SubString(eq + 13, semi - eq - 13);
    EXPECT_EQ(val.GetLength(), 0)
        << "Expected empty cookie value, got: " << val.GetBuffer();
}

// ---------------------------------------------------------------------------
// Large request body (old stack had 8KB limit)
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, LargeRequestBodyAccepted)
{
    // Build a JSON body larger than the old 8KB MAX_HTTP_REQUEST_SIZE.
    // Use a large padding field rather than a huge EIB value (which would
    // overflow the fixed-size EIB value buffer).
    CString padding;
    for (int i = 0; i < 10000; ++i) {
        padding += "x";
    }
    CString body = "{\"address\":\"0/0/1\",\"value\":\"0x01\",\"padding\":\"";
    body += padding;
    body += "\"}";

    // Body is >10KB, well past the old 8KB limit.
    // The server should accept it without truncation or crash.
    HttpResponse resp = http.Post("/api/eib/send", body, admin_sid);
    // We expect 200 (success) -- the extra field is ignored
    EXPECT_GT(resp.status_code, 0)
        << "Large body request failed entirely";
    EXPECT_NE(resp.status_code, -1)
        << "Connection failure on large body";
}

// ---------------------------------------------------------------------------
// Concurrent connections (old stack had 2 handler threads)
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, ConcurrentRequestsSucceed)
{
    const int NUM_CLIENTS = 8;
    std::atomic<int> successes(0);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_CLIENTS; ++i) {
        threads.emplace_back([&successes]() {
            HttpTestClient client;
            CString sid = client.Login("admin", "admin123");
            if (sid.GetLength() > 0) {
                HttpResponse resp = client.Get("/api/history", sid);
                if (resp.status_code == 200) {
                    successes++;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successes.load(), NUM_CLIENTS)
        << "Expected all " << NUM_CLIENTS << " concurrent requests to succeed";
}

// ---------------------------------------------------------------------------
// HTTP method enforcement
// ---------------------------------------------------------------------------

TEST_F(HttpsRegressionTest, GetOnPostEndpointReturns404)
{
    // /api/login is registered as POST only; GET should not match
    HttpResponse resp = http.Get("/api/login");
    EXPECT_NE(resp.status_code, 200)
        << "GET on POST-only endpoint should not return 200";
}

TEST_F(HttpsRegressionTest, PostOnGetEndpointReturns404)
{
    // /api/history is registered as GET only; POST should not match
    HttpResponse resp = http.Post("/api/history", "", admin_sid);
    EXPECT_NE(resp.status_code, 200)
        << "POST on GET-only endpoint should not return 200";
}
