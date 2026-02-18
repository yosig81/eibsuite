#include <gtest/gtest.h>
#include "DataBuffer.h"
#include "CException.h"
#include "../fixtures/TestHelpers.h"
#include <cstring>
#include <string>

using namespace EIBStdLibTest;

class DataBufferTest : public BaseTestFixture {};

TEST_F(DataBufferTest, DefaultConstructor_InitialState) {
    CDataBuffer buffer;
    EXPECT_EQ(0, buffer.GetLength());
    EXPECT_EQ(DATA_BUFFER_DEFAULT_SIZE, buffer.GetAllocated());
}

TEST_F(DataBufferTest, AddAndOperators_AppendsDataInOrder) {
    CDataBuffer buffer;
    buffer.Add('A');
    buffer += "BC";
    buffer += CString("DE");

    EXPECT_EQ(5, buffer.GetLength());
    EXPECT_EQ('A', buffer[0]);
    EXPECT_EQ('B', buffer[1]);
    EXPECT_EQ('C', buffer[2]);
    EXPECT_EQ('D', buffer[3]);
    EXPECT_EQ('E', buffer[4]);
}

TEST_F(DataBufferTest, AddBinaryData_PreservesBytes) {
    const unsigned char raw[] = {0x00, 0x01, 0x7f, 0xff};
    CDataBuffer buffer;
    buffer.Add(raw, 4);

    ASSERT_EQ(4, buffer.GetLength());
    const unsigned char* out = buffer.GetBuffer();
    EXPECT_EQ(0x00, out[0]);
    EXPECT_EQ(0x01, out[1]);
    EXPECT_EQ(0x7f, out[2]);
    EXPECT_EQ(0xff, out[3]);
}

TEST_F(DataBufferTest, Reallocate_PreservesContent) {
    CDataBuffer buffer;
    std::string large(300, 'x');
    buffer.Add(large.c_str(), static_cast<int>(large.size()));

    EXPECT_EQ(300, buffer.GetLength());
    EXPECT_GT(buffer.GetAllocated(), DATA_BUFFER_DEFAULT_SIZE);
    EXPECT_EQ('x', buffer[0]);
    EXPECT_EQ('x', buffer[299]);
}

TEST_F(DataBufferTest, EncryptDecrypt_RoundTripBufferContent) {
    CDataBuffer buffer;
    CString key("secret");
    const char* payload = "hello world";
    buffer.Add(payload, static_cast<int>(std::strlen(payload)));

    std::string before(reinterpret_cast<const char*>(buffer.GetBuffer()), buffer.GetLength());
    buffer.Encrypt(&key);
    std::string encrypted(reinterpret_cast<const char*>(buffer.GetBuffer()), buffer.GetLength());
    EXPECT_NE(before, encrypted);

    buffer.Decrypt(&key);
    std::string after(reinterpret_cast<const char*>(buffer.GetBuffer()), buffer.GetLength());
    EXPECT_EQ(before, after);
}

TEST_F(DataBufferTest, StaticEncryptDecrypt_RoundTripBufferContent) {
    CString key("key");
    char payload[] = "payload";
    const size_t len = std::strlen(payload);
    std::string before(payload, len);

    CDataBuffer::Encrypt(payload, static_cast<int>(len), &key);
    std::string encrypted(payload, len);
    EXPECT_NE(before, encrypted);

    CDataBuffer::Decrypt(payload, static_cast<int>(len), &key);
    std::string after(payload, len);
    EXPECT_EQ(before, after);
}

TEST_F(DataBufferTest, Read_AdvancesCursorAndReturnsSequentialData) {
    CDataBuffer buffer;
    buffer.Add("abcd", 4);

    char first[2];
    char second[2];
    buffer.Read(first, 2);
    buffer.Read(second, 2);

    EXPECT_EQ('a', first[0]);
    EXPECT_EQ('b', first[1]);
    EXPECT_EQ('c', second[0]);
    EXPECT_EQ('d', second[1]);
}

TEST_F(DataBufferTest, Read_BeyondLengthThrows) {
    CDataBuffer buffer;
    buffer.Add("ab", 2);
    char out[3];
    EXPECT_THROW(buffer.Read(out, 3), CEIBException);
}
