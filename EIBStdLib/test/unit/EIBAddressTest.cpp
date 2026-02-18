#include <gtest/gtest.h>
#include "EIBAddress.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;
using namespace EibStack;

class EIBAddressTest : public BaseTestFixture {};

TEST_F(EIBAddressTest, IndividualAddress_ParsesAndFormats) {
    CEibAddress addr("1.2.3");

    EXPECT_FALSE(addr.IsGroupAddress());
    EXPECT_EQ(1, addr.GetZone());
    EXPECT_EQ(2, addr.GetLine());
    EXPECT_EQ(3, addr.GetDevice());
    EXPECT_STREQ("1.2.3", addr.ToString().GetBuffer());
}

TEST_F(EIBAddressTest, GroupAddress3Level_ParsesAndFormats) {
    CEibAddress addr("1/2/3");

    EXPECT_TRUE(addr.IsGroupAddress());
    EXPECT_EQ(1, addr.GetMainGroup());
    EXPECT_EQ(2, addr.GetMiddleGroup());
    EXPECT_EQ(3, addr.GetSubGroup8());
    EXPECT_STREQ("1/2/3", addr.ToString().GetBuffer());
}

TEST_F(EIBAddressTest, GroupAddress2Level_ParsesAndFormats) {
    CEibAddress addr("1/513");

    EXPECT_TRUE(addr.IsGroupAddress());
    EXPECT_EQ(1, addr.GetMainGroup());
    EXPECT_EQ(513, addr.GetSubGroup11());
    EXPECT_STREQ("1/513", addr.ToString().GetBuffer());
}

TEST_F(EIBAddressTest, RawAddressConstructor_IndividualRoundTrip) {
    CEibAddress addr(static_cast<unsigned int>(0x1234), false);
    EXPECT_STREQ("1.2.52", addr.ToString().GetBuffer());
    EXPECT_EQ(static_cast<unsigned short>(0x1234), addr.ToByteArray());
}

TEST_F(EIBAddressTest, TypeSpecificAccessors_ThrowOnWrongAddressType) {
    CEibAddress individual("1.2.3");
    CEibAddress group("1/2/3");

    EXPECT_THROW(individual.GetMainGroup(), CEIBException);
    EXPECT_THROW(individual.GetSubGroup8(), CEIBException);
    EXPECT_THROW(group.GetZone(), CEIBException);
    EXPECT_THROW(group.GetDevice(), CEIBException);
}

TEST_F(EIBAddressTest, InvalidSyntax_Throws) {
    EXPECT_THROW(CEibAddress("1-2-3"), CEIBException);
    EXPECT_THROW(CEibAddress("1.2"), CEIBException);
    EXPECT_THROW(CEibAddress("1/2/3/4"), CEIBException);
}

TEST_F(EIBAddressTest, ComparisonOperators_WorkAsExpected) {
    CEibAddress a("1.1.1");
    CEibAddress b("1.1.2");
    CEibAddress c("1/1/2");

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(b != c);
}
