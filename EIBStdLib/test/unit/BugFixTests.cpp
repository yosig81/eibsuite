#include <gtest/gtest.h>
#include "CString.h"
#include "StringTokenizer.h"
#include "../fixtures/TestHelpers.h"
#include <algorithm>
#include <vector>

using namespace EIBStdLibTest;

/**
 * Tests for bugs that were found and fixed in EIBStdLib
 * Each test documents the original bug and verifies the fix
 */

// =============================================================================
// BUG FIX 1: Comparison operators were inverted
// =============================================================================

class ComparisonOperatorTests : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(ComparisonOperatorTests, LessThan_ComparesCorrectly) {
    // BUG: operator< was implemented as operator> (inverted)
    // FIX: Changed _str > rhs._str to _str < rhs._str

    CString apple("Apple");
    CString banana("Banana");
    CString zebra("Zebra");

    // Alphabetically: Apple < Banana < Zebra
    EXPECT_TRUE(apple < banana);
    EXPECT_TRUE(apple < zebra);
    EXPECT_TRUE(banana < zebra);

    EXPECT_FALSE(banana < apple);
    EXPECT_FALSE(zebra < apple);
    EXPECT_FALSE(zebra < banana);
}

TEST_F(ComparisonOperatorTests, LessThan_WithCharPtr_ComparesCorrectly) {
    CString apple("Apple");

    EXPECT_TRUE(apple < "Banana");
    EXPECT_FALSE(apple < "Aardvark");
}

TEST_F(ComparisonOperatorTests, GreaterThan_StillWorksCorrectly) {
    // operator> was already correct, verify it still works
    CString apple("Apple");
    CString banana("Banana");

    EXPECT_TRUE(banana > apple);
    EXPECT_FALSE(apple > banana);
}

TEST_F(ComparisonOperatorTests, Sorting_WorksCorrectly) {
    // Real-world test: sorting should now work
    std::vector<CString> fruits;
    fruits.push_back(CString("Zebra"));
    fruits.push_back(CString("Apple"));
    fruits.push_back(CString("Mango"));
    fruits.push_back(CString("Banana"));

    std::sort(fruits.begin(), fruits.end());

    EXPECT_STREQ("Apple", fruits[0].GetBuffer());
    EXPECT_STREQ("Banana", fruits[1].GetBuffer());
    EXPECT_STREQ("Mango", fruits[2].GetBuffer());
    EXPECT_STREQ("Zebra", fruits[3].GetBuffer());
}

TEST_F(ComparisonOperatorTests, LessThan_EqualStrings_ReturnsFalse) {
    CString str1("Hello");
    CString str2("Hello");

    EXPECT_FALSE(str1 < str2);
    EXPECT_FALSE(str2 < str1);
}

TEST_F(ComparisonOperatorTests, LessThan_CaseSensitive) {
    CString lower("apple");
    CString upper("APPLE");

    // Uppercase letters come before lowercase in ASCII
    EXPECT_TRUE(upper < lower);
    EXPECT_FALSE(lower < upper);
}

// =============================================================================
// BUG FIX 2: EndsWith() was checking if substring exists anywhere
// =============================================================================

class EndsWithTests : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(EndsWithTests, EndsWith_TrueWhenAtEnd) {
    // This worked even with the bug (by accident)
    CString str("Hello World");

    EXPECT_TRUE(str.EndsWith("World"));
    EXPECT_TRUE(str.EndsWith("orld"));
    EXPECT_TRUE(str.EndsWith("d"));
}

TEST_F(EndsWithTests, EndsWith_FalseWhenAtBeginning) {
    // BUG: This returned true because it just checked if substring exists
    // FIX: Now checks if last occurrence is at the end
    CString str("Hello World");

    EXPECT_FALSE(str.EndsWith("Hello"));
    EXPECT_FALSE(str.EndsWith("Hell"));
}

TEST_F(EndsWithTests, EndsWith_FalseWhenInMiddle) {
    // BUG: This returned true
    CString str("Hello World");

    EXPECT_FALSE(str.EndsWith("lo Wo"));
    EXPECT_FALSE(str.EndsWith("llo"));
}

