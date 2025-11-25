// Created by Artifact Engine Team on 2025-11-25.
// Singleton pattern base class using CRTP (Curiously Recurring Template Pattern)
// Inspired by RNGOEngine-Core by Oskar Norberg

#pragma once

namespace Artifact::Core {
    /**
     * @brief Thread-safe singleton base class using CRTP pattern
     * @tparam T The derived class type
     *
     * Usage:
     *   class MyManager : public Singleton<MyManager> {
     *       friend class Singleton<MyManager>;
     *   private:
     *       MyManager() = default;
     *   public:
     *       void DoSomething();
     *   };
     *
     *   // Access:
     *   MyManager::Instance().DoSomething();
     */
    template<typename T>
    class Singleton {
    public:
        /**
         * @brief Get the singleton instance
         * @return Reference to the singleton instance
         */
        static T &Instance() {
            static T instance;
            return instance;
        }

        // Delete copy/move operations
        Singleton(const Singleton &) = delete;

        Singleton &operator=(const Singleton &) = delete;

        Singleton(Singleton &&) = delete;

        Singleton &operator=(Singleton &&) = delete;

    protected:
        Singleton() = default;

        virtual ~Singleton() = default;
    };
} // namespace Artifact::Core
