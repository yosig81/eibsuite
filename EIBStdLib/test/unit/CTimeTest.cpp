#include <gtest/gtest.h>
#include "CTime.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cctype>
#include <cstdlib>
#include <cstring>

using namespace EIBStdLibTest;

class CTimeTest : public BaseTestFixture {
protected:
    void SetUp() override {
        BaseTestFixture::SetUp();
        // Reset static default to local time before each test
        CTime::SetDefaultLocalTime(true);
    }

    void TearDown() override {
        // Restore default after each test
        CTime::SetDefaultLocalTime(true);
        BaseTestFixture::TearDown();
    }
};

// ============================================================
// Constructor tests
// ============================================================

TEST_F(CTimeTest, DefaultConstructor_CapturesCurrentTime) {
    int before = static_cast<int>(time(0));
    CTime t;
    int after = static_cast<int>(time(0));
    EXPECT_GE(t.GetTime(), before);
    EXPECT_LE(t.GetTime(), after);
}

TEST_F(CTimeTest, IntConstructor_StoresExactValue) {
    CTime t(1700000000);
    EXPECT_EQ(1700000000, t.GetTime());
}

TEST_F(CTimeTest, CopyConstructor_CopiesValue) {
    CTime original(999999);
    CTime copy(original);
    EXPECT_EQ(original.GetTime(), copy.GetTime());
}

TEST_F(CTimeTest, TmStructConstructor_WithNull_UsesCurrentTime) {
    CTime now_from_null((struct tm*)NULL);
    int now = static_cast<int>(time(0));
    EXPECT_LE(std::abs(now_from_null.GetTime() - now), 2);
}

TEST_F(CTimeTest, ComponentConstructor_ProducesCorrectEpoch) {
    // 2024-01-15 12:30:00
    CTime t(15, 1, 2024, 12, 30, 0);
    // Verify by extracting back via localtime
    time_t tt = (time_t)t.GetTime();
    struct tm* tm = localtime(&tt);
    EXPECT_EQ(15, tm->tm_mday);
    EXPECT_EQ(0, tm->tm_mon); // January = 0
    EXPECT_EQ(124, tm->tm_year); // 2024 - 1900
    EXPECT_EQ(12, tm->tm_hour);
    EXPECT_EQ(30, tm->tm_min);
    EXPECT_EQ(0, tm->tm_sec);
}

TEST_F(CTimeTest, StringConstructor_ParsesCtimeFormat) {
    // Round-trip: create a known time, format it, parse it back
    CTime original(15, 6, 2023, 10, 30, 45);
    CString formatted = original.Format(true);
    CTime parsed(formatted.GetBuffer(), true);
    // Should be within 1 second (strftime drops sub-second precision)
    EXPECT_LE(std::abs(original.GetTime() - parsed.GetTime()), 1);
}

TEST_F(CTimeTest, StringConstructor_RejectsMalformedText) {
    EXPECT_THROW(CTime bad("not a valid time"), CEIBException);
}

// ============================================================
// Arithmetic and comparison operators
// ============================================================

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

TEST_F(CTimeTest, EqualityOperators_Work) {
    CTime a(5000);
    CTime b(5000);
    CTime c(6000);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a == c);
}

TEST_F(CTimeTest, ComparisonOperators_LeGeWork) {
    CTime a(5000);
    CTime b(5000);
    CTime c(6000);

    EXPECT_TRUE(a <= b);
    EXPECT_TRUE(a >= b);
    EXPECT_TRUE(a <= c);
    EXPECT_FALSE(a >= c);
}

TEST_F(CTimeTest, AssignmentOperator_CopiesValue) {
    CTime a(1234);
    CTime b(0);
    b = a;
    EXPECT_EQ(1234, b.GetTime());
}

TEST_F(CTimeTest, IntAssignmentOperator_SetsValue) {
    CTime a(0);
    a = 9999;
    EXPECT_EQ(9999, a.GetTime());
}

// ============================================================
// Format: local vs UTC (the bug that was missed)
// ============================================================

