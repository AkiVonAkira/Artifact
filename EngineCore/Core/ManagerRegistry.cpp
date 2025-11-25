#include "Artifact/Core/ManagerRegistry.h"
#include <algorithm>
#include <spdlog/spdlog.h>

namespace Artifact::Core {
    ManagerRegistry &ManagerRegistry::Instance() {
        static ManagerRegistry instance;
        return instance;
    }

    void ManagerRegistry::RegisterManager(
        const std::string &name,
        InitFunction init,
        ShutdownFunction shutdown,
        int priority
    ) {
        m_managers.push_back({name, init, shutdown, priority});
    }

    void ManagerRegistry::InitializeAll() {
        if (m_initialized) {
            spdlog::warn("ManagerRegistry::InitializeAll called but already initialized");
            return;
        }

        // Sort by priority (lower priority first)
        std::sort(m_managers.begin(), m_managers.end(),
                  [](const ManagerEntry &a, const ManagerEntry &b) {
                      return a.priority < b.priority;
                  });

        spdlog::info("Initializing {} managers...", m_managers.size());

        for (const auto &entry: m_managers) {
            spdlog::info("  Initializing: {}", entry.name);
            if (entry.init) {
                entry.init();
            }
        }

        m_initialized = true;
        spdlog::info("All managers initialized successfully");
    }

    void ManagerRegistry::ShutdownAll() {
        if (!m_initialized) {
            return;
        }

        spdlog::info("Shutting down {} managers...", m_managers.size());

        // Shutdown in reverse order
        for (auto it = m_managers.rbegin(); it != m_managers.rend(); ++it) {
            spdlog::info("  Shutting down: {}", it->name);
            if (it->shutdown) {
                it->shutdown();
            }
        }

        m_initialized = false;
        spdlog::info("All managers shut down successfully");
    }
} // namespace Artifact::Core
