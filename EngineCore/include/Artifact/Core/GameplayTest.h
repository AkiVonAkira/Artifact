#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#define ARTIFACT_CONCAT_IMPL(a, b) a##b
#define ARTIFACT_CONCAT(a, b) ARTIFACT_CONCAT_IMPL(a, b)

namespace Artifact {
    namespace Scene {
        class SceneService;
    }

    namespace Scripting {
        class LuaRuntimeSystem;
    }

    namespace Core {
        class PlayModeController;
    }
}

namespace Artifact::Core {
    struct GameplayTestContext {
        Scene::SceneService &scene;
        Scripting::LuaRuntimeSystem &luaRuntime;
        PlayModeController &playMode;
    };

    struct GameplayTestResult {
        bool passed = true;
        std::string message;
        double durationSeconds = 0.0;
    };

    class IGameplayTest {
    public:
        virtual ~IGameplayTest() = default;

        virtual std::string_view name() const = 0;

        virtual std::string_view description() const = 0;

        virtual GameplayTestResult run(GameplayTestContext &context) = 0;
    };

    class GameplayTestRegistry {
    public:
        using GameplayTestFactory = std::function<std::unique_ptr<IGameplayTest>()>;

        struct Entry {
            std::string identifier;
            std::string displayName;
            std::string description;
            GameplayTestFactory factory;
        };

        static GameplayTestRegistry &instance();

        void registerTest(std::string identifier,
                          std::string displayName,
                          std::string description,
                          GameplayTestFactory factory);

        const std::vector<Entry> &tests() const;

    private:
        GameplayTestRegistry() = default;

        mutable std::mutex mutex_;
        std::vector<Entry> entries_;
    };

    class GameplayTestRegistration {
    public:
        GameplayTestRegistration(std::string identifier,
                                 std::string displayName,
                                 std::string description,
                                 GameplayTestRegistry::GameplayTestFactory factory);
    };

#define ARTIFACT_REGISTER_GAMEPLAY_TEST(TestType, Identifier, DisplayName, Description) \
    namespace { \
        ::Artifact::Core::GameplayTestRegistration ARTIFACT_CONCAT(TestType, _GameplayTestRegistration){ \
            Identifier, DisplayName, Description, [] { \
                return std::make_unique<TestType>(); \
            } \
        }; \
    }
} // namespace Artifact::Core
