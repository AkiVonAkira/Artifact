#pragma once

#include <memory>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

namespace Artifact::Core {
    class LoggingManager {
    public:
        LoggingManager();

        ~LoggingManager();

        void initialize(const std::string &logDirectory, const std::string &logFileName = "artifact.log");

        std::shared_ptr<spdlog::logger> defaultLogger() const;

        std::shared_ptr<spdlog::logger> getLogger(const std::string &name) const;

        std::shared_ptr<spdlog::logger> createLogger(const std::string &name);

        void setLevel(spdlog::level::level_enum level);

        void flushOn(spdlog::level::level_enum level);

        void attachConsoleSink(bool enableColors = true);

        void attachFileSink(const std::string &filePath, size_t maxSize = 4 * 1024 * 1024, size_t maxFiles = 3);

        void addSink(const std::shared_ptr<spdlog::sinks::sink> &sink);

        void removeSink(const std::shared_ptr<spdlog::sinks::sink> &sink);

        void attachConsoleMirror(const std::shared_ptr<spdlog::sinks::sink> &sink);

        void removeConsoleMirror(const std::shared_ptr<spdlog::sinks::sink> &sink);

        const std::vector<std::shared_ptr<spdlog::sinks::sink> > &sinks() const;

    private:
        void rebuildDefaultLogger();

        std::vector<std::shared_ptr<spdlog::sinks::sink> > m_sinks;
        std::vector<std::shared_ptr<spdlog::sinks::sink> > m_consoleMirrors;
        std::shared_ptr<spdlog::logger> m_defaultLogger;
    };
}
