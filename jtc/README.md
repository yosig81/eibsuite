# JTC (JThreads/C++)

JTC is a C++ threading library that provides Java-style threading primitives on top of POSIX threads (pthreads) and Win32 threads. It offers reference-counted smart pointers, mutexes, condition variables, monitors, semaphores, reader-writer locks, thread-local storage, and a full thread lifecycle model with thread groups.

## Building

```bash
cd jtc/linux
make
```

This produces `libjtc.so` in the `linux/` directory.

## Initialization

The JTC library must be initialized before use. Create a `JTCInitialize` object in `main()` -- it sets up thread-specific storage, the root thread group, and registers the main thread.

```cpp
#include <JTC.h>

int main(int argc, char** argv)
{
    JTCInitialize jtc;
    // ... use JTC classes ...
    return 0;
}
```

`JTCInitialize` is reference-counted: multiple instances can coexist safely and the library stays active until the last one is destroyed. The `argc`/`argv` constructor accepts optional `-JTCss <bytes>` to set thread stack size.

### Adopting External Threads

Threads created outside JTC (e.g., via `std::thread` or `pthread_create`) cannot use JTC primitives by default. Wrap them with `JTCAdoptCurrentThread`:

```cpp
std::thread t([]() {
    JTCAdoptCurrentThread adopt;
    // Now this thread can use JTC mutexes, monitors, etc.
    JTCThread* me = JTCThread::currentThread();
});
t.join();
```

The adoption lasts until the `JTCAdoptCurrentThread` object is destroyed.

## Smart Pointers (JTCHandleT / JTCRefCount)

JTC provides intrusive reference-counted smart pointers. Any class deriving from `JTCRefCount` can be managed by `JTCHandleT<T>`.

```cpp
class MyObject : public virtual JTCRefCount
{
public:
    void doWork() { /* ... */ }
};

typedef JTCHandleT<MyObject> MyObjectHandle;

MyObjectHandle h = new MyObject();  // refcount = 1
{
    MyObjectHandle h2 = h;          // refcount = 2
}                                   // refcount = 1
// h goes out of scope -> refcount = 0 -> object deleted
```

Predefined handle types:
- `JTCThreadHandle` -- handle to `JTCThread`
- `JTCThreadGroupHandle` -- handle to `JTCThreadGroup`
- `JTCRunnableHandle` -- handle to `JTCRunnable`

## Threads (JTCThread)

Create threads by subclassing `JTCThread` and overriding `run()`, or by passing a `JTCRunnable`.

### Subclassing

```cpp
class Worker : public JTCThread
{
public:
    Worker() : JTCThread("Worker") {}
    void run() override
    {
        // thread body
    }
};

JTCThreadHandle t = new Worker();
t->start();
t->join();
```

### Using JTCRunnable

```cpp
class Task : public JTCRunnable
{
public:
    void run() override { /* ... */ }
};

JTCRunnableHandle task = new Task();
JTCThreadHandle t = new JTCThread(task, "TaskThread");
t->start();
t->join();
```

### Thread API

| Method | Description |
|--------|-------------|
| `start()` | Begin thread execution. Throws `JTCIllegalThreadStateException` if already started. |
| `join()` | Block until the thread finishes. |
| `join(long millis)` | Block for at most `millis` milliseconds. |
| `isAlive()` | Returns `true` if the thread is running. |
| `destroy()` | Clean up an unstarted thread. Throws if the thread is running. |
| `getName()` / `setName()` | Get/set the thread name. |
| `setPriority(int)` / `getPriority()` | Set/get thread priority (values: `JTC_MIN_PRIORITY`, `JTC_NORM_PRIORITY`, `JTC_MAX_PRIORITY`). Note: priorities are effectively no-ops on Linux/macOS since all values map to the same OS scheduling priority. |
| `currentThread()` | Static. Returns a pointer to the calling thread's `JTCThread` object. |
| `sleep(long millis)` | Static. Suspend the current thread. |
| `yield()` | Static. Yield the current thread's timeslice. |
| `activeCount()` | Static. Number of active threads in the current thread group. |
| `enumerate(list, len)` | Static. Copy active threads into the provided array. |

