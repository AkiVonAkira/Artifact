#include "Artifact/UI/Overlay.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>

namespace {

bool ShouldApplyDefaultLayout(const ImGuiIO& io) {
    if (!io.IniFilename || io.IniFilename[0] == '\0') {
        return true;
    }

    std::error_code ec;
    return !std::filesystem::exists(io.IniFilename, ec);
}

void BuildDefaultLayout(ImGuiID dockspaceID) {
    ImGui::DockBuilderRemoveNode(dockspaceID);
    ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->Size);

    ImGuiID dockMainID = dockspaceID;
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Left, 0.2f, nullptr, &dockMainID);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Right, 0.25f, nullptr, &dockMainID);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMainID, ImGuiDir_Down, 0.27f, nullptr, &dockMainID);
    ImGuiID dockLeftBottom = ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Down, 0.5f, nullptr, &dockLeft);

    ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
    ImGui::DockBuilderDockWindow("Assets", dockLeftBottom);
    ImGui::DockBuilderDockWindow("Scene", dockMainID);
    ImGui::DockBuilderDockWindow("Inspector", dockRight);
    ImGui::DockBuilderDockWindow("Frame Stats", dockRight);
    ImGui::DockBuilderDockWindow("Console", dockBottom);

    ImGui::DockBuilderFinish(dockspaceID);
}

} // namespace

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
        if (ImGui::BeginMenuBar()) {
            if (m_statusBarCallback) {
                m_statusBarCallback();
            }
            ImGui::EndMenuBar();
        }

        ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), dockspaceFlags);

        if (ShouldApplyDefaultLayout(io) || m_forceDefaultLayout) {
            BuildDefaultLayout(dockspaceID);
            m_forceDefaultLayout = false;
        }
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

void Overlay::setStatusBarCallback(PanelCallback callback) {
    m_statusBarCallback = std::move(callback);
}

void Overlay::requestDefaultLayout() {
    m_forceDefaultLayout = true;
}

}
