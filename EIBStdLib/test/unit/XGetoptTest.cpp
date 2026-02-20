#include <gtest/gtest.h>
#include "XGetopt.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

#if defined(__APPLE__)
extern int optreset;
#endif

namespace {
void ResetGetoptState() {
#if defined(__APPLE__)
    optreset = 1;
    optind = 1;
#else
    optind = 0;  // GNU getopt requires 0 for a full reset
#endif
    opterr = 0;
    optarg = NULL;
}
}  // namespace

class XGetoptTest : public BaseTestFixture {};

TEST_F(XGetoptTest, ParsesFlagsAndArgument) {
    char arg0[] = "prog";
    char arg1[] = "-a";
    char arg2[] = "-b";
    char arg3[] = "value";
    char arg4[] = "tail";
    char* argv[] = {arg0, arg1, arg2, arg3, arg4, NULL};
    int argc = 5;
    char opts[] = "ab:";

    ResetGetoptState();

    int first = getopt(argc, argv, opts);
    EXPECT_EQ('a', first);
    EXPECT_EQ(nullptr, optarg);

    int second = getopt(argc, argv, opts);
    EXPECT_EQ('b', second);
    ASSERT_NE(nullptr, optarg);
    EXPECT_STREQ("value", optarg);

    int done = getopt(argc, argv, opts);
    EXPECT_EQ(-1, done);
    ASSERT_LT(optind, argc);
    EXPECT_STREQ("tail", argv[optind]);
}

TEST_F(XGetoptTest, UnknownOption_ReturnsQuestionMark) {
    char arg0[] = "prog";
    char arg1[] = "-z";
    char* argv[] = {arg0, arg1, NULL};
    int argc = 2;
    char opts[] = "ab:";

    ResetGetoptState();

    EXPECT_EQ('?', getopt(argc, argv, opts));
}

TEST_F(XGetoptTest, DoubleDash_StopsOptionParsing) {
    char arg0[] = "prog";
    char arg1[] = "-a";
    char arg2[] = "--";
    char arg3[] = "-b";
    char* argv[] = {arg0, arg1, arg2, arg3, NULL};
    int argc = 4;
    char opts[] = "ab:";

    ResetGetoptState();

    EXPECT_EQ('a', getopt(argc, argv, opts));
    EXPECT_EQ(-1, getopt(argc, argv, opts));
    ASSERT_LT(optind, argc);
    EXPECT_STREQ("-b", argv[optind]);
}