### Thread Hooks

Hooks allow application-specific customization of thread startup:

```cpp
void myRunHook(JTCThread* thread)
{
    // custom setup before run()
    thread->run();
    // custom teardown after run()
}

JTCRunHook oldHook;
JTCThread::setRunHook(myRunHook, &oldHook);
```

Available hooks:
- **Run hook** (`JTCRunHook`) -- wraps the `run()` call. Must call `thread->run()` itself.
- **Start hook** (`JTCStartHook`) -- called when a new thread starts, before `run()`.
- **Attribute hook** (`JTCAttrHook`, POSIX only) -- customize `pthread_attr_t` before thread creation.

## Thread Groups (JTCThreadGroup)

Thread groups organize threads into a hierarchy for collective management.

```cpp
JTCThreadGroupHandle group = new JTCThreadGroup("Workers");

class GroupedWorker : public JTCThread
{
public:
    GroupedWorker(JTCThreadGroupHandle& g, const char* name)
        : JTCThread(g, name) {}
    void run() override { JTCThread::sleep(100); }
};

JTCThreadHandle t1 = new GroupedWorker(group, "W1");
JTCThreadHandle t2 = new GroupedWorker(group, "W2");
t1->start();
t2->start();

int active = group->activeCount();  // 2

t1->join();
t2->join();
group->destroy();  // must be empty to destroy
```

Groups support:
- Parent/child hierarchy (`getParent()`, `parentOf()`)
- Thread and group enumeration (`enumerate()`)
- Maximum priority enforcement (`setMaxPriority()`)
- Daemon groups (`setDaemon()`)
- Uncaught exception handling (`uncaughtException()`)

## Synchronization Primitives

### JTCMutex

Non-recursive mutex. A thread that locks it a second time will deadlock.

```cpp
JTCMutex mutex;
mutex.lock();
// critical section
mutex.unlock();
```

### JTCRecursiveMutex

Recursive mutex that can be locked multiple times by the same thread. Must be unlocked the same number of times.

```cpp
JTCRecursiveMutex mutex;
bool first = mutex.lock();    // true (first acquisition)
bool second = mutex.lock();   // false (recursive)
mutex.unlock();               // returns false (still held)
mutex.unlock();               // returns true (fully released)
```

`count()` tracks the recursion depth. `get_owner()` returns the owning thread's ID.

### JTCRWMutex

Reader-writer lock. Multiple readers can hold the lock simultaneously, but writers get exclusive access. Writers have priority over waiting readers.

```cpp
JTCRWMutex rwmutex;

// Reader
rwmutex.read_lock();
// ... read shared data ...
rwmutex.unlock();

// Writer
rwmutex.write_lock();
// ... modify shared data ...
rwmutex.unlock();
```

### JTCSemaphore

Counting semaphore. `wait()` decrements the count (blocking if zero), `post()` increments it.

```cpp
JTCSemaphore sem(0);

// Producer
sem.post();       // increment by 1
sem.post(5);      // increment by 5

// Consumer
sem.wait();           // block until count > 0, then decrement
bool ok = sem.wait(100);  // wait up to 100ms, returns false on timeout
sem.wait(-1);         // negative timeout = infinite wait
```

### JTCCond

Condition variable for signaling between threads. Works with both `JTCMutex` and `JTCRecursiveMutex`.

```cpp
JTCMutex mutex;
JTCCond cond;
bool ready = false;

// Waiting thread
mutex.lock();
while (!ready)
    cond.wait(mutex);    // atomically unlocks mutex and waits
// ready == true, mutex is locked
mutex.unlock();

// Signaling thread
mutex.lock();
ready = true;
cond.signal();           // wake one waiter
mutex.unlock();
```

`broadcast()` wakes all waiting threads. `wait(mutex, timeout)` returns `false` on timeout.

## Monitors (JTCMonitor)

Monitors combine a recursive mutex with wait/notify semantics, following Mesa monitor conventions. Notifications are deferred until the lock is fully released.

Subclass `JTCMonitor` to create a monitor object:

