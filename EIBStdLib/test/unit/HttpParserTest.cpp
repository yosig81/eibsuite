#include <gtest/gtest.h>
#include "HttpParser.h"
#include "HttpParameter.h"
#include "../fixtures/TestHelpers.h"
#include <cstring>

using namespace EIBStdLibTest;

class HttpParserTest : public BaseTestFixture {};

TEST_F(HttpParserTest, ParseRequest_GetWithQueryAndCookie) {
    const char* raw =
        "GET /api?token=abc HTTP/1.0\r\n"
        "Host: example.com\r\n"
        "Cookie: sid=abc123; Path=/\r\n"
        "\r\n";

    CHttpRequest request;
    CHttpParser parser(request, raw, static_cast<int>(std::strlen(raw)));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(GET_M, request.GetMethod());
    EXPECT_EQ(HTTP_1_0, request.GetVersion());
    EXPECT_STREQ("/api", request.GetRequestURI().GetBuffer());

    CHttpHeader host;
    ASSERT_TRUE(request.GetHeader("Host", host));
    EXPECT_STREQ("example.com", host.GetValue().GetBuffer());

    CHttpParameter param;
    ASSERT_TRUE(request.GetParameter("token", param));
    EXPECT_STREQ("abc", param.GetValue().GetBuffer());

    CHttpCookie cookie;
    ASSERT_TRUE(request.GetCookie("sid", cookie));
    EXPECT_STREQ("abc123", cookie.GetValue().GetBuffer());
}

TEST_F(HttpParserTest, ParseRequest_UnknownMethodIsIllegal) {
    const char* raw =
        "PUT /api HTTP/1.0\r\n"
        "Host: example.com\r\n"
        "\r\n";

    CHttpRequest request;
    CHttpParser parser(request, raw, static_cast<int>(std::strlen(raw)));
    EXPECT_FALSE(parser.IsLegalRequest());
}

TEST_F(HttpParserTest, ParseReply_OkResponseParsesVersionStatusAndHeaders) {
    const char* raw =
        "HTTP/1.0 200 OK\r\n"
        "Content-Length: 5\r\n"
        "Server: unit-test\r\n"
        "\r\n"
        "hello";

    CHttpReply reply;
    CHttpParser parser(reply, raw, static_cast<int>(std::strlen(raw)));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(HTTP_1_0, reply.GetVersion());
    EXPECT_EQ(STATUS_OK, reply.GetStatusCode());

    CHttpHeader server;
    ASSERT_TRUE(reply.GetHeader("Server", server));
    EXPECT_STREQ("unit-test", server.GetValue().GetBuffer());
}

TEST_F(HttpParserTest, ParseReply_UnknownVersionIsIllegal) {
    const char* raw =
        "HTTP/2.0 200 OK\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    CHttpReply reply;
    CHttpParser parser(reply, raw, static_cast<int>(std::strlen(raw)));
    EXPECT_FALSE(parser.IsLegalRequest());
}

// -----------------------------------------------------------------------
// POST request content parsing
// -----------------------------------------------------------------------

TEST_F(HttpParserTest, ParseRequest_PostWithXmlBody) {
    const char* body = "<Root><Name>test</Name></Root>";
    int body_len = static_cast<int>(std::strlen(body));

    std::string raw =
        std::string("POST /api/admin/users HTTP/1.0\r\n") +
        "Host: localhost\r\n" +
        "Content-Type: text/xml\r\n" +
        "Content-Length: " + std::to_string(body_len) + "\r\n" +
        "\r\n" +
        body;

    CHttpRequest request;
    CHttpParser parser(request, raw.c_str(), static_cast<int>(raw.size()));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(POST_M, request.GetMethod());
    EXPECT_EQ(HTTP_1_0, request.GetVersion());
    EXPECT_STREQ("/api/admin/users", request.GetRequestURI().GetBuffer());

    // Verify the body was captured in the request content
    ASSERT_GT(request.GetContent().GetLength(), 0)
        << "Request content should not be empty after POST with body";
    EXPECT_EQ(body_len, request.GetContent().GetLength());

    std::string captured(
        reinterpret_cast<const char*>(request.GetContent().GetBuffer()),
        request.GetContent().GetLength());
    EXPECT_EQ(body, captured);
}

