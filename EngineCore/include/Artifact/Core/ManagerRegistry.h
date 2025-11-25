// Created by Artifact Engine Team on 2025-11-25.
// Manager registry for coordinated initialization and shutdown

#pragma once

#include <functional>
#include <vector>
#include <string>

namespace Artifact::Core {
    /**
     * @brief Manages initialization and shutdown order of all engine managers
     *
     * This ensures managers are initialized in the correct order and shut down
     * in reverse order, preventing use-after-free and dependency issues.
     */
    class ManagerRegistry {
    public:
        using InitFunction = std::function<void()>;
        using ShutdownFunction = std::function<void()>;

        struct ManagerEntry {
            std::string name;
            InitFunction init;
            ShutdownFunction shutdown;
            int priority; // Lower priority initializes first
        };

        static ManagerRegistry &Instance();

        /**
         * @brief Register a manager with initialization and shutdown callbacks
         * @param name Manager name for debugging
         * @param init Initialization function
         * @param shutdown Shutdown function
         * @param priority Initialization priority (lower = earlier)
         */
        void RegisterManager(
            const std::string &name,
            InitFunction init,
            ShutdownFunction shutdown,
            int priority = 100
        );

        /**
         * @brief Initialize all registered managers in priority order
         */
        void InitializeAll();

        /**
         * @brief Shutdown all registered managers in reverse priority order
         */
        void ShutdownAll();

        /**
         * @brief Check if all managers are initialized
         */
        bool IsInitialized() const { return m_initialized; }

    private:
        ManagerRegistry() = default;

        ~ManagerRegistry() = default;

        std::vector<ManagerEntry> m_managers;
        bool m_initialized = false;
    };
} // namespace Artifact::Core
