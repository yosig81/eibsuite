#include <gtest/gtest.h>
#include "StringTokenizer.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

class StringTokenizerTest : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
    }
};

// Basic Tests
TEST_F(StringTokenizerTest, EmptyString_HasNoTokens) {
    StringTokenizer tokenizer("", ",");
    EXPECT_FALSE(tokenizer.HasMoreTokens());
    EXPECT_EQ(0, tokenizer.CountTokens());
}

TEST_F(StringTokenizerTest, HasMoreTokens_TrueForNonEmptyString) {
    StringTokenizer tokenizer("test", ",");
    EXPECT_TRUE(tokenizer.HasMoreTokens());
}

TEST_F(StringTokenizerTest, HasMoreTokens_FalseForEmptyString) {
    StringTokenizer tokenizer("", ",");
    EXPECT_FALSE(tokenizer.HasMoreTokens());
}

TEST_F(StringTokenizerTest, RemainingString_ReturnsProcessedString) {
    StringTokenizer tokenizer("test", ",");

    CString remaining = tokenizer.RemainingString();
    EXPECT_FALSE(remaining.IsEmpty());
    EXPECT_STREQ("test", remaining.GetBuffer());
}

TEST_F(StringTokenizerTest, RemainingString_EmptyForEmptyInput) {
    StringTokenizer tokenizer("", ",");

    CString remaining = tokenizer.RemainingString();
    EXPECT_TRUE(remaining.IsEmpty());
}
