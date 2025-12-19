#pragma once

#include "Artifact/Core/Config.h"
#include "Artifact/UI/Overlay.h"
#include "Artifact/Core/PlayModeController.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace Artifact::Rendering {
    class RenderManager;
    using Renderer = RenderManager; // Backward compatibility alias
}

namespace Artifact::Assets {
    class AssetWatcher;
    struct AssetChange;
}

namespace Artifact::Core {
    class Application;
    class PlayModeController;
    class LoggingManager;
    class LogManager;
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
        explicit Application(const ApplicationCreateInfo &info);

        ~Application();

        void run();

        void requestExit();

        void registerEditorPanel(UI::Overlay::PanelCallback callback);

        void clearEditorPanels();

        void setStatusBarCallback(UI::Overlay::PanelCallback callback);

        void resetLayout();

        bool isKeyDown(int key) const;

        using ScriptAssetCallback = std::function<void(const Assets::AssetChange &)>;

        void registerScriptAssetListener(ScriptAssetCallback callback);

        UI::Overlay *overlay();

        const RuntimeConfig &runtimeConfig() const;

        PlayModeController *playMode();

    private:
        void initializeWindow();

        void initializeDearImgui();

        void pumpWatchers();

        ApplicationCreateInfo m_info{};
        RuntimeConfig m_runtimeConfig{};
        bool m_shouldExit{false};
        GLFWwindow *m_window{nullptr};

        std::unique_ptr<Assets::AssetWatcher> m_assetWatcher;
        std::unique_ptr<UI::Overlay> m_overlay;
        std::unique_ptr<PlayModeController> m_playModeController;
        std::unique_ptr<LoggingManager> m_loggingManager;
    };
}