TEST_F(EndsWithTests, EndsWith_EmptyString_ReturnsTrue) {
    CString str("Hello");

    EXPECT_TRUE(str.EndsWith(""));
}

TEST_F(EndsWithTests, EndsWith_LongerString_ReturnsFalse) {
    CString str("Hi");

    EXPECT_FALSE(str.EndsWith("Hello"));
}

TEST_F(EndsWithTests, EndsWith_ExactMatch_ReturnsTrue) {
    CString str("Hello");

    EXPECT_TRUE(str.EndsWith("Hello"));
}

TEST_F(EndsWithTests, EndsWith_MultipleOccurrences_ChecksEnd) {
    // String with repeated pattern
    CString str("test test test");

    EXPECT_TRUE(str.EndsWith("test"));
    EXPECT_TRUE(str.EndsWith(" test"));
    EXPECT_FALSE(str.EndsWith("test ")); // Not at the end
}

TEST_F(EndsWithTests, EndsWith_CStringOverload_Works) {
    CString str("Hello World");
    CString suffix("World");

    EXPECT_TRUE(str.EndsWith(suffix));

    CString notSuffix("Hello");
    EXPECT_FALSE(str.EndsWith(notSuffix));
}

// =============================================================================
// BUG FIX 3: ToBool() was case-sensitive
// =============================================================================

class ToBoolTests : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(ToBoolTests, ToBool_Lowercase_ReturnsTrue) {
    // This always worked
    CString str("true");
    EXPECT_TRUE(str.ToBool());
}

TEST_F(ToBoolTests, ToBool_Uppercase_ReturnsTrue) {
    // BUG: This returned false (case-sensitive)
    // FIX: Now uses strcasecmp instead of strcmp
    CString str("TRUE");
    EXPECT_TRUE(str.ToBool());
}

TEST_F(ToBoolTests, ToBool_MixedCase_ReturnsTrue) {
    // BUG: These all returned false
    CString str1("True");
    EXPECT_TRUE(str1.ToBool());

    CString str2("TrUe");
    EXPECT_TRUE(str2.ToBool());

    CString str3("tRuE");
    EXPECT_TRUE(str3.ToBool());
}

TEST_F(ToBoolTests, ToBool_False_ReturnsFalse) {
    CString str1("false");
    EXPECT_FALSE(str1.ToBool());

    CString str2("False");
    EXPECT_FALSE(str2.ToBool());

    CString str3("FALSE");
    EXPECT_FALSE(str3.ToBool());
}

TEST_F(ToBoolTests, ToBool_OtherStrings_ReturnFalse) {
    CString str1("yes");
    EXPECT_FALSE(str1.ToBool());

    CString str2("1");
    EXPECT_FALSE(str2.ToBool());

    CString str3("anything");
    EXPECT_FALSE(str3.ToBool());
}

TEST_F(ToBoolTests, ToBool_RoundTrip_WithCamelCase) {
    // Constructor can create "True", now ToBool can parse it back
    CString trueStr(true, true); // Creates "True"
    EXPECT_STREQ("True", trueStr.GetBuffer());
    EXPECT_TRUE(trueStr.ToBool()); // BUG: This used to fail

    CString falseStr(false, true); // Creates "False"
    EXPECT_STREQ("False", falseStr.GetBuffer());
    EXPECT_FALSE(falseStr.ToBool());
}

// =============================================================================
// BUG FIX 4: StringTokenizer SubString length calculation
// =============================================================================

class StringTokenizerNextTokenTests : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(StringTokenizerNextTokenTests, NextToken_BasicTokenization) {
    // BUG: NextToken() caused assertion failure in SubString
    // FIX: Corrected length calculation in SubString call
    StringTokenizer tokenizer("apple,banana,cherry", ",");

    EXPECT_STREQ("apple", tokenizer.NextToken().GetBuffer());
    EXPECT_STREQ("banana", tokenizer.NextToken().GetBuffer());
    EXPECT_STREQ("cherry", tokenizer.NextToken().GetBuffer());
    EXPECT_FALSE(tokenizer.HasMoreTokens());
}

