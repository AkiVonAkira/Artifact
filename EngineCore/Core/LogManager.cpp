#include "Artifact/Core/LogManager.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <filesystem>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace Artifact::Core {
    // ============================================================================
    // VectorSink Implementation
    // ============================================================================

    VectorSink::VectorSink(size_t maxMessages)
        : m_maxMessages(maxMessages) {
        m_logs.reserve(maxMessages);
    }

    void VectorSink::Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        m_logs.clear();
    }

    void VectorSink::sink_it_(const spdlog::details::log_msg &msg) {
        // Convert timestamp to string
        auto time = std::chrono::system_clock::to_time_t(msg.time);
        std::tm tm_time;
#ifdef _WIN32
        localtime_s(&tm_time, &time);
#else
        localtime_r(&time, &tm_time);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_time, "%H:%M:%S");

        LogEntry entry{
            .level = ConvertLevel(msg.level),
            .message = std::string(msg.payload.data(), msg.payload.size()),
            .timestamp = oss.str()
        };

        // Circular buffer behavior: remove oldest if at capacity
        if (m_logs.size() >= m_maxMessages) {
            m_logs.erase(m_logs.begin());
        }

        m_logs.push_back(std::move(entry));
    }

    LogEntry::Level VectorSink::ConvertLevel(spdlog::level::level_enum level) {
        switch (level) {
            case spdlog::level::trace: return LogEntry::Level::Trace;
            case spdlog::level::debug: return LogEntry::Level::Debug;
            case spdlog::level::info: return LogEntry::Level::Info;
            case spdlog::level::warn: return LogEntry::Level::Warn;
            case spdlog::level::err: return LogEntry::Level::Error;
            case spdlog::level::critical: return LogEntry::Level::Critical;
            default: return LogEntry::Level::Info;
        }
    }

    // ============================================================================
    // LogManager Implementation
    // ============================================================================

    void LogManager::Initialize(const std::string &logDirectory, const std::string &logFileName) {
        if (m_initialized) {
            return;
        }

        // Create log directory
        std::filesystem::create_directories(logDirectory);

        // Clear previous log files for a fresh run: remove .log files that start with the base name
        try {
            const std::string baseName = std::filesystem::path(logFileName).stem().string();
            for (auto &entry: std::filesystem::directory_iterator(logDirectory)) {
                if (!entry.is_regular_file()) continue;
                const auto fname = entry.path().filename().string();
                if (entry.path().extension() == ".log") {
                    if (fname == logFileName || fname.rfind(baseName, 0) == 0) {
                        std::error_code rmec;
                        std::filesystem::remove(entry.path(), rmec);
                        // ignore errors; cleanup best-effort
                    }
                }
            }
        } catch (...) {
            // best-effort cleanup; do not fail initialization on cleanup errors
        }

        // Create vector sink for ImGui console
        m_vectorSink = std::make_shared<VectorSink>(10000);
        // Ensure in-memory logs are empty for this run
        m_vectorSink->Clear();
        m_sinks.push_back(m_vectorSink);

        // Create daily file sink
        const auto filePath = std::filesystem::path(logDirectory) / logFileName;
        auto fileSink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            filePath.string(),
            0, 0, // Rotate at midnight
            false, // Don't truncate
            4 // Keep 4 files
        );
        fileSink->set_level(spdlog::level::trace);
        m_sinks.push_back(fileSink);

        // Create colored console sink
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        m_sinks.push_back(consoleSink);

        // Set default pattern for all sinks
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        RebuildDefaultLogger();

        // Ensure default logger and sinks have proper flush behavior
        if (m_defaultLogger) {
            m_defaultLogger->set_level(spdlog::level::debug);
            m_defaultLogger->flush_on(spdlog::level::trace);
        }

        m_initialized = true;
        if (m_defaultLogger) {
            m_defaultLogger->info("LogManager initialized");
            m_defaultLogger->flush();
        }
    }

    void LogManager::Shutdown() {
        if (!m_initialized) {
            return;
        }

        if (m_defaultLogger) {
            m_defaultLogger->info("LogManager shutting down");
        }

        m_defaultLogger.reset();
        m_sinks.clear();
        m_vectorSink.reset();

        spdlog::shutdown();
        m_initialized = false;
    }

    std::shared_ptr<spdlog::logger> LogManager::GetLogger(const std::string &name) const {
        return spdlog::get(name);
    }

    std::shared_ptr<spdlog::logger> LogManager::CreateLogger(const std::string &name) {
        auto logger = spdlog::get(name);
        if (logger) {
            return logger;
        }

        logger = std::make_shared<spdlog::logger>(name, m_sinks.begin(), m_sinks.end());
        logger->set_level(m_defaultLogger ? m_defaultLogger->level() : spdlog::level::info);
        spdlog::register_logger(logger);

        return logger;
    }

    void LogManager::SetLevel(spdlog::level::level_enum level) {
        if (m_defaultLogger) {
            m_defaultLogger->set_level(level);
        }
        spdlog::set_level(level);
    }

    void LogManager::FlushOn(spdlog::level::level_enum level) {
        if (m_defaultLogger) {
            m_defaultLogger->flush_on(level);
        }
        spdlog::flush_on(level);
    }

    void LogManager::AttachSink(const std::shared_ptr<spdlog::sinks::sink> &sink) {
        m_sinks.push_back(sink);
        RebuildDefaultLogger();
    }

    void LogManager::RebuildDefaultLogger() {
        const std::string loggerName = "Artifact";

        // Unregister old logger if exists
        if (m_defaultLogger) {
            spdlog::drop(loggerName);
        }

        // If there are no sinks, ensure we at least have a console sink so spdlog calls don't crash
        if (m_sinks.empty()) {
            auto fallbackConsole = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            m_sinks.push_back(fallbackConsole);
        }

        // Create new logger with all sinks
        m_defaultLogger = std::make_shared<spdlog::logger>(
            loggerName,
            m_sinks.begin(),
            m_sinks.end()
        );

        // Give each sink a default formatter if none set
        for (auto &sink: m_sinks) {
            sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        }

        // Register as default
        spdlog::register_logger(m_defaultLogger);
        spdlog::set_default_logger(m_defaultLogger);
    }

    void LogManager::Flush() {
        try {
            // Flush default logger if present
            if (m_defaultLogger) {
                m_defaultLogger->flush();
            }
            // Also flush all registered loggers
            spdlog::apply_all([](const std::shared_ptr<spdlog::logger> &logger) {
                logger->flush();
            });
        } catch (...) {
            // Swallow any exceptions during flush to avoid crashing
        }
    }
} // namespace Artifact::Core
