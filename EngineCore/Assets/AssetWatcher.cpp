#include "Artifact/Assets/AssetWatcher.h"

#include "Artifact/Core/Instrumentation.h"

#include <efsw/efsw.hpp>

#include <mutex>
#include <queue>

namespace Artifact::Assets {

struct AssetWatcher::Impl : efsw::FileWatchListener {
    efsw::FileWatcher watcher;
    std::mutex mutex;
    std::queue<std::filesystem::path> changedPaths;

    void handleFileAction(efsw::WatchID,
                          const std::string& dir,
                          const std::string& filename,
                          efsw::Action action,
                          std::string) override {
        if (action == efsw::Actions::Modified || action == efsw::Actions::Add || action == efsw::Actions::Moved) {
            std::scoped_lock lock(mutex);
            changedPaths.emplace(std::filesystem::path(dir) / filename);
        }
    }
};

AssetWatcher::AssetWatcher()
    : m_impl(std::make_unique<Impl>()) {}

AssetWatcher::~AssetWatcher() = default;

void AssetWatcher::watch(const std::filesystem::path& root, PathCallback callback) {
    ARTIFACT_ZONE();
    m_callbacks.emplace_back(std::move(callback));
    m_impl->watcher.addWatch(root.string(), m_impl.get(), true);
    m_impl->watcher.watch();
}

void AssetWatcher::pump() {
    ARTIFACT_ZONE();
    std::queue<std::filesystem::path> local;
    {
        std::scoped_lock lock(m_impl->mutex);
        std::swap(local, m_impl->changedPaths);
    }

    while (!local.empty()) {
        const auto path = local.front();
        for (auto& cb : m_callbacks) {
            cb(path);
        }
        local.pop();
    }
}

}
