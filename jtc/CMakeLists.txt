add_library(jtc SHARED
    src/Cond.cpp
    src/Monitor.cpp
    src/Mutex.cpp
    src/RWMutex.cpp
    src/Semaphore.cpp
    src/Sync.cpp
    src/TSS.cpp
    src/TSSManager.cpp
    src/Thread.cpp
    src/ThreadGroup.cpp
    src/Version.cpp
)

target_include_directories(jtc PUBLIC include)
target_link_libraries(jtc PUBLIC Threads::Threads)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
