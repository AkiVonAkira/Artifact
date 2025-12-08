#include "Artifact/Core/Mutex.h"
#include <chrono>

namespace Artifact::Core {

    // ============================================================================
    // Mutex
    // ============================================================================

    void Mutex::lock() {
        m_mutex.lock();
    }

    bool Mutex::tryLock() {
        return m_mutex.try_lock();
    }

    void Mutex::unlock() {
        m_mutex.unlock();
    }

    // ============================================================================
    // RWMutex
    // ============================================================================

    void RWMutex::lockWrite() {
        m_mutex.lock();
    }

    bool RWMutex::tryLockWrite() {
        return m_mutex.try_lock();
    }

    void RWMutex::unlockWrite() {
        m_mutex.unlock();
    }

    void RWMutex::lockRead() {
        m_mutex.lock_shared();
    }

    bool RWMutex::tryLockRead() {
        return m_mutex.try_lock_shared();
    }

    void RWMutex::unlockRead() {
        m_mutex.unlock_shared();
    }

    // ============================================================================
    // ConditionVariable
    // ============================================================================

    void ConditionVariable::wait(Mutex& mutex) {
        std::unique_lock<std::mutex> lock(mutex.native(), std::adopt_lock);
        m_cv.wait(lock);
        lock.release();
    }

    bool ConditionVariable::waitFor(Mutex& mutex, unsigned int milliseconds) {
        std::unique_lock<std::mutex> lock(mutex.native(), std::adopt_lock);
        auto result = m_cv.wait_for(lock, std::chrono::milliseconds(milliseconds));
        lock.release();
        return result == std::cv_status::no_timeout;
    }

    void ConditionVariable::notifyOne() {
        m_cv.notify_one();
    }

    void ConditionVariable::notifyAll() {
        m_cv.notify_all();
    }

} // namespace Artifact::Core

