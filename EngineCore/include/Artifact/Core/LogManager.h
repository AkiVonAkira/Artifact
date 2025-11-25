// Created by Artifact Engine Team on 2025-11-25.
// Logging manager with singleton pattern and ImGui integration
// Inspired by RNGOEngine-Core by Oskar Norberg

#pragma once

#include "Artifact/Core/Singleton.h"
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

namespace Artifact::Core {
    /**
     * @brief Log entry for ImGui console display
     */
    struct LogEntry {
        enum class Level {
            Trace,
            Debug,
            Info,
            Warn,
            Error,
            Critical
        };

        Level level;
        std::string message;
        std::string timestamp;
    };

    /**
     * @brief Custom spdlog sink that stores logs in a vector for ImGui display
     *
     * This sink maintains a circular buffer of log messages that can be displayed
     * in an ImGui console window. It automatically manages memory by limiting
     * the number of stored messages.
     */
    class VectorSink : public spdlog::sinks::base_sink<std::mutex> {
    public:
        explicit VectorSink(size_t maxMessages = 10000);

        /**
         * @brief Get all stored log entries
         */
        std::span<const LogEntry> GetLogs() const {
            return m_logs;
        }

        /**
         * @brief Clear all stored logs
         */
        void Clear();

    protected:
        void sink_it_(const spdlog::details::log_msg &msg) override;

        void flush_() override {
        }

    private:
        static LogEntry::Level ConvertLevel(spdlog::level::level_enum level);

        std::vector<LogEntry> m_logs;
        size_t m_maxMessages;
    };

    /**
     * @brief Centralized logging manager with singleton access
     *
     * Features:
     * - Daily rotating file logs
     * - Console output with colors
     * - Vector sink for ImGui console
     * - Multiple logger support
     * - Global log level control
     */
    class LogManager : public Singleton<LogManager> {
        friend class Singleton<LogManager>;

    public:
        /**
         * @brief Initialize the logging system
         * @param logDirectory Directory for log files
         * @param logFileName Base name for log files
         */
        void Initialize(const std::string &logDirectory, const std::string &logFileName = "artifact.log");

        /**
         * @brief Shutdown the logging system
         */
        void Shutdown();

        // Convenience logging methods
        template<typename... Args>
        void Trace(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            if (m_defaultLogger) {
                m_defaultLogger->trace(fmt, std::forward<Args>(args)...);
            }
        }

        template<typename... Args>
        void Debug(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            if (m_defaultLogger) {
                m_defaultLogger->debug(fmt, std::forward<Args>(args)...);
            }
        }

        template<typename... Args>
        void Info(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            if (m_defaultLogger) {
                m_defaultLogger->info(fmt, std::forward<Args>(args)...);
            }
        }

        template<typename... Args>
        void Warn(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            if (m_defaultLogger) {
                m_defaultLogger->warn(fmt, std::forward<Args>(args)...);
            }
        }

        template<typename... Args>
        void Error(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            if (m_defaultLogger) {
                m_defaultLogger->error(fmt, std::forward<Args>(args)...);
            }
        }

        template<typename... Args>
        void Critical(spdlog::format_string_t<Args...> fmt, Args &&... args) {
            if (m_defaultLogger) {
                m_defaultLogger->critical(fmt, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Get the default logger
         */
        std::shared_ptr<spdlog::logger> GetDefaultLogger() const { return m_defaultLogger; }

        /**
         * @brief Get a named logger
         */
        std::shared_ptr<spdlog::logger> GetLogger(const std::string &name) const;

        /**
         * @brief Create a new named logger
         */
        std::shared_ptr<spdlog::logger> CreateLogger(const std::string &name);

        /**
         * @brief Set global log level
         */
        void SetLevel(spdlog::level::level_enum level);

        /**
         * @brief Set flush level
         */
        void FlushOn(spdlog::level::level_enum level);

        /**
         * @brief Get the vector sink for ImGui console
         */
        std::shared_ptr<VectorSink> GetVectorSink() const { return m_vectorSink; }

        /**
         * @brief Attach a custom sink
         */
        void AttachSink(const std::shared_ptr<spdlog::sinks::sink> &sink);

        /**
         * @brief Flush all sinks
         */
        void Flush();

    private:
        LogManager() = default;

        ~LogManager() = default;

        void RebuildDefaultLogger();

        std::vector<std::shared_ptr<spdlog::sinks::sink> > m_sinks;
        std::shared_ptr<spdlog::logger> m_defaultLogger;
        std::shared_ptr<VectorSink> m_vectorSink;
        bool m_initialized = false;
    };
} // namespace Artifact::Core
