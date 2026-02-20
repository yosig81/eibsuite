#include <gtest/gtest.h>
#include "CommandScheduler.h"
#include "CTime.h"
#include "../fixtures/TestHelpers.h"

// ---------------------------------------------------------------------------
// ScheduledCommandComparison -- standalone functor defined in the header
// ---------------------------------------------------------------------------

TEST(ScheduledCommandComparison, EarlierFirst)
{
    // The comparator returns true when lhs._time > rhs._time,
    // which means the priority queue pops the EARLIEST time first (min-heap).
    ScheduledCommandComparison cmp;

    unsigned char val = 0x01;
    CTime early(1000);
    CTime late(2000);

    ScheduledCommand cmd_early(early, CEibAddress(), &val, 1);
    ScheduledCommand cmd_late(late, CEibAddress(), &val, 1);

    // cmp(late, early) should be true  (late > early -> late should be lower priority)
    EXPECT_TRUE(cmp(cmd_late, cmd_early));
    // cmp(early, late) should be false (early < late -> early is higher priority)
    EXPECT_FALSE(cmp(cmd_early, cmd_late));
}

TEST(ScheduledCommandComparison, EqualTimes)
{
    ScheduledCommandComparison cmp;

    unsigned char val = 0x01;
    CTime same(1500);

    ScheduledCommand cmd1(same, CEibAddress(), &val, 1);
    ScheduledCommand cmd2(same, CEibAddress(), &val, 1);

    // Equal times: neither is strictly greater, so both return false
    EXPECT_FALSE(cmp(cmd1, cmd2));
    EXPECT_FALSE(cmp(cmd2, cmd1));
}

// ---------------------------------------------------------------------------
// AddScheduledCommand -- requires JTC init (CCommandScheduler is a JTCThread)
// ---------------------------------------------------------------------------

class CommandSchedulerTest : public ::testing::Test {
protected:
    // Share a single CCommandScheduler across all tests to avoid JTC resource
    // exhaustion (CCommandScheduler inherits JTCThread + JTCMonitor).
    static CCommandScheduler* scheduler_;

    static void SetUpTestSuite() {
        scheduler_ = new CCommandScheduler();
    }
    static void TearDownTestSuite() {
        // Intentionally leak to avoid segfault during static destruction.
        // JTC thread objects crash if destroyed after JTCInitialize goes
        // out of scope; since we're at process exit, the OS reclaims memory.
        scheduler_ = nullptr;
    }
};

CCommandScheduler* CommandSchedulerTest::scheduler_ = nullptr;

TEST_F(CommandSchedulerTest, AddScheduledCommand_FutureTime_Succeeds)
{
    CTime future;
    future.SetNow();
    future += 3600; // 1 hour from now

    unsigned char val[] = { 0x00, 0x80 };
    CString err;
    EXPECT_TRUE(scheduler_->AddScheduledCommand(future, CEibAddress("1/2/3"), val, 2, err));
    EXPECT_STREQ(err.GetBuffer(), "");
}

TEST_F(CommandSchedulerTest, AddScheduledCommand_PastTime_Fails)
{
    CTime past;
    past.SetNow();
    past -= 3600; // 1 hour ago

    unsigned char val[] = { 0x00, 0x80 };
    CString err;
    EXPECT_FALSE(scheduler_->AddScheduledCommand(past, CEibAddress("1/2/3"), val, 2, err));
    EXPECT_TRUE(err.GetLength() > 0);
}

TEST_F(CommandSchedulerTest, AddScheduledCommand_CurrentTime_Fails)
{
    CTime now;
    now.SetNow();

    unsigned char val[] = { 0x00, 0x80 };
    CString err;
    EXPECT_FALSE(scheduler_->AddScheduledCommand(now, CEibAddress("1/2/3"), val, 2, err));
}
