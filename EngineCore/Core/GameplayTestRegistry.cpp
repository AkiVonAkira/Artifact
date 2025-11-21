#include "Artifact/Core/Application.h"
#include "Artifact/Core/GameplayTest.h"

#include <algorithm>
#include <mutex>
#include <utility>

namespace Artifact::Core {

GameplayTestRegistry& GameplayTestRegistry::instance() {
    static GameplayTestRegistry registry;
    return registry;
}

void GameplayTestRegistry::registerTest(std::string identifier,
                                        std::string displayName,
                                        std::string description,
                                        GameplayTestFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(entries_.begin(), entries_.end(), [&](const Entry& entry) {
        return entry.identifier == identifier;
    });
    if (it != entries_.end()) {
        *it = Entry{std::move(identifier), std::move(displayName), std::move(description), std::move(factory)};
    } else {
        entries_.push_back(Entry{std::move(identifier), std::move(displayName), std::move(description), std::move(factory)});
    }
}

const std::vector<GameplayTestRegistry::Entry>& GameplayTestRegistry::tests() const {
    return entries_;
}

GameplayTestRegistration::GameplayTestRegistration(std::string identifier,
                                                   std::string displayName,
                                                   std::string description,
                                                   GameplayTestRegistry::GameplayTestFactory factory) {
    GameplayTestRegistry::instance().registerTest(std::move(identifier),
                                                 std::move(displayName),
                                                 std::move(description),
                                                 std::move(factory));
}

} // namespace Artifact::Core
