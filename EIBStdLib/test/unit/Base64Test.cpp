#include <gtest/gtest.h>
#include "Base64.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

class Base64Test : public BaseTestFixture {};

TEST_F(Base64Test, Decode_ValidInputWithoutPadding) {
    CString decoded;
    EXPECT_TRUE(CBase64::Decode("TWFu", decoded));
    EXPECT_STREQ("Man", decoded.GetBuffer());
}

TEST_F(Base64Test, Decode_ValidInputWithPadding) {
    CString decoded1;
    CString decoded2;

    EXPECT_TRUE(CBase64::Decode("TWE=", decoded1));
    EXPECT_TRUE(CBase64::Decode("TQ==", decoded2));

    EXPECT_STREQ("Ma", decoded1.GetBuffer());
    EXPECT_STREQ("M", decoded2.GetBuffer());
}

TEST_F(Base64Test, Decode_StopsOnInvalidCharacter) {
    CString decoded;
    EXPECT_TRUE(CBase64::Decode("SGVsbG8$junk", decoded));
    EXPECT_STREQ("Hello", decoded.GetBuffer());
}

TEST_F(Base64Test, Decode_EmptyInputProducesEmptyOutput) {
    CString decoded("not empty");
    EXPECT_TRUE(CBase64::Decode("", decoded));
    EXPECT_TRUE(decoded.IsEmpty());
}

TEST_F(Base64Test, Encode_NotImplementedThrows) {
    CString encoded;
    EXPECT_THROW(CBase64::Encode("Hello", encoded), CEIBException);
}
