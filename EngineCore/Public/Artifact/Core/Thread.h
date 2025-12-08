#pragma once

#include <thread>
#include <functional>
#include <memory>
#include <string>

namespace Artifact::Core {

    /// Cross-platform thread wrapper
    class Thread {
    public:
        using ThreadFunc = std::function<void()>;

        Thread() = default;

        /// Create a thread with a function
        explicit Thread(ThreadFunc func, const std::string& name = "");

        /// Destructor - joins if joinable
        ~Thread();

        // Disable copy
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        // Enable move
        Thread(Thread&& other) noexcept;
        Thread& operator=(Thread&& other) noexcept;

        /// Start the thread
        void start();

        /// Join the thread (wait for completion)
        void join();

        /// Detach the thread
        void detach();

        /// Check if thread is joinable
        [[nodiscard]] bool joinable() const;

        /// Get native thread handle
        [[nodiscard]] std::thread::native_handle_type nativeHandle();

        /// Get thread ID
        [[nodiscard]] std::thread::id getId() const;

        /// Get thread name
        [[nodiscard]] const std::string& getName() const { return m_name; }

        /// Static: Get current thread ID
        static std::thread::id getCurrentThreadId();

        /// Static: Sleep current thread for milliseconds
        static void sleepFor(unsigned int milliseconds);

        /// Static: Yield current thread
        static void yield();

    private:
        std::unique_ptr<std::thread> m_thread;
        ThreadFunc m_function;
        std::string m_name;
        bool m_started{false};
    };

} // namespace Artifact::Core

