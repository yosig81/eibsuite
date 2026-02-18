#include <gtest/gtest.h>
#include "Directory.h"
#include "CString.h"
#include "../fixtures/TestHelpers.h"
#include <limits.h>
#include <unistd.h>

using namespace EIBStdLibTest;

class DirectoryTest : public BaseTestFixture {};

TEST_F(DirectoryTest, CreateExistDelete_RoundTrip) {
    char temp_template[] = "/tmp/eib_dir_test_XXXXXX";
    char* created = mkdtemp(temp_template);
    ASSERT_NE(nullptr, created);

    CString path(created);
    ASSERT_TRUE(CDirectory::IsExist(path));
    ASSERT_TRUE(CDirectory::Delete(path));
    EXPECT_FALSE(CDirectory::IsExist(path));

    EXPECT_TRUE(CDirectory::Create(path));
    EXPECT_TRUE(CDirectory::IsExist(path));

    EXPECT_TRUE(CDirectory::Delete(path));
    EXPECT_FALSE(CDirectory::IsExist(path));
}

TEST_F(DirectoryTest, ChangeAndCurrentDirectory_RoundTrip) {
    CString original = CDirectory::CurrentDirectory();

    char temp_template[] = "/tmp/eib_dir_change_test_XXXXXX";
    char* created = mkdtemp(temp_template);
    ASSERT_NE(nullptr, created);

    CString path(created);
    ASSERT_TRUE(CDirectory::Change(path));

    char expected_real[PATH_MAX] = {0};
    char actual_real[PATH_MAX] = {0};
    ASSERT_NE(nullptr, realpath(path.GetBuffer(), expected_real));
    ASSERT_NE(nullptr, realpath(CDirectory::CurrentDirectory().GetBuffer(), actual_real));
    EXPECT_STREQ(expected_real, actual_real);

    ASSERT_TRUE(CDirectory::Change(original));
    EXPECT_STREQ(original.GetBuffer(), CDirectory::CurrentDirectory().GetBuffer());

    EXPECT_TRUE(CDirectory::Delete(path));
}

TEST_F(DirectoryTest, CreateExistingDirectory_ReturnsFalse) {
    char temp_template[] = "/tmp/eib_dir_exists_test_XXXXXX";
    char* created = mkdtemp(temp_template);
    ASSERT_NE(nullptr, created);

    CString path(created);
    EXPECT_FALSE(CDirectory::Create(path));

    EXPECT_TRUE(CDirectory::Delete(path));
}

TEST_F(DirectoryTest, DirectoryWithSpaces_CreateChangeDelete) {
    CString original = CDirectory::CurrentDirectory();

    char parent_template[] = "/tmp/eib_dir_space_parent_XXXXXX";
    char* parent_created = mkdtemp(parent_template);
    ASSERT_NE(nullptr, parent_created);
    CString parent(parent_created);

    CString spaced_dir = parent + "/folder with spaces";
    EXPECT_TRUE(CDirectory::Create(spaced_dir));
    EXPECT_TRUE(CDirectory::IsExist(spaced_dir));

    ASSERT_TRUE(CDirectory::Change(spaced_dir));

    char expected_real[PATH_MAX] = {0};
    char actual_real[PATH_MAX] = {0};
    ASSERT_NE(nullptr, realpath(spaced_dir.GetBuffer(), expected_real));
    ASSERT_NE(nullptr, realpath(CDirectory::CurrentDirectory().GetBuffer(), actual_real));
    EXPECT_STREQ(expected_real, actual_real);

    ASSERT_TRUE(CDirectory::Change(original));
    EXPECT_TRUE(CDirectory::Delete(spaced_dir));
    EXPECT_TRUE(CDirectory::Delete(parent));
}
