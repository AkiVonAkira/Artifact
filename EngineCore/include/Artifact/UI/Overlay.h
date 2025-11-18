#pragma once

#include <functional>
#include <string>
#include <vector>

namespace Artifact::UI {

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

private:
    std::vector<PanelCallback> m_panelCallbacks;
    PanelCallback m_statusBarCallback;
    bool m_forceDefaultLayout{false};
};

}
