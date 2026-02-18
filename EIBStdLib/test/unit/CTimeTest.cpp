#include <gtest/gtest.h>
#include "CTime.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cctype>
#include <cstdlib>

using namespace EIBStdLibTest;

class CTimeTest : public BaseTestFixture {};

TEST_F(CTimeTest, ArithmeticAndComparisonOperators_WorkWithRawEpochValues) {
    CTime base(1000);
    CTime delta(25);

    EXPECT_TRUE(base > delta);
    EXPECT_TRUE(delta < base);

    CTime sum = base + delta;
    EXPECT_EQ(1025, sum.GetTime());

    CTime diff = base - delta;
    EXPECT_EQ(975, diff.GetTime());

    sum -= 5;
    EXPECT_EQ(1020, sum.GetTime());

    diff += 10;
    EXPECT_EQ(985, diff.GetTime());
}

TEST_F(CTimeTest, ParseConstructor_RejectsMalformedText) {
    EXPECT_THROW(CTime bad("not a valid time"), CEIBException);
}

TEST_F(CTimeTest, GetTimeZeroAndSetTimeZero_AreConsistent) {
    CTime value(123456);

    int zero_time = 0;
    EXPECT_NO_THROW(zero_time = value.GetTimeZero());
    EXPECT_GT(zero_time, 0);

    EXPECT_NO_THROW(value.SetTimeZero());
    EXPECT_TRUE(value.IsTimeZero());
    EXPECT_EQ(zero_time, value.GetTime());
}

TEST_F(CTimeTest, FormatAndSegmentApis_ReturnNonEmptyOutput) {
    CTime value(1700000000);

    CString utc = value.Format(false);
    CString local = value.Format(true);

    EXPECT_GT(utc.GetLength(), 0);
    EXPECT_GT(local.GetLength(), 0);

    CString year = value.GetTimeStrSegment("%Y", 16, false);
    ASSERT_EQ(4, year.GetLength());
    for (int i = 0; i < year.GetLength(); ++i) {
        EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(year[i])));
    }
}

TEST_F(CTimeTest, NullTmPointerConstructor_UsesCurrentTime) {
    CTime now_from_null((struct tm*)NULL);
    int now = static_cast<int>(time(0));
    EXPECT_LE(std::abs(now_from_null.GetTime() - now), 2);
}
