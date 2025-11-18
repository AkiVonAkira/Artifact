#pragma once

#include "Artifact/Core/Config.h"
#include "Artifact/UI/Overlay.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace Artifact::Rendering {
class Renderer;
}

namespace Artifact::Scripting {
class LuaContext;
}

namespace Artifact::Assets {
class AssetWatcher;
struct AssetChange;
}

namespace Artifact::Core {

struct ApplicationCreateInfo {
    std::string windowTitle{"Artifact Editor"};
    int32_t width{1600};
    int32_t height{900};
    bool enableEditor{true};
    bool enableRuntime{true};
};

class Application {
public:
    explicit Application(const ApplicationCreateInfo& info);
    ~Application();

    void run();
    void requestExit();
    void registerEditorPanel(UI::Overlay::PanelCallback callback);
    void clearEditorPanels();
    void setStatusBarCallback(UI::Overlay::PanelCallback callback);
    void resetLayout();
    bool isKeyDown(int key) const;
    using ScriptAssetCallback = std::function<void(const Assets::AssetChange&)>;
    void registerScriptAssetListener(ScriptAssetCallback callback);

    UI::Overlay* overlay();
    Scripting::LuaContext* luaContext();
    Rendering::Renderer* renderer();
    const RuntimeConfig& runtimeConfig() const;

private:
    void initializeWindow();
    void initializeDearImgui();
    void initializeLua();
    void notifyScriptAssetListeners(const Assets::AssetChange& change);
    void pumpWatchers();

    ApplicationCreateInfo m_info{};
    RuntimeConfig m_runtimeConfig{};
    bool m_shouldExit{false};
    GLFWwindow* m_window{nullptr};

    std::unique_ptr<Rendering::Renderer> m_renderer;
    std::unique_ptr<Scripting::LuaContext> m_luaContext;
    std::unique_ptr<Assets::AssetWatcher> m_assetWatcher;
    std::unique_ptr<UI::Overlay> m_overlay;
    std::vector<ScriptAssetCallback> m_scriptAssetCallbacks;
};

}
