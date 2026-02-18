#include <gtest/gtest.h>
#include "Digest.h"
#include "MD5.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

using namespace EIBStdLibTest;

namespace {
std::string ToHex(const unsigned char* data, size_t len) {
    static const char* kHex = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(kHex[(data[i] >> 4) & 0x0F]);
        out.push_back(kHex[data[i] & 0x0F]);
    }
    return out;
}
}  // namespace

class DigestMd5Test : public BaseTestFixture {};

TEST_F(DigestMd5Test, MD5Class_KnownVectorAbc) {
    MD5 md5;
    MD5_CTX ctx;
    unsigned char digest[16];
    unsigned char input[] = "abc";

    md5.MD5Init(&ctx);
    md5.MD5Update(&ctx, input, 3);
    md5.MD5Final(digest, &ctx);

    EXPECT_EQ("900150983cd24fb0d6963f7d28e17f72", ToHex(digest, 16));
}

TEST_F(DigestMd5Test, CDigest_HashStringKnownVectors) {
    CDigest digest(ALGORITHM_MD5);
    CString hash1;
    CString hash2;

    digest.HashString("abc", hash1);
    digest.HashString("", hash2);

    EXPECT_STREQ("900150983cd24fb0d6963f7d28e17f72", hash1.GetBuffer());
    EXPECT_STREQ("d41d8cd98f00b204e9800998ecf8427e", hash2.GetBuffer());
}

TEST_F(DigestMd5Test, CDigest_HashFileMatchesExpected) {
    char path[] = "/tmp/eibstdlib_md5_test_XXXXXX";
    int fd = mkstemp(path);
    ASSERT_NE(-1, fd);

    const char* content = "hello";
    ASSERT_EQ(static_cast<ssize_t>(std::strlen(content)),
              write(fd, content, std::strlen(content)));
    close(fd);

    CDigest digest(ALGORITHM_MD5);
    CString hash;
    digest.HashFile(path, hash);
    EXPECT_STREQ("5d41402abc4b2a76b9719d911017c592", hash.GetBuffer());

    std::remove(path);
}

TEST_F(DigestMd5Test, CDigest_HashFileMissingFileThrows) {
    CDigest digest(ALGORITHM_MD5);
    CString hash;
    EXPECT_THROW(digest.HashFile("/tmp/this_file_should_not_exist_12345", hash), CEIBException);
}

TEST_F(DigestMd5Test, CDigest_Base64DecodeWorks) {
    CDigest digest(ALGORITHM_BASE64);
    CString out;
    EXPECT_TRUE(digest.Decode("SGVsbG8=", out));
    EXPECT_STREQ("Hello", out.GetBuffer());
}

TEST_F(DigestMd5Test, CDigest_MD5DecodeThrows) {
    CDigest digest(ALGORITHM_MD5);
    CString out;
    EXPECT_THROW(digest.Decode("irrelevant", out), CEIBException);
}