```cpp
class BoundedQueue : public JTCMonitor
{
    int items[10];
    int count = 0;
public:
    void put(int item)
    {
        JTCSynchronized sync(*this);
        while (count == 10)
            wait();           // releases lock, waits for notify
        items[count++] = item;
        notify();             // wake one consumer (deferred until unlock)
    }

    int get()
    {
        JTCSynchronized sync(*this);
        while (count == 0)
            wait();
        int item = items[--count];
        notify();
        return item;
    }
};
```

Key behavior:
- `wait()` releases the monitor lock and suspends the calling thread.
- `notify()` / `notifyAll()` are **deferred** until the monitor is fully unlocked (Mesa semantics).
- Calling `wait()`, `notify()`, or `notifyAll()` without holding the lock throws `JTCIllegalMonitorStateException`.

`JTCMonitorT<T>` is a template variant that lets you choose the underlying mutex type.

## RAII Lock Guards

JTC provides RAII wrappers that automatically release locks when they go out of scope, even in the presence of exceptions.

### JTCSynchronized

Works with any JTC lock type:

```cpp
JTCMutex mutex;
{
    JTCSynchronized sync(mutex);
    // mutex is locked
}
// mutex is automatically unlocked

JTCRWMutex rwmutex;
{
    JTCSynchronized sync(rwmutex, JTCSynchronized::read_lock);
    // read lock held
}
```

### JTCSyncT\<T\>

Template version -- avoids the runtime dispatch overhead of `JTCSynchronized`:

```cpp
JTCRecursiveMutex mutex;
{
    JTCSyncT<JTCRecursiveMutex> guard(mutex);
    // locked
}
```

### JTCReadLock / JTCWriteLock

Dedicated RAII wrappers for reader-writer locks:

```cpp
JTCRWMutex rwmutex;
{
    JTCReadLock rlock(rwmutex);
    // read lock held
}
{
    JTCWriteLock wlock(rwmutex);
    // write lock held
}
```

## Thread-Specific Storage (JTCTSS)

Per-thread data storage with optional cleanup callbacks:

```cpp
void cleanup(void* data) { delete static_cast<std::string*>(data); }

JTCThreadKey key = JTCTSS::allocate(cleanup);

// In any thread:
JTCTSS::set(key, new std::string("thread-local data"));
std::string* val = static_cast<std::string*>(JTCTSS::get(key));

// cleanup() is called automatically when the thread exits
JTCTSS::release(key);  // when done with the key entirely
```

Each thread sees its own value for a given key. `get()` returns `nullptr` if the key has not been set in the current thread.

## Exception Hierarchy

All JTC exceptions derive from `JTCException`:

```
JTCException
  |-- JTCInterruptedException
  |-- JTCIllegalThreadStateException
  |-- JTCIllegalMonitorStateException
  |-- JTCIllegalArgumentException
  |-- JTCSystemCallException
  |-- JTCOutOfMemoryError
  |-- JTCUnknownThreadException
  |-- JTCInitializeError

JTCThreadDeath  (not derived from JTCException)
```

```cpp
try {
    thread->start();
} catch (const JTCIllegalThreadStateException& e) {
    std::cerr << e.getType() << ": " << e.getMessage() << std::endl;
}
```

`JTCThreadDeath` is a separate class (not derived from `JTCException`) used internally for thread termination control flow.

## Platform Support

JTC abstracts over two threading backends selected at compile time:

| Feature | POSIX (`HAVE_POSIX_THREADS`) | Win32 (`HAVE_WIN32_THREADS`) |
|---------|-----|------|
| Mutex | `pthread_mutex_t` | `CRITICAL_SECTION` |
| Condition variable | `pthread_cond_t` | Manual event-based emulation |
| Thread creation | `pthread_create` | `_beginthreadex` |
| Thread-local storage | `pthread_key_t` | `TlsAlloc` / `TlsFree` |
| Semaphore | `pthread_mutex_t` + `pthread_cond_t` | `CreateSemaphore` |
| Thread ID | `pthread_t` | `DWORD` |

## Testing

The library includes a comprehensive test suite with 152 tests covering all classes:

```bash
cd jtc/linux
make test-run
```

Or directly:

```bash
cd jtc/tests
make run
```
