#include "Artifact/Core/Thread.h"
#include "Artifact/Core/LogManager.h"
#include <chrono>
#include <sstream>

#include "Artifact/Core/Macros.h"

namespace Artifact::Core {

    Thread::Thread(ThreadFunc func, const std::string& name)
        : m_function(std::move(func))
        , m_name(name)
        , m_started(false) {
    }

    Thread::~Thread() {
        if (m_thread && m_thread->joinable()) {
            LOG_WARN("Thread '{}' destroyed while still joinable - auto-joining", m_name);
            m_thread->join();
        }
    }

    Thread::Thread(Thread&& other) noexcept
        : m_thread(std::move(other.m_thread))
        , m_function(std::move(other.m_function))
        , m_name(std::move(other.m_name))
        , m_started(other.m_started) {
        other.m_started = false;
    }

    Thread& Thread::operator=(Thread&& other) noexcept {
        if (this != &other) {
            if (m_thread && m_thread->joinable()) {
                m_thread->join();
            }

            m_thread = std::move(other.m_thread);
            m_function = std::move(other.m_function);
            m_name = std::move(other.m_name);
            m_started = other.m_started;
            other.m_started = false;
        }
        return *this;
    }

    void Thread::start() {
        if (m_started) {
            LOG_WARN("Thread '{}' already started", m_name);
            return;
        }

        if (!m_function) {
            LOG_ERROR("Thread '{}' has no function to execute", m_name);
            return;
        }

        m_thread = std::make_unique<std::thread>(m_function);
        m_started = true;

        if (!m_name.empty()) {
            std::ostringstream oss;
            oss << m_thread->get_id();
            LOG_INFO("Thread '{}' started (ID: {})", m_name, oss.str());
        }
    }

    void Thread::join() {
        if (m_thread && m_thread->joinable()) {
            m_thread->join();
        }
    }

    void Thread::detach() {
        if (m_thread && m_thread->joinable()) {
            m_thread->detach();
        }
    }

    bool Thread::joinable() const {
        return m_thread && m_thread->joinable();
    }

    std::thread::native_handle_type Thread::nativeHandle() {
        if (m_thread) {
            return m_thread->native_handle();
        }
        return std::thread::native_handle_type{};
    }

    std::thread::id Thread::getId() const {
        if (m_thread) {
            return m_thread->get_id();
        }
        return std::thread::id{};
    }

    std::thread::id Thread::getCurrentThreadId() {
        return std::this_thread::get_id();
    }

    void Thread::sleepFor(unsigned int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    void Thread::yield() {
        std::this_thread::yield();
    }

} // namespace Artifact::Core

