#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace Artifact::Assets {

class AssetWatcher {
public:
    using PathCallback = std::function<void(const std::filesystem::path&)>;

    AssetWatcher();
    ~AssetWatcher();

    void watch(const std::filesystem::path& root, PathCallback callback);
    void pump();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::vector<PathCallback> m_callbacks;
};

}