TEST_F(HttpParserTest, ParseRequest_PostWithJsonBody) {
    const char* body = "{\"user\":\"admin\",\"pass\":\"secret\"}";
    int body_len = static_cast<int>(std::strlen(body));

    std::string raw =
        std::string("POST /api/login HTTP/1.1\r\n") +
        "Host: localhost\r\n" +
        "Content-Type: application/json\r\n" +
        "Content-Length: " + std::to_string(body_len) + "\r\n" +
        "Cookie: WEBSESSIONID=abc123\r\n" +
        "\r\n" +
        body;

    CHttpRequest request;
    CHttpParser parser(request, raw.c_str(), static_cast<int>(raw.size()));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(POST_M, request.GetMethod());
    EXPECT_EQ(HTTP_1_1, request.GetVersion());

    ASSERT_EQ(body_len, request.GetContent().GetLength());
    std::string captured(
        reinterpret_cast<const char*>(request.GetContent().GetBuffer()),
        request.GetContent().GetLength());
    EXPECT_EQ(body, captured);

    CHttpCookie cookie;
    ASSERT_TRUE(request.GetCookie("WEBSESSIONID", cookie));
    EXPECT_STREQ("abc123", cookie.GetValue().GetBuffer());
}

TEST_F(HttpParserTest, ParseRequest_PostWithNoContentLength_EmptyBody) {
    const char* raw =
        "POST /api/test HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    CHttpRequest request;
    CHttpParser parser(request, raw, static_cast<int>(std::strlen(raw)));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(POST_M, request.GetMethod());
    EXPECT_EQ(0, request.GetContent().GetLength())
        << "POST with no Content-Length should have empty body";
}

TEST_F(HttpParserTest, ParseRequest_PostWithZeroContentLength_EmptyBody) {
    const char* raw =
        "POST /api/test HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";

    CHttpRequest request;
    CHttpParser parser(request, raw, static_cast<int>(std::strlen(raw)));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(0, request.GetContent().GetLength());
}

TEST_F(HttpParserTest, ParseRequest_PostContentLengthHeaderLookup) {
    // Verify that Content-Length is looked up correctly (exact case match)
    const char* body = "data";
    std::string raw =
        std::string("POST /api HTTP/1.0\r\n") +
        "Host: localhost\r\n" +
        "Content-Type: text/plain\r\n" +
        "Content-Length: 4\r\n" +
        "\r\n" +
        body;

    CHttpRequest request;
    CHttpParser parser(request, raw.c_str(), static_cast<int>(raw.size()));

    ASSERT_TRUE(parser.IsLegalRequest());

    CHttpHeader cl;
    ASSERT_TRUE(request.GetHeader("Content-Length", cl))
        << "Content-Length header should be retrievable after parsing";
    EXPECT_STREQ("4", cl.GetValue().GetBuffer());
}

// -----------------------------------------------------------------------
// HTTP/1.1 parsing
// -----------------------------------------------------------------------

TEST_F(HttpParserTest, ParseRequest_Http11GetIsLegal) {
    const char* raw =
        "GET /api/status HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    CHttpRequest request;
    CHttpParser parser(request, raw, static_cast<int>(std::strlen(raw)));

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(GET_M, request.GetMethod());
    EXPECT_EQ(HTTP_1_1, request.GetVersion());
}

// -----------------------------------------------------------------------
// CDataBuffer constructor (for completeness — no socket needed)
// -----------------------------------------------------------------------

TEST_F(HttpParserTest, ParseRequest_FromDataBuffer) {
    const char* raw =
        "GET /test HTTP/1.0\r\n"
        "Host: localhost\r\n"
        "\r\n";

    CDataBuffer buf(raw, static_cast<int>(std::strlen(raw)));
    CHttpRequest request;
    CHttpParser parser(request, buf);

    ASSERT_TRUE(parser.IsLegalRequest());
    EXPECT_EQ(GET_M, request.GetMethod());
    EXPECT_STREQ("/test", request.GetRequestURI().GetBuffer());
}
