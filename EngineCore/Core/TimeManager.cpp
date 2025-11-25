#include "Artifact/Core/TimeManager.h"
#include <algorithm>

namespace Artifact::Core {
    void TimeManager::Initialize() {
        if (m_initialized) {
            return;
        }

        m_startTime = Clock::now();
        m_lastFrameTime = m_startTime;
        m_deltaTime = 0.0f;
        m_totalTime = 0.0f;
        m_frameCount = 0;
        m_timeScale = 1.0f;
        m_fps = 0.0f;
        m_fpsAccumulator = 0.0f;
        m_fpsFrameCount = 0;

        m_initialized = true;
    }

    void TimeManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        m_initialized = false;
    }

    void TimeManager::Update() {
        if (!m_initialized) {
            return;
        }

        auto now = Clock::now();

        // Calculate delta time
        std::chrono::duration<float> deltaSeconds = now - m_lastFrameTime;
        m_deltaTime = deltaSeconds.count();

        // Clamp delta time to prevent large spikes (e.g., when debugging)
        m_deltaTime = std::min(m_deltaTime, 0.1f);

        // Update total time
        std::chrono::duration<float> totalSeconds = now - m_startTime;
        m_totalTime = totalSeconds.count();

        // Update frame count
        m_frameCount++;

        // Calculate FPS (averaged over 0.5 seconds)
        m_fpsAccumulator += m_deltaTime;
        m_fpsFrameCount++;

        if (m_fpsAccumulator >= 0.5f) {
            m_fps = static_cast<float>(m_fpsFrameCount) / m_fpsAccumulator;
            m_fpsAccumulator = 0.0f;
            m_fpsFrameCount = 0;
        }

        m_lastFrameTime = now;
    }

    void TimeManager::Reset() {
        m_startTime = Clock::now();
        m_lastFrameTime = m_startTime;
        m_deltaTime = 0.0f;
        m_totalTime = 0.0f;
        m_frameCount = 0;
        m_fpsAccumulator = 0.0f;
        m_fpsFrameCount = 0;
    }
} // namespace Artifact::Core
