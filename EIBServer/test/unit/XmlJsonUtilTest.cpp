#include <gtest/gtest.h>
#include "XmlJsonUtil.h"

// ---------------------------------------------------------------------------
// JsonEscape tests
// ---------------------------------------------------------------------------

TEST(XmlJsonUtil_JsonEscape, PlainText)
{
    CString result = CXmlJsonUtil::JsonEscape("hello world");
    EXPECT_STREQ(result.GetBuffer(), "hello world");
}

TEST(XmlJsonUtil_JsonEscape, Quotes)
{
    CString result = CXmlJsonUtil::JsonEscape("say \"hello\"");
    EXPECT_STREQ(result.GetBuffer(), "say \\\"hello\\\"");
}

TEST(XmlJsonUtil_JsonEscape, Backslash)
{
    CString result = CXmlJsonUtil::JsonEscape("a\\b");
    EXPECT_STREQ(result.GetBuffer(), "a\\\\b");
}

TEST(XmlJsonUtil_JsonEscape, Newline)
{
    CString result = CXmlJsonUtil::JsonEscape("line1\nline2");
    EXPECT_STREQ(result.GetBuffer(), "line1\\nline2");
}

TEST(XmlJsonUtil_JsonEscape, Tab)
{
    CString result = CXmlJsonUtil::JsonEscape("col1\tcol2");
    EXPECT_STREQ(result.GetBuffer(), "col1\\tcol2");
}

TEST(XmlJsonUtil_JsonEscape, CarriageReturn)
{
    CString result = CXmlJsonUtil::JsonEscape("line\rend");
    EXPECT_STREQ(result.GetBuffer(), "line\\rend");
}

TEST(XmlJsonUtil_JsonEscape, MixedSpecials)
{
    CString result = CXmlJsonUtil::JsonEscape("a\"b\\c\nd\te\rf");
    EXPECT_STREQ(result.GetBuffer(), "a\\\"b\\\\c\\nd\\te\\rf");
}

TEST(XmlJsonUtil_JsonEscape, EmptyString)
{
    CString result = CXmlJsonUtil::JsonEscape("");
    EXPECT_STREQ(result.GetBuffer(), "");
}

// ---------------------------------------------------------------------------
// JsonError / JsonOk tests
// ---------------------------------------------------------------------------

TEST(XmlJsonUtil_JsonError, SimpleMessage)
{
    CString result = CXmlJsonUtil::JsonError("not found");
    EXPECT_STREQ(result.GetBuffer(), "{\"error\":\"not found\"}");
}

TEST(XmlJsonUtil_JsonError, MessageWithQuotes)
{
    CString result = CXmlJsonUtil::JsonError("file \"x\" missing");
    EXPECT_STREQ(result.GetBuffer(), "{\"error\":\"file \\\"x\\\" missing\"}");
}

TEST(XmlJsonUtil_JsonOk, ReturnsStatusOk)
{
    CString result = CXmlJsonUtil::JsonOk();
    EXPECT_STREQ(result.GetBuffer(), "{\"status\":\"ok\"}");
}

// ---------------------------------------------------------------------------
// XmlToJson tests
// ---------------------------------------------------------------------------

TEST(XmlJsonUtil_XmlToJson, EmptyString)
{
    CString result = CXmlJsonUtil::XmlToJson("");
    EXPECT_STREQ(result.GetBuffer(), "{}");
}

TEST(XmlJsonUtil_XmlToJson, SingleElement)
{
    CString result = CXmlJsonUtil::XmlToJson("<root><key>val</key></root>");
    EXPECT_STREQ(result.GetBuffer(), "{\"key\":\"val\"}");
}

TEST(XmlJsonUtil_XmlToJson, MultipleElements)
{
    CString xml = "<root><name>alice</name><age>30</age></root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"name\":\"alice\",\"age\":\"30\"}");
}

TEST(XmlJsonUtil_XmlToJson, RepeatedElements)
{
    CString xml = "<root><item>a</item><item>b</item><item>c</item></root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"item\":[\"a\",\"b\",\"c\"]}");
}

TEST(XmlJsonUtil_XmlToJson, NestedElements)
{
    CString xml = "<root><parent><child>val</child></parent></root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"parent\":{\"child\":\"val\"}}");
}

TEST(XmlJsonUtil_XmlToJson, EmptyElement)
{
    CString xml = "<root><empty></empty></root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"empty\":\"\"}");
}

TEST(XmlJsonUtil_XmlToJson, XmlDeclaration)
{
    CString xml = "<?xml version=\"1.0\"?><root><key>val</key></root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"key\":\"val\"}");
}

TEST(XmlJsonUtil_XmlToJson, WhitespaceHandling)
{
    CString xml = "<root>\n  <key>val</key>\n  <key2>val2</key2>\n</root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"key\":\"val\",\"key2\":\"val2\"}");
}

TEST(XmlJsonUtil_XmlToJson, SpecialCharsInText)
{
    CString xml = "<root><msg>say \"hi\"</msg></root>";
    CString result = CXmlJsonUtil::XmlToJson(xml);
    EXPECT_STREQ(result.GetBuffer(), "{\"msg\":\"say \\\"hi\\\"\"}");
}
