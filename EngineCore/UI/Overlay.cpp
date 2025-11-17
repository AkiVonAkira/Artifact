#include "Artifact/UI/Overlay.h"

#include <imgui.h>

namespace Artifact::UI {

void Overlay::drawDockspace(bool enableEditorPanels) {
    if (!enableEditorPanels) {
        return;
    }

    static bool dockspaceOpen = true;
    static bool optFullScreen = true;
    static bool optPadding = false;
    ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (optFullScreen) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    if (!optPadding) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    }

    ImGui::Begin("ArtifactDockspace", &dockspaceOpen, windowFlags);

    if (!optPadding) {
        ImGui::PopStyleVar();
    }
    if (optFullScreen) {
        ImGui::PopStyleVar(2);
    }

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        ImGuiID dockspaceID = ImGui::GetID("ArtifactDockspaceID");
        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);
    }

    ImGui::End();
}

void Overlay::drawProfiler(float frameTimeMs) {
    ImGui::Begin("Frame Stats");
    ImGui::Text("Frame: %.2f ms (%.1f FPS)", frameTimeMs, frameTimeMs > 0.0f ? 1000.0f / frameTimeMs : 0.0f);
    ImGui::End();
}

void Overlay::drawPanels() {
    for (auto& callback : m_panelCallbacks) {
        if (callback) {
            callback();
        }
    }
}

void Overlay::registerPanel(PanelCallback callback) {
    m_panelCallbacks.emplace_back(std::move(callback));
}

void Overlay::clearPanels() {
    m_panelCallbacks.clear();
}

}