TEST_F(StringTokenizerNextTokenTests, NextToken_WithSpaces) {
    StringTokenizer tokenizer("one two three", " ");

    EXPECT_STREQ("one", tokenizer.NextToken().GetBuffer());
    EXPECT_STREQ("two", tokenizer.NextToken().GetBuffer());
    EXPECT_STREQ("three", tokenizer.NextToken().GetBuffer());
}

TEST_F(StringTokenizerNextTokenTests, NextToken_MultiCharDelimiter) {
    StringTokenizer tokenizer("one::two::three", "::");

    EXPECT_STREQ("one", tokenizer.NextToken().GetBuffer());
    EXPECT_STREQ("two", tokenizer.NextToken().GetBuffer());
    EXPECT_STREQ("three", tokenizer.NextToken().GetBuffer());
}

TEST_F(StringTokenizerNextTokenTests, NextToken_SingleToken) {
    StringTokenizer tokenizer("onlyOneToken", ",");

    EXPECT_STREQ("onlyOneToken", tokenizer.NextToken().GetBuffer());
    EXPECT_FALSE(tokenizer.HasMoreTokens());
}

TEST_F(StringTokenizerNextTokenTests, NextIntToken_ParsesIntegers) {
    StringTokenizer tokenizer("10,20,30", ",");

    EXPECT_EQ(10, tokenizer.NextIntToken());
    EXPECT_EQ(20, tokenizer.NextIntToken());
    EXPECT_EQ(30, tokenizer.NextIntToken());
}

TEST_F(StringTokenizerNextTokenTests, NextFloatToken_ParsesFloats) {
    StringTokenizer tokenizer("3.14,2.5,1.0", ",");

    EXPECT_TRUE(FloatEquals(3.14, tokenizer.NextFloatToken()));
    EXPECT_TRUE(FloatEquals(2.5, tokenizer.NextFloatToken()));
    EXPECT_TRUE(FloatEquals(1.0, tokenizer.NextFloatToken()));
}

TEST_F(StringTokenizerNextTokenTests, NextInt64Token_ParsesLargeIntegers) {
    StringTokenizer tokenizer("1000000000,2000000000", ",");

    EXPECT_EQ(1000000000LL, tokenizer.NextInt64Token());
    EXPECT_EQ(2000000000LL, tokenizer.NextInt64Token());
}

TEST_F(StringTokenizerNextTokenTests, NextToken_WithCustomDelimiter) {
    // Tests NextToken(delimiter) overload
    StringTokenizer tokenizer("a,b;c,d", ",");

    EXPECT_STREQ("a", tokenizer.NextToken().GetBuffer());

    // Switch delimiter mid-stream
    CString token = tokenizer.NextToken(";");
    EXPECT_STREQ("b", token.GetBuffer());
}

TEST_F(StringTokenizerNextTokenTests, HasMoreTokens_UpdatesCorrectly) {
    StringTokenizer tokenizer("a,b", ",");

    EXPECT_TRUE(tokenizer.HasMoreTokens());
    tokenizer.NextToken();
    EXPECT_TRUE(tokenizer.HasMoreTokens());
    tokenizer.NextToken();
    EXPECT_FALSE(tokenizer.HasMoreTokens());
}

TEST_F(StringTokenizerNextTokenTests, RemainingString_UpdatesAfterNextToken) {
    StringTokenizer tokenizer("one,two,three", ",");

    tokenizer.NextToken(); // Consume "one"
    CString remaining = tokenizer.RemainingString();
    EXPECT_STREQ("two,three", remaining.GetBuffer());

    tokenizer.NextToken(); // Consume "two"
    remaining = tokenizer.RemainingString();
    EXPECT_STREQ("three", remaining.GetBuffer());
}

// =============================================================================
// BUG FIX 5: StringTokenizer CountTokens() infinite loop
// =============================================================================