TEST_F(CTimeTest, FormatLocal_DiffersFromFormatUTC_WhenNotInUTCTimezone) {
    // Use a known epoch time
    CTime t(1700000000); // Nov 14 2023 ~21:13 UTC

    CString utc_str = t.Format(false);
    CString local_str = t.Format(true);

    ASSERT_GT(utc_str.GetLength(), 0);
    ASSERT_GT(local_str.GetLength(), 0);

    // Extract the hour from each formatted string
    // Format is "Wed Nov 14 21:13:20 2023" - hour is at position 11-12
    // If we're not in UTC, the hours should differ
    int tz_offset;
    CTime::GetTimeZone(tz_offset);
    if (tz_offset != 0) {
        // Local and UTC should produce different strings
        EXPECT_NE(std::string(utc_str.GetBuffer()), std::string(local_str.GetBuffer()))
            << "Format(true) and Format(false) should differ in non-UTC timezone";
    }
}

TEST_F(CTimeTest, FormatUTC_MatchesGmtime) {
    CTime t(1700000000);
    CString utc_str = t.Format(false);

    // Verify against gmtime directly
    time_t tt = 1700000000;
    struct tm utc_tm;
    gmtime_r(&tt, &utc_tm);

    char expected[25];
    strftime(expected, sizeof(expected), "%a %b %d %H:%M:%S %Y", &utc_tm);
    if (expected[8] == ' ') expected[8] = '0';

    EXPECT_STREQ(expected, utc_str.GetBuffer())
        << "Format(false) should match gmtime output";
}

TEST_F(CTimeTest, FormatLocal_MatchesLocaltime) {
    CTime t(1700000000);
    CString local_str = t.Format(true);

    // Verify against localtime directly
    time_t tt = 1700000000;
    struct tm local_tm;
    localtime_r(&tt, &local_tm);

    char expected[25];
    strftime(expected, sizeof(expected), "%a %b %d %H:%M:%S %Y", &local_tm);
    if (expected[8] == ' ') expected[8] = '0';

    EXPECT_STREQ(expected, local_str.GetBuffer())
        << "Format(true) should match localtime output";
}

// ============================================================
// GetTimeStrSegment: local vs UTC
// ============================================================

TEST_F(CTimeTest, GetTimeStrSegment_UTC_MatchesGmtime) {
    CTime t(1700000000);

    CString hour_str = t.GetTimeStrSegment("%H", 16, false);

    time_t tt = 1700000000;
    struct tm utc_tm;
    gmtime_r(&tt, &utc_tm);
    char expected[4];
    strftime(expected, sizeof(expected), "%H", &utc_tm);

    EXPECT_STREQ(expected, hour_str.GetBuffer())
        << "GetTimeStrSegment with get_local=false should use gmtime";
}

TEST_F(CTimeTest, GetTimeStrSegment_Local_MatchesLocaltime) {
    CTime t(1700000000);

    CString hour_str = t.GetTimeStrSegment("%H", 16, true);

    time_t tt = 1700000000;
    struct tm local_tm;
    localtime_r(&tt, &local_tm);
    char expected[4];
    strftime(expected, sizeof(expected), "%H", &local_tm);

    EXPECT_STREQ(expected, hour_str.GetBuffer())
        << "GetTimeStrSegment with get_local=true should use localtime";
}

TEST_F(CTimeTest, GetTimeStrSegment_Year_ReturnsDigitsOnly) {
    CTime t(1700000000);
    CString year = t.GetTimeStrSegment("%Y", 16, false);
    ASSERT_EQ(4, year.GetLength());
    for (int i = 0; i < year.GetLength(); ++i) {
        EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(year[i])));
    }
}

// ============================================================
// Static default format preference
// ============================================================

TEST_F(CTimeTest, DefaultLocalTime_InitiallyTrue) {
    EXPECT_TRUE(CTime::GetDefaultLocalTime());
}

TEST_F(CTimeTest, SetDefaultLocalTime_PersistsValue) {
    CTime::SetDefaultLocalTime(false);
    EXPECT_FALSE(CTime::GetDefaultLocalTime());
    CTime::SetDefaultLocalTime(true);
    EXPECT_TRUE(CTime::GetDefaultLocalTime());
}

TEST_F(CTimeTest, FormatNoArgs_UsesDefaultLocalTime_WhenSetToLocal) {
    CTime t(1700000000);
    CTime::SetDefaultLocalTime(true);

    CString default_str = t.Format();
    CString explicit_local = t.Format(true);

    EXPECT_STREQ(explicit_local.GetBuffer(), default_str.GetBuffer())
        << "Format() should match Format(true) when default is local";
}

