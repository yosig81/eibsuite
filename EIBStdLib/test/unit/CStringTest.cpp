#include <gtest/gtest.h>
#include "CString.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

class CStringTest : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

// Constructor Tests
TEST_F(CStringTest, DefaultConstructor_CreatesEmptyString) {
    CString str;
    EXPECT_TRUE(str.IsEmpty());
    EXPECT_EQ(0, str.GetLength());
}

TEST_F(CStringTest, CharPtrConstructor_CreatesStringFromCharPtr) {
    CString str("Hello");
    EXPECT_FALSE(str.IsEmpty());
    EXPECT_EQ(5, str.GetLength());
    EXPECT_STREQ("Hello", str.GetBuffer());
}

TEST_F(CStringTest, CharPtrConstructor_HandlesEmptyString) {
    CString str("");
    EXPECT_TRUE(str.IsEmpty());
    EXPECT_EQ(0, str.GetLength());
}

TEST_F(CStringTest, CharPtrWithLengthConstructor_CreatesSubstring) {
    CString str("HelloWorld", 5);
    EXPECT_EQ(5, str.GetLength());
    EXPECT_STREQ("Hello", str.GetBuffer());
}

TEST_F(CStringTest, IntConstructor_ConvertsIntToString) {
    CString str(123);
    EXPECT_STREQ("123", str.GetBuffer());

    CString negative(-456);
    EXPECT_STREQ("-456", negative.GetBuffer());
}

TEST_F(CStringTest, BoolConstructor_ConvertsBoolToString) {
    CString trueStr(true);
    EXPECT_STREQ("true", trueStr.GetBuffer());

    CString falseStr(false);
    EXPECT_STREQ("false", falseStr.GetBuffer());
}

TEST_F(CStringTest, BoolConstructor_WithCamelCase_ConvertsToTitleCase) {
    CString trueStr(true, true);
    EXPECT_STREQ("True", trueStr.GetBuffer());

    CString falseStr(false, true);
    EXPECT_STREQ("False", falseStr.GetBuffer());
}

TEST_F(CStringTest, DoubleConstructor_ConvertsDoubleToString) {
    CString str(3.14159);
    const char* result = str.GetBuffer();
    EXPECT_TRUE(strstr(result, "3.14") != nullptr);
}

TEST_F(CStringTest, CopyConstructor_CreatesExactCopy) {
    CString original("Test String");
    CString copy(original);
    EXPECT_STREQ(original.GetBuffer(), copy.GetBuffer());
    EXPECT_EQ(original.GetLength(), copy.GetLength());
}

// Operator Tests
TEST_F(CStringTest, EqualityOperator_ComparesStrings) {
    CString str1("Hello");
    CString str2("Hello");
    CString str3("World");

    EXPECT_TRUE(str1 == str2);
    EXPECT_FALSE(str1 == str3);
    EXPECT_TRUE(str1 == "Hello");
    EXPECT_FALSE(str1 == "World");
}

TEST_F(CStringTest, InequalityOperator_ComparesStrings) {
    CString str1("Hello");
    CString str2("World");

    EXPECT_TRUE(str1 != str2);
    EXPECT_FALSE(str1 != "Hello");
    EXPECT_TRUE(str1 != "World");
}

TEST_F(CStringTest, PlusOperator_ConcatenatesStrings) {
    CString str1("Hello");
    CString str2(" World");
    CString result = str1 + str2;

    EXPECT_STREQ("Hello World", result.GetBuffer());
    EXPECT_STREQ("Hello", str1.GetBuffer()); // Original unchanged
}

TEST_F(CStringTest, PlusEqualsOperator_AppendsToString) {
    CString str("Hello");
    str += " World";
    EXPECT_STREQ("Hello World", str.GetBuffer());

    str += 123;
    EXPECT_STREQ("Hello World123", str.GetBuffer());
}

TEST_F(CStringTest, AssignmentOperator_AssignsValue) {
    CString str;
    str = "Test";
    EXPECT_STREQ("Test", str.GetBuffer());

    str = 42;
    EXPECT_STREQ("42", str.GetBuffer());
}

TEST_F(CStringTest, ArraySubscriptOperator_ReturnsCharacter) {
    CString str("Hello");
    EXPECT_EQ('H', str[0]);
    EXPECT_EQ('e', str[1]);
    EXPECT_EQ('o', str[4]);
}

// String Operation Tests
TEST_F(CStringTest, ToLower_ConvertsToLowercase) {
    CString str("HeLLo WoRLd");
    str.ToLower();
    EXPECT_STREQ("hello world", str.GetBuffer());
}

TEST_F(CStringTest, ToUpper_ConvertsToUppercase) {
    CString str("HeLLo WoRLd");
    str.ToUpper();
    EXPECT_STREQ("HELLO WORLD", str.GetBuffer());
}

TEST_F(CStringTest, Trim_RemovesLeadingAndTrailingSpaces) {
    CString str("  Hello World  ");
    str.Trim();
    EXPECT_STREQ("Hello World", str.GetBuffer());
}

TEST_F(CStringTest, Trim_RemovesCustomDelimiter) {
    CString str("***Hello***");
    str.Trim('*');
    EXPECT_STREQ("Hello", str.GetBuffer());
}

TEST_F(CStringTest, TrimStart_RemovesLeadingSpaces) {
    CString str("  Hello World  ");
    str.TrimStart();
    EXPECT_STREQ("Hello World  ", str.GetBuffer());
}

TEST_F(CStringTest, SubString_ExtractsSubstring) {
    CString str("Hello World");
    CString sub = str.SubString(0, 5);
    EXPECT_STREQ("Hello", sub.GetBuffer());

    CString sub2 = str.SubString(6, 5);
    EXPECT_STREQ("World", sub2.GetBuffer());
}

