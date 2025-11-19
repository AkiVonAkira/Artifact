#include "Artifact/Core/Application.h"

#include "Artifact/Assets/AssetWatcher.h"
#include "Artifact/Core/Config.h"
#include "Artifact/Core/Instrumentation.h"
#include "Artifact/Rendering/Renderer.h"
#include "Artifact/Scripting/LuaContext.h"
#include "Artifact/UI/Overlay.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <utility>

namespace Artifact::Core {

namespace {
constexpr const char* kImguiGlsl = "#version 330";
}

Application::Application(const ApplicationCreateInfo& info)
    : m_info(info) {
    ARTIFACT_ZONE();
    m_runtimeConfig = LoadRuntimeConfig();
    initializeWindow();
    initializeDearImgui();
    initializeLua();

    m_renderer = std::make_unique<Rendering::Renderer>();
    m_assetWatcher = std::make_unique<Assets::AssetWatcher>();
    m_overlay = std::make_unique<UI::Overlay>();
    m_sceneService = std::make_unique<Scene::SceneService>();
    m_scriptAssets = std::make_unique<Scripting::ScriptAssetService>(m_runtimeConfig, *m_luaContext);
    m_scriptAssets->refresh(true);
    m_sceneService->createDefaultScene(m_scriptAssets.get());
    m_playModeController = std::make_unique<PlayModeController>();
    m_luaBindings = std::make_unique<Scripting::LuaBindings>(*m_luaContext);
    m_luaRuntime = std::make_unique<Scripting::LuaRuntimeSystem>(*m_luaContext, *m_scriptAssets, *m_luaBindings);

    if (m_assetWatcher) {
        m_assetWatcher->watch(m_runtimeConfig.paths.rawAssets, [this](const Assets::AssetChange& change) {
            const auto ext = change.path.extension().string();
            if (ext == ".lua") {
                if (m_luaContext && change.type != Assets::AssetChange::Type::Removed) {
                    m_luaContext->enqueueReload(change.path.stem().string());
                }
                notifyScriptAssetListeners(change);
            }
        });

        m_assetWatcher->watch(m_runtimeConfig.paths.shaderRoot, [](const Assets::AssetChange& change) {
            spdlog::info("Shader changed: {} (queued for recompile)", change.path.string());
        });
    }
}

Application::~Application() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }
}

void Application::run() {
    ARTIFACT_ZONE();
    auto lastFrame = std::chrono::high_resolution_clock::now();

    while (!m_shouldExit && !glfwWindowShouldClose(m_window)) {
        ARTIFACT_ZONE();
        auto now = std::chrono::high_resolution_clock::now();
        const float delta = std::chrono::duration<float>(now - lastFrame).count();
        lastFrame = now;

        int fbWidth = 0;
        int fbHeight = 0;
        glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

        glfwPollEvents();
        pumpWatchers();
        m_luaContext->pollReloads();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Rendering::FrameContext ctx{delta, fbWidth, fbHeight};
        m_renderer->beginFrame(ctx);

        m_overlay->drawDockspace(m_info.enableEditor);
        if (m_info.enableEditor) {
            m_overlay->drawPanels();
        }
        m_overlay->drawProfiler(delta * 1000.0f);

        ImGui::Render();
        m_renderer->endFrame();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backupWindow = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backupWindow);
        }

        glfwSwapBuffers(m_window);
        ARTIFACT_FRAME_MARK;
    }
}

void Application::requestExit() {
    m_shouldExit = true;
}

void Application::registerEditorPanel(UI::Overlay::PanelCallback callback) {
    if (!m_overlay || !m_info.enableEditor) {
        return;
    }
    m_overlay->registerPanel(std::move(callback));
}

void Application::clearEditorPanels() {
    if (m_overlay) {
        m_overlay->clearPanels();
    }
}

void Application::setStatusBarCallback(UI::Overlay::PanelCallback callback) {
    if (m_overlay) {
        m_overlay->setStatusBarCallback(std::move(callback));
    }
}

void Application::resetLayout() {
    if (m_overlay) {
        m_overlay->requestDefaultLayout();
    }
}

bool Application::isKeyDown(int key) const {
    if (!m_window) {
        return false;
    }
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

UI::Overlay* Application::overlay() {
    return m_overlay.get();
}

Scripting::LuaContext* Application::luaContext() {
    return m_luaContext.get();
}

Scene::SceneService* Application::sceneService() {
    return m_sceneService.get();
}

Scripting::ScriptAssetService* Application::scriptAssets() {
    return m_scriptAssets.get();
}

Scripting::LuaRuntimeSystem* Application::luaRuntime() {
    return m_luaRuntime.get();
}

PlayModeController* Application::playMode() {
    return m_playModeController.get();
}

Scripting::LuaBindings* Application::luaBindings() {
    return m_luaBindings.get();
}

const Scripting::LuaBindings* Application::luaBindings() const {
    return m_luaBindings.get();
}

const RuntimeConfig& Application::runtimeConfig() const {
    return m_runtimeConfig;
}

void Application::registerScriptAssetListener(ScriptAssetCallback callback) {
    if (!callback) {
        return;
    }
    m_scriptAssetCallbacks.emplace_back(std::move(callback));
}

void Application::initializeWindow() {
    ARTIFACT_ZONE();
    if (!glfwInit()) {
        spdlog::critical("Failed to initialize GLFW");
        std::terminate();
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(m_info.width, m_info.height, m_info.windowTitle.c_str(), nullptr, nullptr);
    if (!m_window) {
        spdlog::critical("Failed to create GLFW window");
        std::terminate();
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        spdlog::critical("Failed to initialize GLAD");
        std::terminate();
    }
}

void Application::initializeDearImgui() {
    ARTIFACT_ZONE();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(kImguiGlsl);
}

void Application::initializeLua() {
    ARTIFACT_ZONE();
    // Register script reload callback to log changes.
    m_luaContext = std::make_unique<Scripting::LuaContext>();
    m_luaContext->registerReloadCallback([](const std::string& module) {
        spdlog::info("Reloaded Lua module: {}", module);
    });
}

void Application::notifyScriptAssetListeners(const Assets::AssetChange& change) {
    if (m_scriptAssetCallbacks.empty()) {
        return;
    }
    for (auto& callback : m_scriptAssetCallbacks) {
        callback(change);
    }
}

void Application::pumpWatchers() {
    ARTIFACT_ZONE();
    if (m_assetWatcher) {
        m_assetWatcher->pump();
    }
}

Rendering::Renderer* Application::renderer() {
    return m_renderer.get();
}

const Rendering::Renderer* Application::renderer() const {
    return m_renderer.get();
}

}
