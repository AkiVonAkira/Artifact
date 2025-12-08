#include "Artifact/Core/MessageQueue.h"
#include <algorithm>

namespace Artifact::Core {

    void MessageQueue::post(std::unique_ptr<Message> message) {
        if (!message) {
            return;
        }

        std::lock_guard<std::mutex> lock(m_mutex);

        // Insert based on priority
        auto it = std::lower_bound(
            m_queue.begin(),
            m_queue.end(),
            message,
            [](const std::unique_ptr<Message>& a, const std::unique_ptr<Message>& b) {
                return static_cast<int>(a->priority) > static_cast<int>(b->priority);
            }
        );

        m_queue.insert(it, std::move(message));
    }

    void MessageQueue::process() {
        std::deque<std::unique_ptr<Message>> messagesToProcess;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            messagesToProcess.swap(m_queue);
        }

        for (const auto& message : messagesToProcess) {
            if (message) {
                dispatchMessage(*message);
            }
        }
    }

    void MessageQueue::clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.clear();
    }

    size_t MessageQueue::size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    bool MessageQueue::empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    void MessageQueue::dispatchMessage(const Message& message) {
        std::vector<MessageHandler> handlersToCall;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_handlers.find(message.getType());
            if (it != m_handlers.end()) {
                handlersToCall = it->second;
            }
        }

        for (const auto& handler : handlersToCall) {
            handler(message);
        }
    }

} // namespace Artifact::Core