class StringTokenizerCountTokensTests : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(StringTokenizerCountTokensTests, CountTokens_BasicCounting) {
    // BUG: CountTokens() would hang with certain inputs
    // FIX: Rewrote the logic to properly handle loop termination
    StringTokenizer tokenizer("a,b,c", ",");
    EXPECT_EQ(3, tokenizer.CountTokens());
}

TEST_F(StringTokenizerCountTokensTests, CountTokens_SingleToken) {
    StringTokenizer tokenizer("single", ",");
    EXPECT_EQ(1, tokenizer.CountTokens());
}

TEST_F(StringTokenizerCountTokensTests, CountTokens_EmptyString) {
    StringTokenizer tokenizer("", ",");
    EXPECT_EQ(0, tokenizer.CountTokens());
}

TEST_F(StringTokenizerCountTokensTests, CountTokens_ManyTokens) {
    StringTokenizer tokenizer("1,2,3,4,5,6,7,8,9,10", ",");
    EXPECT_EQ(10, tokenizer.CountTokens());
}

TEST_F(StringTokenizerCountTokensTests, CountTokens_MultiCharDelimiter) {
    StringTokenizer tokenizer("one::two::three", "::");
    EXPECT_EQ(3, tokenizer.CountTokens());
}

TEST_F(StringTokenizerCountTokensTests, CountTokens_DoesNotConsumeTokens) {
    StringTokenizer tokenizer("a,b,c", ",");

    EXPECT_EQ(3, tokenizer.CountTokens());
    EXPECT_EQ(3, tokenizer.CountTokens()); // Should still be 3
    EXPECT_TRUE(tokenizer.HasMoreTokens()); // Tokens not consumed
}

TEST_F(StringTokenizerCountTokensTests, CountTokens_MatchesActualTokens) {
    // Verify count matches what we actually get from NextToken
    StringTokenizer tokenizer1("a,b,c,d,e", ",");
    int count = tokenizer1.CountTokens();

    StringTokenizer tokenizer2("a,b,c,d,e", ",");
    int actualTokens = 0;
    while(tokenizer2.HasMoreTokens()) {
        tokenizer2.NextToken();
        actualTokens++;
    }

    EXPECT_EQ(count, actualTokens);
}

// =============================================================================
// Integration Tests - Multiple Bugs Fixed Together
// =============================================================================

class IntegrationTests : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

TEST_F(IntegrationTests, TokenizeAndSort_WorksTogether) {
    // Use fixed StringTokenizer and fixed comparison operators together
    StringTokenizer tokenizer("Zebra,Apple,Mango,Banana", ",");

    std::vector<CString> fruits;
    while(tokenizer.HasMoreTokens()) {
        fruits.push_back(tokenizer.NextToken());
    }

    std::sort(fruits.begin(), fruits.end());

    EXPECT_STREQ("Apple", fruits[0].GetBuffer());
    EXPECT_STREQ("Banana", fruits[1].GetBuffer());
    EXPECT_STREQ("Mango", fruits[2].GetBuffer());
    EXPECT_STREQ("Zebra", fruits[3].GetBuffer());
}

TEST_F(IntegrationTests, ParseBooleans_CaseInsensitive) {
    StringTokenizer tokenizer("true,FALSE,True,false", ",");

    EXPECT_TRUE(tokenizer.NextToken().ToBool());   // "true"
    EXPECT_FALSE(tokenizer.NextToken().ToBool());  // "FALSE"
    EXPECT_TRUE(tokenizer.NextToken().ToBool());   // "True" (was broken)
    EXPECT_FALSE(tokenizer.NextToken().ToBool());  // "false"
}

TEST_F(IntegrationTests, FilterFilesByExtension) {
    // Real-world scenario: filter files ending with .cpp
    CString file1("main.cpp");
    CString file2("header.h");
    CString file3("test.cpp");
    CString file4("readme.txt");

    EXPECT_TRUE(file1.EndsWith(".cpp"));
    EXPECT_FALSE(file2.EndsWith(".cpp"));
    EXPECT_TRUE(file3.EndsWith(".cpp"));
    EXPECT_FALSE(file4.EndsWith(".cpp"));
}
