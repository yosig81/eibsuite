// DispatcherNullGuardTest.cpp -- Verify Dispatcher handles Init() failure gracefully.
//
// When Dispatcher::Init() fails (e.g. port already in use), _server_sock
// remains NULL.  Previously this caused two problems:
//   1. Init()'s catch block called GetServerPort() which dereferenced NULL.
//   2. run() called _server_sock->Accept() which dereferenced NULL.

#include "IntegrationHelpers.h"
#include "Dispatcher.h"

using namespace IntegrationTest;

TEST(DispatcherNullGuardTest, InitErrorPathDoesNotCrash)
{
    // The integration environment already has the server's Dispatcher
    // bound to port 18080.  Creating a second Dispatcher and calling
    // Init() will fail because the port is already in use.
    //
    // Before the fix, Init()'s catch block called GetServerPort() which
    // dereferenced the NULL _server_sock pointer.  After the fix it uses
    // conf.GetWEBServerPort() instead.
    //
    // Heap-allocate and intentionally leak to avoid JTC static destruction
    // issues (same pattern as IntegrationEnvironment::TearDown).
    CDispatcher* dispatcher = new CDispatcher();

    bool caught_exception = false;
    try {
        dispatcher->Init();
    } catch (const CEIBException&) {
        caught_exception = true;
    } catch (...) {
        caught_exception = true;
    }

    EXPECT_TRUE(caught_exception)
        << "Expected Dispatcher::Init() to throw on occupied port 18080";

    // Intentionally leak -- JTCThread destructor on an un-started thread
    // can corrupt JTC internal state.
}
