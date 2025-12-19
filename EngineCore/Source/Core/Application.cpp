#include "Artifact/Core/Application.h"
#include "Artifact/Core/LoggingManager.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/InputManager.h"
#include "Artifact/Core/TimeManager.h"

#include "Artifact/Assets/AssetWatcher.h"
#include "Artifact/Assets/AssetManager.h"
#include "Artifact/Scene/SceneManager.h"
#include "Artifact/Rendering/RenderManager.h"
#include "Artifact/Physics/PhysicsManager.h"
#include "Artifact/Core/Config.h"
#include "Artifact/Core/Instrumentation.h"
#include "Artifact/UI/Overlay.h"
#include "Artifact/Core/MessageQueue.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <utility>

#include "Artifact/Core/Macros.h"

namespace Artifact::Core {
    namespace {
        constexpr const char *kImguiGlsl = "#version 330";
    }

    Application::Application(const ApplicationCreateInfo &info)
        : m_info(info) {
        // Initialize singleton managers
        const std::string logDir = (std::filesystem::path(Artifact::Core::Paths::ProjectRoot()) / "Logs").string();
        LogManager::Instance().Initialize(logDir);
        // Start with warnings-only to avoid console noise during startup. EditorLayer will enable info when ready.
        LogManager::Instance().SetLevel(spdlog::level::warn);
        LogManager::Instance().FlushOn(spdlog::level::warn);

        initializeWindow();

        LOG_INFO("Initializing InputManager...");
        spdlog::default_logger()->flush();
        // Initialize InputManager with window
        InputManager::Instance().Initialize(m_window);
        LOG_INFO("InputManager initialized successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Initializing TimeManager...");
        spdlog::default_logger()->flush();
        // Initialize TimeManager
        TimeManager::Instance().Initialize();
        LOG_INFO("TimeManager initialized successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Initializing AssetManager...");
        spdlog::default_logger()->flush();
        // Initialize AssetManager with hot-reload enabled
        Assets::AssetManager::Instance().Initialize(true);
        LOG_INFO("AssetManager initialized successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Initializing SceneManager...");
        spdlog::default_logger()->flush();
        // Initialize SceneManager
        Scene::SceneManager::Instance().Initialize();
        LOG_INFO("SceneManager initialized successfully");
        spdlog::default_logger()->flush();

        initializeDearImgui();

        LOG_INFO("Initializing RenderManager...");
        spdlog::default_logger()->flush();
        // Initialize singleton managers (they initialize in their constructors)
        // Just access to trigger construction
        Rendering::RenderManager::Instance();
        LOG_INFO("RenderManager initialized successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Initializing PhysicsManager...");
        spdlog::default_logger()->flush();
        Physics::PhysicsManager::Instance();
        LOG_INFO("PhysicsManager initialized successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Creating AssetWatcher...");
        spdlog::default_logger()->flush();
        m_assetWatcher = std::make_unique<Assets::AssetWatcher>();
        LOG_INFO("AssetWatcher created successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Creating UI Overlay...");
        spdlog::default_logger()->flush();
        m_overlay = std::make_unique<UI::Overlay>();
        LOG_INFO("UI Overlay created successfully");
        spdlog::default_logger()->flush();

        LOG_INFO("Creating PlayModeController...");
        spdlog::default_logger()->flush();
        m_playModeController = std::make_unique<PlayModeController>();
        LOG_INFO("PlayModeController created successfully");
        spdlog::default_logger()->flush();

        if (m_assetWatcher) {
            m_assetWatcher->watch(m_runtimeConfig.paths.rawAssets, [this](const Assets::AssetChange &change) {
                const auto ext = change.path.extension().string();
                // TODO: handle file extensions here
            });

            m_assetWatcher->watch(m_runtimeConfig.paths.shaderRoot, [](const Assets::AssetChange &change) {
                LOG_INFO("Shader changed: {} (queued for recompile)", change.path.string());
            });
        }
    }

    Application::~Application() {
        // Note: Singleton managers clean up automatically in their destructors when the program exits
        // We keep the explicit Shutdown calls here for managers that need explicit cleanup
        Scene::SceneManager::Instance().Shutdown();
        Assets::AssetManager::Instance().Shutdown();
        TimeManager::Instance().Shutdown();
        InputManager::Instance().Shutdown();
        LogManager::Instance().Shutdown();

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

        while (!m_shouldExit && !glfwWindowShouldClose(m_window)) {
            ARTIFACT_ZONE();

            // Process messages
            Artifact::Core::MessageQueue::Instance().process();

            // Update singleton managers
            TimeManager::Instance().Update();
            InputManager::Instance().Update();

            const float delta = TimeManager::Instance().DeltaTime();

            int fbWidth = 0;
            int fbHeight = 0;
            glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

            glfwPollEvents();

            // Update AssetManager for hot-reload
            Assets::AssetManager::Instance().Update();

            pumpWatchers();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            Rendering::FrameContext ctx{delta, fbWidth, fbHeight};
            Rendering::RenderManager::Instance().BeginFrame(ctx);

            m_overlay->drawDockspace(m_info.enableEditor);
            if (m_info.enableEditor) {
                m_overlay->drawPanels();
            }
            m_overlay->drawProfiler(delta * 1000.0f);

            ImGui::Render();
            Rendering::RenderManager::Instance().EndFrame();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow *backupWindow = glfwGetCurrentContext();
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
        return InputManager::Instance().IsKeyDown(key);
    }

    UI::Overlay *Application::overlay() {
        return m_overlay.get();
    }

    PlayModeController *Application::playMode() {
        return m_playModeController.get();
    }


    const RuntimeConfig &Application::runtimeConfig() const {
        return m_runtimeConfig;
    }

    void Application::initializeWindow() {
        ARTIFACT_ZONE();
        LOG_INFO("Initializing GLFW...");
        spdlog::default_logger()->flush();
        if (!glfwInit()) {
            spdlog::critical("Failed to initialize GLFW");
            spdlog::default_logger()->flush();
            std::terminate();
        }
        LOG_INFO("GLFW initialized successfully");
        spdlog::default_logger()->flush();

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        LOG_INFO("Creating GLFW window...");
        spdlog::default_logger()->flush();
        m_window = glfwCreateWindow(m_info.width, m_info.height, m_info.windowTitle.c_str(), nullptr, nullptr);
        if (!m_window) {
            spdlog::critical("Failed to create GLFW window");
            spdlog::default_logger()->flush();
            std::terminate();
        }
        LOG_INFO("GLFW window created successfully");
        spdlog::default_logger()->flush();

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1);

        LOG_INFO("Initializing GLAD...");
        spdlog::default_logger()->flush();
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
            spdlog::critical("Failed to initialize GLAD");
            spdlog::default_logger()->flush();
            std::terminate();
        }
        LOG_INFO("GLAD initialized successfully");
        spdlog::default_logger()->flush();
    }

    void Application::initializeDearImgui() {
        ARTIFACT_ZONE();
        LOG_INFO("Initializing Dear ImGui...");
        spdlog::default_logger()->flush();

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init(kImguiGlsl);

        LOG_INFO("Dear ImGui initialized successfully");
        spdlog::default_logger()->flush();
    }


    void Application::pumpWatchers() {
        ARTIFACT_ZONE();
        if (m_assetWatcher) {
            m_assetWatcher->pump();
        }
    }
}
