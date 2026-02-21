#include <gtest/gtest.h>
#include "WebHandler.h"
#include "Html.h"

// CWebHandler declares:  friend class WebHandlerUtilTest;
// This gives the test fixture access to the private utility methods.

class WebHandlerUtilTest : public ::testing::Test {
protected:
    // Wrappers that call the private static methods via friend access.
    CString GetJsonField(const CString& json, const CString& field) {
        return CWebHandler::GetJsonField(json, field);
    }
    bool GetByteArrayFromHexString(const CString& str, unsigned char* val, unsigned char& val_len) {
        return CWebHandler::GetByteArrayFromHexString(str, val, val_len);
    }
    CString GetMimeType(const CString& path) {
        return CWebHandler::GetMimeType(path);
    }
    unsigned char HexToChar(const CString& hex) {
        return CWebHandler::HexToChar(hex);
    }
    int GetDigitValue(char c) {
        return CWebHandler::GetDigitValue(c);
    }
};

// ---------------------------------------------------------------------------
// GetJsonField
// ---------------------------------------------------------------------------

TEST_F(WebHandlerUtilTest, GetJsonField_SimpleField)
{
    CString json = "{\"user\":\"alice\"}";
    EXPECT_STREQ(GetJsonField(json, "user").GetBuffer(), "alice");
}

TEST_F(WebHandlerUtilTest, GetJsonField_FieldNotFound)
{
    CString json = "{\"user\":\"alice\"}";
    EXPECT_STREQ(GetJsonField(json, "missing").GetBuffer(), "");
}

TEST_F(WebHandlerUtilTest, GetJsonField_EmptyJson)
{
    EXPECT_STREQ(GetJsonField("", "user").GetBuffer(), "");
}

TEST_F(WebHandlerUtilTest, GetJsonField_MultipleFields)
{
    CString json = "{\"user\":\"alice\",\"pass\":\"secret\"}";
    EXPECT_STREQ(GetJsonField(json, "user").GetBuffer(), "alice");
    EXPECT_STREQ(GetJsonField(json, "pass").GetBuffer(), "secret");
}

TEST_F(WebHandlerUtilTest, GetJsonField_EmptyValue)
{
    CString json = "{\"user\":\"\"}";
    EXPECT_STREQ(GetJsonField(json, "user").GetBuffer(), "");
}

TEST_F(WebHandlerUtilTest, GetJsonField_NumericLookupFails)
{
    // GetJsonField expects "field":"value" pattern -- numeric values
    // without quotes won't match the search pattern.
    CString json = "{\"count\":42}";
    EXPECT_STREQ(GetJsonField(json, "count").GetBuffer(), "");
}

// ---------------------------------------------------------------------------
// GetByteArrayFromHexString
// ---------------------------------------------------------------------------

TEST_F(WebHandlerUtilTest, GetByteArrayFromHexString_ValidSingleByte)
{
    unsigned char val[16];
    unsigned char len = 0;
    EXPECT_TRUE(GetByteArrayFromHexString("0x0A", val, len));
    EXPECT_EQ(len, 1);
    EXPECT_EQ(val[0], 0x0A);
}

TEST_F(WebHandlerUtilTest, GetByteArrayFromHexString_ValidMultiByte)
{
    unsigned char val[16];
    unsigned char len = 0;
    EXPECT_TRUE(GetByteArrayFromHexString("0x1234", val, len));
    EXPECT_EQ(len, 2);
    EXPECT_EQ(val[0], 0x12);
    EXPECT_EQ(val[1], 0x34);
}

TEST_F(WebHandlerUtilTest, GetByteArrayFromHexString_LeadingZeros)
{
    unsigned char val[16];
    unsigned char len = 0;
    EXPECT_TRUE(GetByteArrayFromHexString("0x000A", val, len));
    // Leading zeros are trimmed, so result is {0x0A}, len=1
    EXPECT_EQ(len, 1);
    EXPECT_EQ(val[0], 0x0A);
}

TEST_F(WebHandlerUtilTest, GetByteArrayFromHexString_NoPrefix)
{
    unsigned char val[16];
    unsigned char len = 0;
    EXPECT_FALSE(GetByteArrayFromHexString("1234", val, len));
}

