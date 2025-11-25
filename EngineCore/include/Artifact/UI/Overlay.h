#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Artifact::UI {
    struct OverlayPanelDescriptor {
        std::string id;
        std::string label;
        bool allowMultiple{true};
        bool hasWindow{true};
        bool closeable{true};
        bool *openFlag{nullptr};
        std::function<void()> draw;
    };

    class Overlay {
    public:
        using PanelCallback = std::function<void()>;

        void drawDockspace(bool enableEditorPanels);

        void drawProfiler(float frameTimeMs);

        void drawPanels();

        void registerPanel(PanelCallback callback);

        void clearPanels();

        void setStatusBarCallback(PanelCallback callback);

        void requestDefaultLayout();

        void setPanelDescriptors(std::vector<OverlayPanelDescriptor> descriptors);

        const std::vector<OverlayPanelDescriptor> &panelDescriptors() const { return m_descriptors; }

    private:
        std::vector<PanelCallback> m_panelCallbacks;
        PanelCallback m_statusBarCallback;
        bool m_forceDefaultLayout{false};

        std::vector<OverlayPanelDescriptor> m_descriptors;
    };
}
