// GenerateIndicationsTest.cpp -- Tests the random indication generation feature.
//
// Verifies that EmulatorGenerateRandomIndications sends indications that
// are received by the server and visible in the bus monitor / history.

#include "IntegrationHelpers.h"

using namespace IntegrationTest;

class GenerateIndicationsTest : public ::testing::Test {
protected:
    HttpTestClient http;
    CString admin_sid;

    void SetUp() override {
        admin_sid = http.Login("admin", "admin123");
        ASSERT_GT(admin_sid.GetLength(), 0) << "Admin login failed";
    }
};

TEST_F(GenerateIndicationsTest, GeneratedIndicationsAppearInHistory)
{
    // Get baseline history
    HttpResponse before = http.Get("/api/history", admin_sid);
    ASSERT_EQ(before.status_code, 200);
    int body_len_before = before.body.GetLength();

    // Generate 5 random indications with no delay
    EmulatorGenerateRandomIndications(5, 0);

    // Poll until history grows.  The indications are processed asynchronously
    // by the output handler (each needing an ACK round-trip), so allow enough
    // time for all 5 to complete when running after other tests.
    bool grew = false;
    for (int elapsed = 0; elapsed < 5000; elapsed += 20) {
        HttpResponse after = http.Get("/api/history", admin_sid);
        if (after.status_code == 200 && after.body.GetLength() > body_len_before) {
            grew = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_TRUE(grew) << "History should contain new entries after generating indications";
}

TEST_F(GenerateIndicationsTest, BurstModeDoesNotCrash)
{
    // Send a burst of indications with no delay -- validates that the
    // output handler queue absorbs the burst without crashes.
    EmulatorGenerateRandomIndications(20, 0);

    // Poll until server responds (proves it didn't crash)
    bool responsive = false;
    for (int elapsed = 0; elapsed < 5000; elapsed += 20) {
        HttpResponse resp = http.Get("/api/history", admin_sid);
        if (resp.status_code == 200) {
            responsive = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    EXPECT_TRUE(responsive) << "Server should still be responsive after burst";
}
