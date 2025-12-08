#include "Artifact/Assets/AssetWatcher.h"

#include "Artifact/Core/Instrumentation.h"

#include <efsw/efsw.hpp>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <mutex>
#include <queue>
#include <system_error>

namespace Artifact::Assets {
    struct AssetWatcher::Impl : efsw::FileWatchListener {
        efsw::FileWatcher watcher;
        std::mutex mutex;
        std::queue<AssetChange> changedEvents;

        static AssetChange::Type MapAction(efsw::Action action) {
            switch (action) {
                case efsw::Actions::Add:
                    return AssetChange::Type::Added;
                case efsw::Actions::Delete:
                    return AssetChange::Type::Removed;
                case efsw::Actions::Moved:
                    return AssetChange::Type::Moved;
                case efsw::Actions::Modified:
                default:
                    return AssetChange::Type::Modified;
            }
        }

        void handleFileAction(efsw::WatchID,
                              const std::string &dir,
                              const std::string &filename,
                              efsw::Action action,
                              std::string oldFilename) override {
            AssetChange change;
            change.path = std::filesystem::path(dir) / filename;
            if (!oldFilename.empty()) {
                change.previousPath = std::filesystem::path(dir) / oldFilename;
            }
            change.type = MapAction(action);

            std::scoped_lock lock(mutex);
            changedEvents.emplace(std::move(change));
        }
    };

    AssetWatcher::AssetWatcher()
        : m_impl(std::make_unique<Impl>()) {
    }

    AssetWatcher::~AssetWatcher() = default;

    void AssetWatcher::watch(const std::filesystem::path &root, ChangeCallback callback) {
        ARTIFACT_ZONE();
        if (!callback) {
            return;
        }

        std::error_code ec;
        if (!std::filesystem::exists(root)) {
            std::filesystem::create_directories(root, ec);
            if (ec) {
                spdlog::error("Failed to create asset watch directory '{}': {}", root.string(), ec.message());
                return;
            }
        }

        const auto canonicalRoot = std::filesystem::weakly_canonical(root, ec);
        const auto watchPath = ec ? root : canonicalRoot;

        const auto watchId = m_impl->watcher.addWatch(watchPath.string(), m_impl.get(), true);
        if (watchId < 0) {
            spdlog::error("Failed to add asset watcher for '{}'", watchPath.string());
            return;
        }

        spdlog::info("Watching assets at '{}'", watchPath.string());
        m_callbacks.emplace_back(std::move(callback));
        m_impl->watcher.watch();
    }

    void AssetWatcher::pump() {
        ARTIFACT_ZONE();
        std::queue<AssetChange> local;
        {
            std::scoped_lock lock(m_impl->mutex);
            std::swap(local, m_impl->changedEvents);
        }

        while (!local.empty()) {
            const auto &change = local.front();
            for (auto &cb: m_callbacks) {
                cb(change);
            }
            local.pop();
        }
    }
}
