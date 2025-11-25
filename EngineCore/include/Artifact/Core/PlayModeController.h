#pragma once

#include <functional>

namespace Artifact::Core {
    class PlayModeController {
    public:
        enum class State {
            Edit,
            Playing
        };

        using StateChangedCallback = std::function<void(State)>;

        void play();

        void stop();

        void pause(bool isPaused);

        void step();

        State state() const;

        bool paused() const;

        void onStateChanged(StateChangedCallback callback);

    private:
        State m_state{State::Edit};
        bool m_paused{false};
        StateChangedCallback m_stateChanged;
    };
} // namespace Artifact::Core