TEST_F(WebHandlerUtilTest, GetByteArrayFromHexString_OddLength)
{
    unsigned char val[16];
    unsigned char len = 0;
    // "0xA" -> after trimming "0x" and leading zeros: "A", odd -> pad to "0A"
    EXPECT_TRUE(GetByteArrayFromHexString("0xA", val, len));
    EXPECT_EQ(len, 1);
    EXPECT_EQ(val[0], 0x0A);
}

TEST_F(WebHandlerUtilTest, GetByteArrayFromHexString_UpperAndLowerCase)
{
    unsigned char val[16];
    unsigned char len = 0;
    EXPECT_TRUE(GetByteArrayFromHexString("0xABcd", val, len));
    EXPECT_EQ(len, 2);
    EXPECT_EQ(val[0], 0xAB);
    EXPECT_EQ(val[1], 0xCD);
}

// ---------------------------------------------------------------------------
// GetMimeType
// ---------------------------------------------------------------------------

TEST_F(WebHandlerUtilTest, GetMimeType_Html)
{
    EXPECT_STREQ(GetMimeType("index.html").GetBuffer(), MIME_TEXT_HTML);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Htm)
{
    EXPECT_STREQ(GetMimeType("page.htm").GetBuffer(), MIME_TEXT_HTML);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Css)
{
    EXPECT_STREQ(GetMimeType("style.css").GetBuffer(), MIME_TEXT_CSS);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Js)
{
    EXPECT_STREQ(GetMimeType("app.js").GetBuffer(), MIME_TEXT_JS);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Json)
{
    EXPECT_STREQ(GetMimeType("data.json").GetBuffer(), MIME_TEXT_JSON);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Png)
{
    EXPECT_STREQ(GetMimeType("logo.png").GetBuffer(), MIME_IMAGE_PNG);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Jpg)
{
    EXPECT_STREQ(GetMimeType("photo.jpg").GetBuffer(), MIME_IMAGE_JPEG);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Svg)
{
    EXPECT_STREQ(GetMimeType("icon.svg").GetBuffer(), MIME_IMAGE_SVG);
}

TEST_F(WebHandlerUtilTest, GetMimeType_Unknown)
{
    EXPECT_STREQ(GetMimeType("data.xyz").GetBuffer(), "application/octet-stream");
}

// ---------------------------------------------------------------------------
// HexToChar
// ---------------------------------------------------------------------------

TEST_F(WebHandlerUtilTest, HexToChar_FF)
{
    EXPECT_EQ(HexToChar("FF"), 0xFF);
}

TEST_F(WebHandlerUtilTest, HexToChar_0A)
{
    EXPECT_EQ(HexToChar("0A"), 0x0A);
}

TEST_F(WebHandlerUtilTest, HexToChar_00)
{
    EXPECT_EQ(HexToChar("00"), 0x00);
}

TEST_F(WebHandlerUtilTest, HexToChar_LowerCase)
{
    EXPECT_EQ(HexToChar("ab"), 0xAB);
}

// ---------------------------------------------------------------------------
// GetDigitValue
// ---------------------------------------------------------------------------

TEST_F(WebHandlerUtilTest, GetDigitValue_AllDigits)
{
    for (char c = '0'; c <= '9'; ++c) {
        EXPECT_EQ(GetDigitValue(c), c - '0') << "Failed for char: " << c;
    }
}

TEST_F(WebHandlerUtilTest, GetDigitValue_UpperHex)
{
    EXPECT_EQ(GetDigitValue('A'), 10);
    EXPECT_EQ(GetDigitValue('B'), 11);
    EXPECT_EQ(GetDigitValue('C'), 12);
    EXPECT_EQ(GetDigitValue('D'), 13);
    EXPECT_EQ(GetDigitValue('E'), 14);
    EXPECT_EQ(GetDigitValue('F'), 15);
}

TEST_F(WebHandlerUtilTest, GetDigitValue_LowerHex)
{
    EXPECT_EQ(GetDigitValue('a'), 10);
    EXPECT_EQ(GetDigitValue('b'), 11);
    EXPECT_EQ(GetDigitValue('c'), 12);
    EXPECT_EQ(GetDigitValue('d'), 13);
    EXPECT_EQ(GetDigitValue('e'), 14);
    EXPECT_EQ(GetDigitValue('f'), 15);
}

TEST_F(WebHandlerUtilTest, GetDigitValue_Invalid)
{
    EXPECT_EQ(GetDigitValue('G'), -1);
    EXPECT_EQ(GetDigitValue('z'), -1);
    EXPECT_EQ(GetDigitValue(' '), -1);
}
