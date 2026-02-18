#include <gtest/gtest.h>
#include "DiffieHellman.h"
#include "../fixtures/TestHelpers.h"

using namespace EIBStdLibTest;

class DiffieHellmanTest : public BaseTestFixture {};

TEST_F(DiffieHellmanTest, CreateKeys_IsStableWithinSameInstance) {
    CDiffieHellman dh;
    int64 generator_first = 0;
    int64 modulus_first = 0;
    int64 generator_second = 0;
    int64 modulus_second = 0;

    dh.CreateKeys(generator_first, modulus_first);
    dh.CreateKeys(generator_second, modulus_second);

    EXPECT_GT(generator_first, 0);
    EXPECT_GT(modulus_first, 0);
    EXPECT_LE(generator_first, modulus_first);
    EXPECT_EQ(generator_first, generator_second);
    EXPECT_EQ(modulus_first, modulus_second);
}

TEST_F(DiffieHellmanTest, SenderAndRecipient_DeriveTheSameSharedKey) {
    CDiffieHellman sender;
    CDiffieHellman recipient;

    int64 generator = 0;
    int64 modulus = 0;
    sender.CreateKeys(generator, modulus);

    int64 sender_intermediate = 0;
    int64 recipient_intermediate = 0;
    sender.CreateSenderInterKey(sender_intermediate);
    recipient.CreateRecipientInterKey(recipient_intermediate, generator, modulus);

    int64 sender_key = 0;
    int64 recipient_key = 0;
    sender.CreateSenderEncryptionKey(sender_key, recipient_intermediate);
    recipient.CreateRecipientEncryptionKey(recipient_key, sender_intermediate);

    EXPECT_EQ(sender_intermediate, sender.GetPublicSender());
    EXPECT_EQ(recipient_intermediate, recipient.GetPublicRecipient());
    EXPECT_EQ(sender_key, recipient_key);
    EXPECT_EQ(CString(sender_key), sender.GetSharedKey());
    EXPECT_EQ(CString(recipient_key), recipient.GetSharedKey());

    int64 sender_key_via_get = 0;
    int64 recipient_key_via_get = 0;
    EXPECT_EQ(0, sender.GetValue(sender_key_via_get, KEY));
    EXPECT_EQ(0, recipient.GetValue(recipient_key_via_get, KEY));
    EXPECT_EQ(sender_key, sender_key_via_get);
    EXPECT_EQ(recipient_key, recipient_key_via_get);
}

TEST_F(DiffieHellmanTest, GetValue_ExposesGeneratedParameters) {
    CDiffieHellman dh;
    int64 generator = 0;
    int64 modulus = 0;
    dh.CreateKeys(generator, modulus);

    int64 sender_intermediate = 0;
    dh.CreateSenderInterKey(sender_intermediate);

    int64 got_generator = 0;
    int64 got_modulus = 0;
    int64 got_public_a = 0;
    EXPECT_EQ(0, dh.GetValue(got_generator, GENERATOR));
    EXPECT_EQ(0, dh.GetValue(got_modulus, MODULUS));
    EXPECT_EQ(0, dh.GetValue(got_public_a, PUBLIC_A));

    EXPECT_EQ(generator, got_generator);
    EXPECT_EQ(modulus, got_modulus);
    EXPECT_EQ(sender_intermediate, got_public_a);
}
