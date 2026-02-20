// **********************************************************************
//
// Copyright (c) 2008
// IONA Technologies, Inc.
// Waltham, MA, USA
//
// All Rights Reserved
//
// **********************************************************************

#include <JTCSemaphore.h>

#if defined(HAVE_POSIX_THREADS)
#   include <sys/time.h>
#endif

#ifndef HAVE_JTC_NO_IOSTREAM
#   ifdef HAVE_STD_IOSTREAM
using namespace std;
#   endif
#endif // !HAVE_JTC_NO_IOSTREAM

// ----------------------------------------------------------------------
// JTCSemaphore constructor and destructor
// ----------------------------------------------------------------------

JTCSemaphore::JTCSemaphore(long initial_count)
{
#if defined(HAVE_POSIX_THREADS)
    m_count = initial_count;
    int rc = pthread_mutex_init(&m_mutex, 0);
    if (rc != 0)
        JTC_THROW_EXCEPTION(rc, "pthread_mutex_init failed in JTCSemaphore")
    rc = pthread_cond_init(&m_cond, 0);
    if (rc != 0)
    {
        pthread_mutex_destroy(&m_mutex);
        JTC_THROW_EXCEPTION(rc, "pthread_cond_init failed in JTCSemaphore")
    }
#endif
#if defined(HAVE_WIN32_THREADS)
    JTC_SYSCALL_4(m_sem = CreateSemaphore, 0, initial_count, 0x7fffffff, 0,
                  == INVALID_HANDLE_VALUE)
#endif
}

JTCSemaphore::~JTCSemaphore()
{
#if defined(HAVE_POSIX_THREADS)
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
#endif
#if defined(HAVE_WIN32_THREADS)
    CloseHandle(m_sem);
#endif
}

// ----------------------------------------------------------------------
// JTCSemaphore public member implementation
// ----------------------------------------------------------------------
bool
JTCSemaphore::wait()
{
#if defined(HAVE_POSIX_THREADS)
    int rc = pthread_mutex_lock(&m_mutex);
    if (rc != 0)
        JTC_THROW_EXCEPTION(rc, "Semaphore wait: pthread_mutex_lock failed")

    while (m_count <= 0)
    {
        rc = pthread_cond_wait(&m_cond, &m_mutex);
        if (rc != 0)
        {
            pthread_mutex_unlock(&m_mutex);
            JTC_THROW_EXCEPTION(rc, "Semaphore wait: pthread_cond_wait failed")
        }
    }
    --m_count;
    pthread_mutex_unlock(&m_mutex);
    return true;
#endif
#if defined(HAVE_WIN32_THREADS)
    int rc;
    JTC_SYSCALL_2(rc = WaitForSingleObject, m_sem, INFINITE, == WAIT_ABANDONED);
    return rc != WAIT_TIMEOUT;
#endif
}

bool
JTCSemaphore::wait(long timeout)
{
#if defined(HAVE_POSIX_THREADS)
    if (timeout < 0)
    {
        return wait(); // infinite wait
    }

    struct timeval tv;
    struct timespec abstime;
    gettimeofday(&tv, 0);
    const long oneBillion = 1000000000;

    abstime.tv_sec = tv.tv_sec + (timeout / 1000);
    abstime.tv_nsec = (tv.tv_usec * 1000) + ((timeout % 1000) * 1000000);
    if (abstime.tv_nsec >= oneBillion)
    {
        ++abstime.tv_sec;
        abstime.tv_nsec -= oneBillion;
    }

    int rc = pthread_mutex_lock(&m_mutex);
    if (rc != 0)
        JTC_THROW_EXCEPTION(rc, "Semaphore wait: pthread_mutex_lock failed")

    while (m_count <= 0)
    {
        rc = pthread_cond_timedwait(&m_cond, &m_mutex, &abstime);
        if (rc == ETIMEDOUT)
        {
            pthread_mutex_unlock(&m_mutex);
            return false;
        }
        if (rc != 0)
        {
            pthread_mutex_unlock(&m_mutex);
            JTC_THROW_EXCEPTION(rc, "Semaphore wait: pthread_cond_timedwait failed")
        }
    }
    --m_count;
    pthread_mutex_unlock(&m_mutex);
    return true;
#endif
#if defined(HAVE_WIN32_THREADS)
    if (timeout < 0)
    {
        timeout = INFINITE;
    }
    int rc;
    JTC_SYSCALL_2(rc = WaitForSingleObject, m_sem, timeout, == WAIT_ABANDONED);
    return rc != WAIT_TIMEOUT;
#endif
}

void
JTCSemaphore::post(int count)
{
#if defined(HAVE_POSIX_THREADS)
    int rc = pthread_mutex_lock(&m_mutex);
    if (rc != 0)
        JTC_THROW_EXCEPTION(rc, "Semaphore post: pthread_mutex_lock failed")

    m_count += count;

    // Wake waiting threads. If count > 1, broadcast to wake all waiters
    // so they can each re-check the count.
    if (count > 1)
        pthread_cond_broadcast(&m_cond);
    else if (count == 1)
        pthread_cond_signal(&m_cond);

    pthread_mutex_unlock(&m_mutex);
#endif
#if defined(HAVE_WIN32_THREADS)
    JTC_SYSCALL_3(ReleaseSemaphore, m_sem, count, 0, == 0)
#endif
}
