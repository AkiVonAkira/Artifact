#include "Artifact/Core/PlayModeController.h"

namespace Artifact::Core {

void PlayModeController::play() {
    if (m_state == State::Playing) {
        return;
    }
    m_state = State::Playing;
    m_paused = false;
    if (m_stateChanged) {
        m_stateChanged(m_state);
    }
}

void PlayModeController::stop() {
    if (m_state == State::Edit) {
        return;
    }
    m_state = State::Edit;
    m_paused = false;
    if (m_stateChanged) {
        m_stateChanged(m_state);
    }
}

void PlayModeController::pause(bool isPaused) {
    if (m_state != State::Playing) {
        return;
    }
    m_paused = isPaused;
}

void PlayModeController::step() {
    if (m_state != State::Playing) {
        return;
    }
    m_paused = true;
}

PlayModeController::State PlayModeController::state() const {
    return m_state;
}

bool PlayModeController::paused() const {
    return m_paused;
}

void PlayModeController::onStateChanged(StateChangedCallback callback) {
    m_stateChanged = std::move(callback);
}

} // namespace Artifact::Core

