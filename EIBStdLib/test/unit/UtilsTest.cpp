#include <gtest/gtest.h>
#include "Utils.h"
#include "Globals.h"
#include "../fixtures/TestHelpers.h"
#include <cctype>
#include <unistd.h>

using namespace EIBStdLibTest;

namespace {
CString MakeTempFilePath(const char* pattern) {
    char temp_template[64] = {0};
    strncpy(temp_template, pattern, sizeof(temp_template) - 1);
    int fd = mkstemp(temp_template);
    EXPECT_NE(-1, fd);
    if (fd != -1) {
        close(fd);
        unlink(temp_template);
    }
    return CString(temp_template);
}
}  // namespace

class UtilsTest : public BaseTestFixture {};

TEST_F(UtilsTest, SaveReadAndGetSize_RoundTrip) {
    CString path = MakeTempFilePath("/tmp/eib_utils_test_XXXXXX");
    CString content("alpha\nbeta");

    ASSERT_NO_THROW(CUtils::SaveFile(path, content));
    EXPECT_EQ(content.GetLength(), CUtils::GetFileSize(path));

    CString loaded;
    CUtils::ReadFile(path, loaded);
    EXPECT_NE(string::npos, loaded.Find("alpha"));
    EXPECT_NE(string::npos, loaded.Find("beta"));

    unlink(path.GetBuffer());
}

TEST_F(UtilsTest, ReadFile_MissingFileReturnsEmptyString) {
    CString path = MakeTempFilePath("/tmp/eib_utils_missing_XXXXXX");

    CString loaded("not-empty");
    CUtils::ReadFile(path, loaded);
    EXPECT_TRUE(loaded.IsEmpty());
}

TEST_F(UtilsTest, GetCurrentPath_AppendsPathDelimiter) {
    CString current;
    CUtils::GetCurrentPath(current);

    EXPECT_GT(current.GetLength(), 0);
    EXPECT_TRUE(current.EndsWith(PATH_DELIM));
}

TEST_F(UtilsTest, GetTimeStrForFile_HasExpectedLayout) {
    CString value;
    CUtils::GetTimeStrForFile(value);

    ASSERT_EQ(12, value.GetLength());
    EXPECT_EQ('_', value[3]);
    EXPECT_EQ('_', value[6]);
    EXPECT_EQ('_', value[9]);

    EXPECT_TRUE(std::isalpha(static_cast<unsigned char>(value[0])) != 0);
    EXPECT_TRUE(std::isalpha(static_cast<unsigned char>(value[1])) != 0);
    EXPECT_TRUE(std::isalpha(static_cast<unsigned char>(value[2])) != 0);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(value[4])) != 0);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(value[5])) != 0);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(value[7])) != 0);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(value[8])) != 0);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(value[10])) != 0);
    EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(value[11])) != 0);
}

TEST_F(UtilsTest, SaveReadAndGetSize_PathWithSpaces) {
    char dir_template[] = "/tmp/eib_utils_space_parent_XXXXXX";
    char* created = mkdtemp(dir_template);
    ASSERT_NE(nullptr, created);
    CString parent(created);

    CString spaced_file = parent + "/file with spaces.txt";
    CString content("line one\nline two");

    ASSERT_NO_THROW(CUtils::SaveFile(spaced_file, content));
    EXPECT_EQ(content.GetLength(), CUtils::GetFileSize(spaced_file));

    CString loaded;
    CUtils::ReadFile(spaced_file, loaded);
    EXPECT_NE(string::npos, loaded.Find("line one"));
    EXPECT_NE(string::npos, loaded.Find("line two"));

    unlink(spaced_file.GetBuffer());
    rmdir(parent.GetBuffer());
}
