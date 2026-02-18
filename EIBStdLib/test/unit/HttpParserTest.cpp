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