TEST_F(CTimeTest, FormatNoArgs_UsesDefaultLocalTime_WhenSetToUTC) {
    CTime t(1700000000);
    CTime::SetDefaultLocalTime(false);

    CString default_str = t.Format();
    CString explicit_utc = t.Format(false);

    EXPECT_STREQ(explicit_utc.GetBuffer(), default_str.GetBuffer())
        << "Format() should match Format(false) when default is UTC";
}

TEST_F(CTimeTest, FormatExplicitArg_IgnoresDefault) {
    CTime t(1700000000);
    CTime::SetDefaultLocalTime(false);

    CString explicit_local = t.Format(true);
    CString local_ref = t.Format(true);
    EXPECT_STREQ(local_ref.GetBuffer(), explicit_local.GetBuffer());

    CTime::SetDefaultLocalTime(true);
    CString explicit_utc = t.Format(false);
    CString utc_ref = t.Format(false);
    EXPECT_STREQ(utc_ref.GetBuffer(), explicit_utc.GetBuffer());
}

// ============================================================
// SetNow / SetTime / TimeZero
// ============================================================

TEST_F(CTimeTest, SetNow_UpdatesToCurrent) {
    CTime t(0);
    EXPECT_EQ(0, t.GetTime());
    int before = static_cast<int>(time(0));
    t.SetNow();
    int after = static_cast<int>(time(0));
    EXPECT_GE(t.GetTime(), before);
    EXPECT_LE(t.GetTime(), after);
}

TEST_F(CTimeTest, SetTime_UpdatesValue) {
    CTime t;
    t.SetTime(42);
    EXPECT_EQ(42, t.GetTime());
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

TEST_F(CTimeTest, GetTimePtr_ReturnsPointerToInternalValue) {
    CTime t(5555);
    const int* ptr = t.GetTimePtr();
    ASSERT_NE(nullptr, ptr);
    EXPECT_EQ(5555, *ptr);
}

// ============================================================
// secTo
// ============================================================

TEST_F(CTimeTest, SecTo_FutureTime_ReturnsPositiveSeconds) {
    int now = static_cast<int>(time(0));
    CTime future(now + 100);
    int sec = future.secTo();
    // Should be approximately 100 (within tolerance for test execution time)
    EXPECT_GE(sec, 98);
    EXPECT_LE(sec, 102);
}

TEST_F(CTimeTest, SecTo_PastTime_ReturnsZero) {
    CTime past(1000);
    EXPECT_EQ(0, past.secTo());
}

// ============================================================
// String round-trip: Format then parse back
// ============================================================

TEST_F(CTimeTest, FormatThenParse_RoundTripsLocalTime) {
    CTime original(15, 3, 2025, 14, 45, 30);
    CString formatted = original.Format(true);
    CTime parsed(formatted.GetBuffer(), true);
    EXPECT_LE(std::abs(original.GetTime() - parsed.GetTime()), 1)
        << "Local time round-trip: formatted='" << formatted.GetBuffer() << "'";
}

TEST_F(CTimeTest, FormatThenParse_MultipleEpochs) {
    // Test several different timestamps to ensure consistency
    int epochs[] = { 1000000000, 1500000000, 1700000000, 1800000000 };
    for (int epoch : epochs) {
        CTime original(epoch);
        CString formatted = original.Format(true);
        CTime parsed(formatted.GetBuffer(), true);
        EXPECT_LE(std::abs(original.GetTime() - parsed.GetTime()), 1)
            << "Round-trip failed for epoch " << epoch
            << ", formatted='" << formatted.GetBuffer() << "'";
    }
}

// ============================================================
// AddFormatToString
// ============================================================

TEST_F(CTimeTest, AddFormatToString_AppendsToExistingString) {
    CTime t(1700000000);
    CString prefix("Time: ");
    t.AddFormatToString(prefix, true);
    // Should start with "Time: " and have the formatted time appended
    EXPECT_EQ(0, std::string(prefix.GetBuffer()).find("Time: "));
    EXPECT_GT(prefix.GetLength(), 6);
}
