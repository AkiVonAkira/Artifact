#include "Artifact/Core/LoggingManager.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <filesystem>
#include <algorithm>

namespace Artifact::Core {
    LoggingManager::LoggingManager() = default;

    LoggingManager::~LoggingManager() = default;

    void LoggingManager::initialize(const std::string &logDirectory, const std::string &logFileName) {
        std::filesystem::create_directories(logDirectory);
        const auto filePath = std::filesystem::path(logDirectory) / logFileName;
        attachFileSink(filePath.string());
        attachConsoleSink(true);
        rebuildDefaultLogger();
    }

    std::shared_ptr<spdlog::logger> LoggingManager::defaultLogger() const {
        return m_defaultLogger;
    }

    std::shared_ptr<spdlog::logger> LoggingManager::getLogger(const std::string &name) const {
        return spdlog::get(name);
    }

    std::shared_ptr<spdlog::logger> LoggingManager::createLogger(const std::string &name) {
        auto logger = spdlog::get(name);
        if (logger) {
            return logger;
        }
        logger = std::make_shared<spdlog::logger>(name, begin(m_sinks), end(m_sinks));
        spdlog::register_logger(logger);
        return logger;
    }

    void LoggingManager::setLevel(spdlog::level::level_enum level) {
        if (m_defaultLogger) {
            m_defaultLogger->set_level(level);
        }
        spdlog::set_level(level);
    }

    void LoggingManager::flushOn(spdlog::level::level_enum level) {
        if (m_defaultLogger) {
            m_defaultLogger->flush_on(level);
        }
        spdlog::flush_on(level);
    }

    void LoggingManager::attachConsoleSink(bool enableColors) {
        if (enableColors) {
            m_sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        } else {
            m_sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
        }
        rebuildDefaultLogger();
    }

    void LoggingManager::attachFileSink(const std::string &filePath, size_t maxSize, size_t maxFiles) {
        m_sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filePath, maxSize, maxFiles));
        rebuildDefaultLogger();
    }

    void LoggingManager::addSink(const std::shared_ptr<spdlog::sinks::sink> &sink) {
        m_sinks.push_back(sink);
        rebuildDefaultLogger();
    }

    void LoggingManager::removeSink(const std::shared_ptr<spdlog::sinks::sink> &sink) {
        const auto it = std::find(m_sinks.begin(), m_sinks.end(), sink);
        if (it != m_sinks.end()) {
            m_sinks.erase(it);
            rebuildDefaultLogger();
        }
    }

    void LoggingManager::attachConsoleMirror(const std::shared_ptr<spdlog::sinks::sink> &sink) {
        if (!sink) {
            return;
        }
        m_consoleMirrors.push_back(sink);
        rebuildDefaultLogger();
    }

    void LoggingManager::removeConsoleMirror(const std::shared_ptr<spdlog::sinks::sink> &sink) {
        const auto it = std::find(m_consoleMirrors.begin(), m_consoleMirrors.end(), sink);
        if (it != m_consoleMirrors.end()) {
            m_consoleMirrors.erase(it);
            rebuildDefaultLogger();
        }
    }

    void LoggingManager::rebuildDefaultLogger() {
        if (m_sinks.empty() && m_consoleMirrors.empty()) {
            m_defaultLogger.reset();
            return;
        }
        std::vector<spdlog::sink_ptr> combined;
        combined.reserve(m_sinks.size() + m_consoleMirrors.size());
        combined.insert(combined.end(), m_sinks.begin(), m_sinks.end());
        combined.insert(combined.end(), m_consoleMirrors.begin(), m_consoleMirrors.end());
        m_defaultLogger = std::make_shared<spdlog::logger>("Artifact", combined.begin(), combined.end());
        m_defaultLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        spdlog::set_default_logger(m_defaultLogger);
    }
}