TEST_F(CStringTest, SubString_HandlesZeroLength) {
    CString str("Test");
    CString sub = str.SubString(0, 0);
    EXPECT_TRUE(sub.IsEmpty());
}

TEST_F(CStringTest, Erase_RemovesSubstring) {
    CString str("Hello World");
    str.Erase(5, 6);
    EXPECT_STREQ("Hello", str.GetBuffer());
}

TEST_F(CStringTest, Clear_EmptiesString) {
    CString str("Hello");
    EXPECT_FALSE(str.IsEmpty());
    str.Clear();
    EXPECT_TRUE(str.IsEmpty());
    EXPECT_EQ(0, str.GetLength());
}

// Search Operation Tests
TEST_F(CStringTest, Find_FindsSubstring) {
    CString str("Hello World");
    EXPECT_EQ(0, str.Find("Hello"));
    EXPECT_EQ(6, str.Find("World"));
    EXPECT_EQ(-1, str.Find("NotFound"));
}

TEST_F(CStringTest, Find_WithStartPosition_FindsFromPosition) {
    CString str("Hello World Hello");
    EXPECT_EQ(0, str.Find("Hello", 0));
    EXPECT_EQ(12, str.Find("Hello", 1));
}

TEST_F(CStringTest, Find_FindsCharacter) {
    CString str("Hello");
    EXPECT_EQ(0, str.Find('H', 0));
    EXPECT_EQ(2, str.Find('l', 0));
    EXPECT_EQ(-1, str.Find('z', 0));
}

// FindFirstOf finds first character match, not substring
TEST_F(CStringTest, FindFirstOf_FindsFirstCharacter) {
    CString str("Hello");
    EXPECT_EQ(0, str.FindFirstOf('H'));
    EXPECT_EQ(1, str.FindFirstOf('e'));
    EXPECT_EQ(-1, str.FindFirstOf('z'));
}

// Conversion Tests
TEST_F(CStringTest, ToInt_ConvertsStringToInt) {
    CString str1("123");
    EXPECT_EQ(123, str1.ToInt());

    CString str2("-456");
    EXPECT_EQ(-456, str2.ToInt());

    CString str3("0");
    EXPECT_EQ(0, str3.ToInt());
}

TEST_F(CStringTest, ToUInt_ConvertsStringToUnsignedInt) {
    CString str("123");
    EXPECT_EQ(123u, str.ToUInt());
}

TEST_F(CStringTest, ToDouble_ConvertsStringToDouble) {
    CString str("3.14159");
    EXPECT_TRUE(FloatEquals(3.14159, str.ToDouble()));

    CString str2("-2.5");
    EXPECT_TRUE(FloatEquals(-2.5, str2.ToDouble()));
}

TEST_F(CStringTest, ToBool_ConvertsCaseSensitive) {
    // ToBool is case sensitive - only lowercase "true" returns true
    CString trueStr1("true");
    EXPECT_TRUE(trueStr1.ToBool());

    CString falseStr("false");
    EXPECT_FALSE(falseStr.ToBool());

    CString anything("anything");
    EXPECT_FALSE(anything.ToBool());
}

TEST_F(CStringTest, ToChar_ConvertsStringToChar) {
    CString str("A");
    EXPECT_EQ('A', str.ToChar());
}

TEST_F(CStringTest, ToShort_ConvertsStringToShort) {
    CString str("32000");
    EXPECT_EQ(32000, str.ToShort());

    CString negative("-100");
    EXPECT_EQ(-100, negative.ToShort());
}

TEST_F(CStringTest, ToLong_ConvertsStringToLong) {
    CString str("1000000");
    EXPECT_EQ(1000000L, str.ToLong());
}

// Edge Case Tests
TEST_F(CStringTest, EmptyString_HandledCorrectly) {
    CString str;
    EXPECT_TRUE(str.IsEmpty());
    EXPECT_EQ(0, str.GetLength());
    EXPECT_STREQ("", str.GetBuffer());

    str.ToLower();
    EXPECT_TRUE(str.IsEmpty());

    str.ToUpper();
    EXPECT_TRUE(str.IsEmpty());

    EXPECT_EQ(-1, str.Find("test"));
}

TEST_F(CStringTest, MultipleOperations_WorkCorrectly) {
    CString str("  HELLO world  ");
    str.Trim();
    str.ToLower();
    CString result = str + "!";
    EXPECT_STREQ("hello world!", result.GetBuffer());
}

TEST_F(CStringTest, HashCode_ReturnsValue) {
    CString str("test");
    unsigned int hash = str.HashCode();
    EXPECT_GT(hash, 0u);

    CString str2("test");
    EXPECT_EQ(hash, str2.HashCode());
}

// Hex Format Tests
TEST_F(CStringTest, ToHexFormat_ConvertsCharToHex) {
    CString hex = CString::ToHexFormat((unsigned char)255);
    EXPECT_TRUE(hex.Find("ff") >= 0 || hex.Find("FF") >= 0);
}

TEST_F(CStringTest, ToHexFormat_ConvertsIntToHex) {
    CString hex = CString::ToHexFormat(255);
    EXPECT_TRUE(hex.Find("ff") >= 0 || hex.Find("FF") >= 0);
}

// GetBuffer Tests
TEST_F(CStringTest, GetBuffer_ReturnsValidPointer) {
    CString str("Test");
    const char* buffer = str.GetBuffer();
    EXPECT_NE(nullptr, buffer);
    EXPECT_STREQ("Test", buffer);
}

// GetLength Tests
TEST_F(CStringTest, GetLength_ReturnsCorrectLength) {
    CString str("Hello");
    EXPECT_EQ(5, str.GetLength());

    CString empty;
    EXPECT_EQ(0, empty.GetLength());
}
