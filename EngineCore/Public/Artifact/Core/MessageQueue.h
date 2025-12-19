#pragma once

#include "Message.h"
#include "Artifact/Core/Singleton.h"
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Artifact::Core {

    using MessageHandler = std::function<void(const Message&)>;

    /// Thread-safe message queue for async communication between engine subsystems
    class MessageQueue : public Singleton<MessageQueue> {
    public:
        MessageQueue() = default;
        ~MessageQueue() override = default;

        // Disable copy
        MessageQueue(const MessageQueue&) = delete;
        MessageQueue& operator=(const MessageQueue&) = delete;

        /// Post a message to the queue
        void post(std::unique_ptr<Message> message);

        /// Subscribe to a specific message type
        template<typename T>
        void subscribe(MessageHandler handler) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handlers[std::type_index(typeid(T))].push_back(std::move(handler));
        }

        /// Process all pending messages
        void process();

        /// Clear all pending messages
        void clear();

        /// Get the number of pending messages
        [[nodiscard]] size_t size() const;

        /// Check if queue is empty
        [[nodiscard]] bool empty() const;

    private:
        mutable std::mutex m_mutex;
        std::deque<std::unique_ptr<Message>> m_queue;
        std::unordered_map<std::type_index, std::vector<MessageHandler>> m_handlers;

        void dispatchMessage(const Message& message);
    };

} // namespace Artifact::Core

