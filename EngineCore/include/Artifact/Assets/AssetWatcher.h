#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace Artifact::Assets {

struct AssetChange {
    enum class Type {
        Added,
        Modified,
        Removed,
        Moved
    };

    std::filesystem::path path;
    std::filesystem::path previousPath;
    Type type{Type::Modified};
};

class AssetWatcher {
public:
    using ChangeCallback = std::function<void(const AssetChange&)>;

    AssetWatcher();
    ~AssetWatcher();

    void watch(const std::filesystem::path& root, ChangeCallback callback);
    void pump();

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::vector<ChangeCallback> m_callbacks;
};

}
