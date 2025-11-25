// Created by Artifact Engine Team on 2025-11-25.
// Input manager with singleton pattern

#pragma once

#include "Artifact/Core/Singleton.h"
#include <unordered_map>
#include <array>

struct GLFWwindow;

namespace Artifact::Core {
    /**
     * @brief Centralized input management system
     *
     * Provides global access to keyboard and mouse state without passing
     * window pointers around the codebase.
     */
    class InputManager : public Singleton<InputManager> {
        friend class Singleton<InputManager>;

    public:
        /**
         * @brief Initialize input system with GLFW window
         */
        void Initialize(GLFWwindow *window);

        /**
         * @brief Shutdown input system
         */
        void Shutdown();

        /**
         * @brief Update input state (call once per frame)
         */
        void Update();

        /**
         * @brief Check if a key is currently pressed
         * @param key GLFW key code
         */
        bool IsKeyDown(int key) const;

        /**
         * @brief Check if a key was just pressed this frame
         * @param key GLFW key code
         */
        bool IsKeyPressed(int key) const;

        /**
         * @brief Check if a key was just released this frame
         * @param key GLFW key code
         */
        bool IsKeyReleased(int key) const;

        /**
         * @brief Check if a mouse button is currently pressed
         * @param button GLFW mouse button code
         */
        bool IsMouseButtonDown(int button) const;

        /**
         * @brief Check if a mouse button was just pressed this frame
         * @param button GLFW mouse button code
         */
        bool IsMouseButtonPressed(int button) const;

        /**
         * @brief Get current mouse position
         */
        void GetMousePosition(double &x, double &y) const;

        /**
         * @brief Get mouse position delta since last frame
         */
        void GetMouseDelta(double &dx, double &dy) const;

        /**
         * @brief Get mouse scroll delta
         */
        void GetScrollDelta(double &dx, double &dy) const;

        /**
         * @brief Get the GLFW window
         */
        GLFWwindow *GetWindow() const { return m_window; }

    private:
        InputManager() = default;

        ~InputManager() = default;

        static constexpr int MAX_KEYS = 512;
        static constexpr int MAX_MOUSE_BUTTONS = 8;

        GLFWwindow *m_window = nullptr;

        // Keyboard state
        std::array<bool, MAX_KEYS> m_keysCurrent{};
        std::array<bool, MAX_KEYS> m_keysPrevious{};

        // Mouse state
        std::array<bool, MAX_MOUSE_BUTTONS> m_mouseButtonsCurrent{};
        std::array<bool, MAX_MOUSE_BUTTONS> m_mouseButtonsPrevious{};

        double m_mouseX = 0.0;
        double m_mouseY = 0.0;
        double m_lastMouseX = 0.0;
        double m_lastMouseY = 0.0;

        double m_scrollDX = 0.0;
        double m_scrollDY = 0.0;

        bool m_initialized = false;
    };
} // namespace Artifact::Core
