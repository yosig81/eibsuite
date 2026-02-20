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

    // Generate 5 random indications with small delay for tunnel acks
    EmulatorGenerateRandomIndications(5, 200);

    // Wait for all indications to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // History should have grown
    HttpResponse after = http.Get("/api/history", admin_sid);
    ASSERT_EQ(after.status_code, 200);
    EXPECT_GT(after.body.GetLength(), body_len_before)
        << "History should contain new entries after generating indications";
}

TEST_F(GenerateIndicationsTest, BurstModeDoesNotCrash)
{
    // Send a burst of indications with no delay -- validates that the
    // output handler queue absorbs the burst without crashes.
    EmulatorGenerateRandomIndications(20, 0);

    // Wait for queue to drain
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    // Server should still be responsive
    HttpResponse resp = http.Get("/api/history", admin_sid);
    EXPECT_EQ(resp.status_code, 200);
}
