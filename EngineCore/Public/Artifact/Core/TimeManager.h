// Created by Artifact Engine Team on 2025-11-25.
// Time manager with singleton pattern

#pragma once

#include "Artifact/Core/Singleton.h"
#include <chrono>

namespace Artifact::Core {
    /**
     * @brief Centralized time management system
     *
     * Manages delta time, time scaling, frame counting, and timing queries.
     */
    class TimeManager : public Singleton<TimeManager> {
        friend class Singleton<TimeManager>;

    public:
        /**
         * @brief Initialize time manager
         */
        void Initialize();

        /**
         * @brief Shutdown time manager
         */
        void Shutdown();

        /**
         * @brief Update timing (call once per frame at the start)
         */
        void Update();

        /**
         * @brief Get delta time in seconds (scaled)
         */
        float DeltaTime() const { return m_deltaTime * m_timeScale; }

        /**
         * @brief Get unscaled delta time in seconds
         */
        float UnscaledDeltaTime() const { return m_deltaTime; }

        /**
         * @brief Get total elapsed time since initialization
         */
        float TotalTime() const { return m_totalTime; }

        /**
         * @brief Get current frame count
         */
        uint64_t FrameCount() const { return m_frameCount; }

        /**
         * @brief Get current frames per second (averaged)
         */
        float FPS() const { return m_fps; }

        /**
         * @brief Set time scale (1.0 = normal speed, 0.5 = half speed, etc.)
         */
        void SetTimeScale(float scale) { m_timeScale = scale; }

        /**
         * @brief Get current time scale
         */
        float GetTimeScale() const { return m_timeScale; }

        /**
         * @brief Reset timing to zero
         */
        void Reset();

    private:
        TimeManager() = default;

        ~TimeManager() = default;

        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;

        TimePoint m_startTime;
        TimePoint m_lastFrameTime;

        float m_deltaTime = 0.0f;
        float m_totalTime = 0.0f;
        float m_timeScale = 1.0f;
        float m_fps = 0.0f;

        uint64_t m_frameCount = 0;

        // FPS calculation
        float m_fpsAccumulator = 0.0f;
        int m_fpsFrameCount = 0;

        bool m_initialized = false;
    };
} // namespace Artifact::Core
