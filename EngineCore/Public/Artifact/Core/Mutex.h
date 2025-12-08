#pragma once

#include <mutex>
#include <shared_mutex>
#include <condition_variable>

namespace Artifact::Core {

    /// Cross-platform mutex wrapper
    class Mutex {
    public:
        Mutex() = default;
        ~Mutex() = default;

        // Disable copy and move
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        Mutex(Mutex&&) = delete;
        Mutex& operator=(Mutex&&) = delete;

        /// Lock the mutex
        void lock();

        /// Try to lock the mutex (non-blocking)
        [[nodiscard]] bool tryLock();

        /// Unlock the mutex
        void unlock();

        /// Get native mutex handle
        [[nodiscard]] std::mutex& native() { return m_mutex; }

    private:
        std::mutex m_mutex;
        friend class ConditionVariable;
    };

    /// RAII lock guard for Mutex
    class ScopedLock {
    public:
        explicit ScopedLock(Mutex& mutex) : m_lock(mutex.native()) {}
        ~ScopedLock() = default;

        // Disable copy and move
        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;

    private:
        std::unique_lock<std::mutex> m_lock;
    };

    /// Reader-Writer lock (shared mutex)
    class RWMutex {
    public:
        RWMutex() = default;
        ~RWMutex() = default;

        // Disable copy and move
        RWMutex(const RWMutex&) = delete;
        RWMutex& operator=(const RWMutex&) = delete;

        /// Lock for writing (exclusive)
        void lockWrite();

        /// Try to lock for writing
        [[nodiscard]] bool tryLockWrite();

        /// Unlock write lock
        void unlockWrite();

        /// Lock for reading (shared)
        void lockRead();

        /// Try to lock for reading
        [[nodiscard]] bool tryLockRead();

        /// Unlock read lock
        void unlockRead();

    private:
        std::shared_mutex m_mutex;
    };

    /// RAII read lock guard
    class ReadLock {
    public:
        explicit ReadLock(RWMutex& mutex) : m_mutex(mutex) {
            m_mutex.lockRead();
        }
        ~ReadLock() {
            m_mutex.unlockRead();
        }

        // Disable copy and move
        ReadLock(const ReadLock&) = delete;
        ReadLock& operator=(const ReadLock&) = delete;

    private:
        RWMutex& m_mutex;
    };

    /// RAII write lock guard
    class WriteLock {
    public:
        explicit WriteLock(RWMutex& mutex) : m_mutex(mutex) {
            m_mutex.lockWrite();
        }
        ~WriteLock() {
            m_mutex.unlockWrite();
        }

        // Disable copy and move
        WriteLock(const WriteLock&) = delete;
        WriteLock& operator=(WriteLock&&) = delete;

    private:
        RWMutex& m_mutex;
    };

    /// Condition variable for thread synchronization
    class ConditionVariable {
    public:
        ConditionVariable() = default;
        ~ConditionVariable() = default;

        // Disable copy and move
        ConditionVariable(const ConditionVariable&) = delete;
        ConditionVariable& operator=(const ConditionVariable&) = delete;

        /// Wait for notification
        void wait(Mutex& mutex);

        /// Wait with timeout (milliseconds)
        [[nodiscard]] bool waitFor(Mutex& mutex, unsigned int milliseconds);

        /// Notify one waiting thread
        void notifyOne();

        /// Notify all waiting threads
        void notifyAll();

    private:
        std::condition_variable m_cv;
    };

} // namespace Artifact::Core

