#include <gtest/gtest.h>
#include "URLEncoding.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

class URLEncodingTest : public BaseTestFixture {};

TEST_F(URLEncodingTest, Encode_LeavesAlphaNumericUnchanged) {
    CString encoded = URLEncoder::Encode("AbC123xyz");
    EXPECT_STREQ("AbC123xyz", encoded.GetBuffer());
}

TEST_F(URLEncodingTest, Encode_EncodesSpaceAndReservedCharacters) {
    CString encoded = URLEncoder::Encode("A+B C/?");
    EXPECT_STREQ("A%2bB+C%2f%3f", encoded.GetBuffer());
}

TEST_F(URLEncodingTest, Decode_ConvertsPlusAndPercentEscapes) {
    CString decoded = URLEncoder::Decode("Hello+World%21");
    EXPECT_STREQ("Hello World!", decoded.GetBuffer());
}

TEST_F(URLEncodingTest, Decode_HandlesUppercaseHexDigits) {
    CString decoded = URLEncoder::Decode("%2B%2F%3A");
    EXPECT_STREQ("+/:", decoded.GetBuffer());
}

TEST_F(URLEncodingTest, EncodeThenDecode_RoundTripsText) {
    CString original("message with symbols: +/?=ok");
    CString encoded = URLEncoder::Encode(original);
    CString decoded = URLEncoder::Decode(encoded);

    EXPECT_STREQ(original.GetBuffer(), decoded.GetBuffer());
}
