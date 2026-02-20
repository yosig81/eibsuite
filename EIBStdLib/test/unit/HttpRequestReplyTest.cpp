#include <gtest/gtest.h>
#include "HttpHeader.h"
#include "HttpRequest.h"
#include "HttpReply.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

namespace {
CString BufferToString(const CDataBuffer& buf) {
    return CString(buf.GetBuffer(), buf.GetLength());
}
}  // namespace

class HttpRequestReplyTest : public BaseTestFixture {};

TEST_F(HttpRequestReplyTest, HttpHeader_ParsesNameAndValue) {
    CHttpHeader header("Content-Length: 42");
    EXPECT_STREQ("Content-Length", header.GetName().GetBuffer());
    EXPECT_STREQ(" 42", header.GetValue().GetBuffer());
}

TEST_F(HttpRequestReplyTest, HttpRequest_AddDuplicateHeaderThrows) {
    CHttpRequest request;
    request.AddHeader("Host", "example.com");
    EXPECT_THROW(request.AddHeader("Host", "example.com"), CEIBException);
}

TEST_F(HttpRequestReplyTest, HttpRequest_FinalizeBuildsRequestText) {
    CHttpRequest request;
    request.SetMethod(GET_M);
    request.SetRequestURI("/health");
    request.SetVersion(HTTP_1_0);
    request.AddHeader("Host", "example.com");

    CDataBuffer raw;
    request.Finalize(raw);
    CString text = BufferToString(raw);

    EXPECT_NE(string::npos, text.Find("GET /health HTTP/1.0\r\n"));
    EXPECT_NE(string::npos, text.Find("Host: example.com\r\n"));
    EXPECT_NE(string::npos, text.Find("Content-Length: 0\r\n"));
}

TEST_F(HttpRequestReplyTest, HttpReply_FinalizeIncludesStatusHeadersAndBody) {
    CHttpReply reply;
    reply.SetVersion(HTTP_1_0);
    reply.SetStatusCode(STATUS_OK);
    reply.SetContentType(CT_TEXT_PLAIN);
    reply.GetContent().Add("hello", 5);

    CDataBuffer raw;
    reply.Finalize(raw);
    CString text = BufferToString(raw);

    EXPECT_NE(string::npos, text.Find("HTTP/1.0 200 OK\r\n"));
    EXPECT_NE(string::npos, text.Find("Content-Type: text/plain\r\n"));
    EXPECT_NE(string::npos, text.Find("Content-Length: 5\r\n"));
    EXPECT_NE(string::npos, text.Find("hello"));
}

TEST_F(HttpRequestReplyTest, HttpReply_RedirectSetsLocationAndStatusFound) {
    CHttpReply reply;
    reply.SetVersion(HTTP_1_0);
    reply.SetContentType(CT_TEXT_PLAIN);
    reply.Redirect("/login");

    CDataBuffer raw;
    reply.Finalize(raw);
    CString text = BufferToString(raw);

    EXPECT_NE(string::npos, text.Find("HTTP/1.0 302 Found\r\n"));
    EXPECT_NE(string::npos, text.Find("Location: /login\r\n"));
}
