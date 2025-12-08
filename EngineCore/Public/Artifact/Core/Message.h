#pragma once

#include <memory>
#include <string>
#include <typeindex>
#include <chrono>

namespace Artifact::Core {

    enum class MessagePriority {
        Low = 0,
        Normal = 1,
        High = 2,
        Critical = 3
    };

    /// Base class for all messages in the engine
    class Message {
    public:
        virtual ~Message() = default;

        [[nodiscard]] virtual std::type_index getType() const = 0;
        [[nodiscard]] virtual std::string getName() const = 0;

        MessagePriority priority{MessagePriority::Normal};
        std::chrono::steady_clock::time_point timestamp;

        Message() : timestamp(std::chrono::steady_clock::now()) {}
    };

    /// Helper macro to define message types
    #define DEFINE_MESSAGE_TYPE(ClassName) \
        [[nodiscard]] std::type_index getType() const override { \
            return std::type_index(typeid(ClassName)); \
        } \
        [[nodiscard]] std::string getName() const override { \
            return #ClassName; \
        }

    // ============================================================================
    // Common Message Types
    // ============================================================================

    class AssetLoadedMessage : public Message {
    public:
        DEFINE_MESSAGE_TYPE(AssetLoadedMessage)

        std::string assetPath;
        bool success{false};

        explicit AssetLoadedMessage(std::string path, bool loaded = true)
            : assetPath(std::move(path)), success(loaded) {}
    };

    class EntityCreatedMessage : public Message {
    public:
        DEFINE_MESSAGE_TYPE(EntityCreatedMessage)

        uint32_t entityId{0};

        explicit EntityCreatedMessage(uint32_t id) : entityId(id) {}
    };

    class EntityDestroyedMessage : public Message {
    public:
        DEFINE_MESSAGE_TYPE(EntityDestroyedMessage)

        uint32_t entityId{0};

        explicit EntityDestroyedMessage(uint32_t id) : entityId(id) {}
    };

    class SceneLoadedMessage : public Message {
    public:
        DEFINE_MESSAGE_TYPE(SceneLoadedMessage)

        std::string scenePath;

        explicit SceneLoadedMessage(std::string path) : scenePath(std::move(path)) {}
    };

    class ShaderReloadedMessage : public Message {
    public:
        DEFINE_MESSAGE_TYPE(ShaderReloadedMessage)

        std::string shaderPath;
        bool success{false};

        explicit ShaderReloadedMessage(std::string path, bool reloaded = true)
            : shaderPath(std::move(path)), success(reloaded) {}
    };

    class InputMessage : public Message {
    public:
        DEFINE_MESSAGE_TYPE(InputMessage)

        int key{-1};
        int action{-1};
        int mods{0};
    };

} // namespace Artifact::Core

