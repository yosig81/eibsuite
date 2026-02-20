// **********************************************************************
//
// Copyright (c) 2008
// IONA Technologies, Inc.
// Waltham, MA, USA
//
// All Rights Reserved
//
// **********************************************************************

#ifndef JTC_SEMAPHORE_H
#define JTC_SEMAPHORE_H

#include <Types.h>
#include <Syscall.h>

#if defined(HAVE_POSIX_THREADS)
#    include <pthread.h>
#endif
#include <errno.h>

class JTC_IMPORT_EXPORT JTCSemaphore
{
public:

    JTCSemaphore(long initial_count = 0);
    ~JTCSemaphore();

    //
    // Wait for the semaphore to be signaled.  Waits for an infinite
    // period of time
    //
    bool
    wait();

    //
    // Wait for the semaphore to be signaled for timeout msec. A
    // negative number means wait indefinitely. Return true if wait
    // terminated successfully (that is no timeout).
    //
    bool
    wait(long timeout);

    //
    // Increment the semaphore count times.
    //
    void
    post(int count = 1);

private:

    // Non-copyable
    JTCSemaphore(const JTCSemaphore&);
    JTCSemaphore& operator=(const JTCSemaphore&);

#if defined(HAVE_POSIX_THREADS)
    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;
    long            m_count;
#endif
#if defined(HAVE_WIN32_THREADS)
    HANDLE m_sem; // The semaphore handle
#endif
};

#endif // JTC_SEMAPHORE_H
