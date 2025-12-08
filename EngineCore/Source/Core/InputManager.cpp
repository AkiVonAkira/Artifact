#include "Artifact/Core/InputManager.h"
#include <GLFW/glfw3.h>
#include <algorithm>

namespace Artifact::Core {
    void InputManager::Initialize(GLFWwindow *window) {
        if (m_initialized) {
            return;
        }

        m_window = window;

        // Initialize arrays to false
        std::fill(m_keysCurrent.begin(), m_keysCurrent.end(), false);
        std::fill(m_keysPrevious.begin(), m_keysPrevious.end(), false);
        std::fill(m_mouseButtonsCurrent.begin(), m_mouseButtonsCurrent.end(), false);
        std::fill(m_mouseButtonsPrevious.begin(), m_mouseButtonsPrevious.end(), false);

        // Get initial mouse position
        glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;

        m_initialized = true;
    }

    void InputManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        m_window = nullptr;
        m_initialized = false;
    }

    void InputManager::Update() {
        if (!m_initialized || !m_window) {
            return;
        }

        // Copy current state to previous
        m_keysPrevious = m_keysCurrent;
        m_mouseButtonsPrevious = m_mouseButtonsCurrent;

        // Update keyboard state
        for (int key = 0; key < MAX_KEYS; ++key) {
            int state = glfwGetKey(m_window, key);
            m_keysCurrent[key] = (state == GLFW_PRESS);
        }

        // Update mouse button state
        for (int button = 0; button < MAX_MOUSE_BUTTONS; ++button) {
            int state = glfwGetMouseButton(m_window, button);
            m_mouseButtonsCurrent[button] = (state == GLFW_PRESS);
        }

        // Update mouse position
        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;
        glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);

        // Reset scroll delta (this would need callback support for proper implementation)
        m_scrollDX = 0.0;
        m_scrollDY = 0.0;
    }

    bool InputManager::IsKeyDown(int key) const {
        if (!m_initialized || key < 0 || key >= MAX_KEYS) {
            return false;
        }
        return m_keysCurrent[key];
    }

    bool InputManager::IsKeyPressed(int key) const {
        if (!m_initialized || key < 0 || key >= MAX_KEYS) {
            return false;
        }
        return m_keysCurrent[key] && !m_keysPrevious[key];
    }

    bool InputManager::IsKeyReleased(int key) const {
        if (!m_initialized || key < 0 || key >= MAX_KEYS) {
            return false;
        }
        return !m_keysCurrent[key] && m_keysPrevious[key];
    }

    bool InputManager::IsMouseButtonDown(int button) const {
        if (!m_initialized || button < 0 || button >= MAX_MOUSE_BUTTONS) {
            return false;
        }
        return m_mouseButtonsCurrent[button];
    }

    bool InputManager::IsMouseButtonPressed(int button) const {
        if (!m_initialized || button < 0 || button >= MAX_MOUSE_BUTTONS) {
            return false;
        }
        return m_mouseButtonsCurrent[button] && !m_mouseButtonsPrevious[button];
    }

    void InputManager::GetMousePosition(double &x, double &y) const {
        x = m_mouseX;
        y = m_mouseY;
    }

    void InputManager::GetMouseDelta(double &dx, double &dy) const {
        dx = m_mouseX - m_lastMouseX;
        dy = m_mouseY - m_lastMouseY;
    }

    void InputManager::GetScrollDelta(double &dx, double &dy) const {
        dx = m_scrollDX;
        dy = m_scrollDY;
    }
} // namespace Artifact::Core
