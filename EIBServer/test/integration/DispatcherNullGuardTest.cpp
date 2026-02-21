// DispatcherNullGuardTest.cpp -- Verify Dispatcher handles resource conflicts gracefully.
//
// With the old socket-based dispatcher, Init() would throw when the port
// was already in use.  With cpp-httplib, Init() succeeds (it only creates
// the SSLServer object), but listen() inside Start() silently fails.
// This test verifies that creating a second dispatcher and calling Init()
// + Close() does not crash, even when the port is occupied.

#include "IntegrationHelpers.h"
#include "Dispatcher.h"

using namespace IntegrationTest;

TEST(DispatcherNullGuardTest, SecondDispatcherInitAndCloseDoesNotCrash)
{
    // The integration environment already has the server's Dispatcher
    // bound to port 18080.  Creating a second Dispatcher and calling
    // Init() should succeed (SSLServer created with valid certs), and
    // Close() should clean up without crashing.
    CDispatcher* dispatcher = new CDispatcher();

    // Init should succeed -- cert/key are valid, server object is created
    EXPECT_NO_THROW(dispatcher->Init());

    // Clean up without crashing
    dispatcher->Close();
    delete dispatcher;
}
