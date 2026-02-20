add_executable(jtc_tests
    unit/CondTest.cpp
    unit/ExceptionTest.cpp
    unit/HandleTest.cpp
    unit/InitializeTest.cpp
    unit/MonitorTest.cpp
    unit/MutexTest.cpp
    unit/RecursiveMutexTest.cpp
    unit/RWMutexTest.cpp
    unit/SemaphoreTest.cpp
    unit/SyncTest.cpp
    unit/TestMain.cpp
    unit/ThreadGroupTest.cpp
    unit/ThreadIdTest.cpp
    unit/ThreadTest.cpp
    unit/TSSTest.cpp
)

set_target_properties(jtc_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_link_libraries(jtc_tests PRIVATE jtc GTest::gtest)

gtest_discover_tests(jtc_tests)
