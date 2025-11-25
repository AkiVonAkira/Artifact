// Created by Artifact Engine Team on 2025-11-25.
// Convenience macros for engine-wide usage

#pragma once

#include "Artifact/Core/LogManager.h"

// Logging macros (defined after LogManager is implemented)
#define LOG_TRACE(...)    ::Artifact::Core::LogManager::Instance().Trace(__VA_ARGS__)
#define LOG_DEBUG(...)    ::Artifact::Core::LogManager::Instance().Debug(__VA_ARGS__)
#define LOG_INFO(...)     ::Artifact::Core::LogManager::Instance().Info(__VA_ARGS__)
#define LOG_WARN(...)     ::Artifact::Core::LogManager::Instance().Warn(__VA_ARGS__)
#define LOG_ERROR(...)    ::Artifact::Core::LogManager::Instance().Error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Artifact::Core::LogManager::Instance().Critical(__VA_ARGS__)

// Manager access shorthand (optional convenience)
#define LOG_MANAGER()     ::Artifact::Core::LogManager::Instance()
#define SCENE_MANAGER()   ::Artifact::Scene::SceneManager::Instance()
#define ASSET_MANAGER()   ::Artifact::Assets::AssetManager::Instance()
#define RENDER_MANAGER()  ::Artifact::Rendering::RenderManager::Instance()
#define INPUT_MANAGER()   ::Artifact::Core::InputManager::Instance()
#define TIME_MANAGER()    ::Artifact::Core::TimeManager::Instance()
#define SCRIPT_MANAGER()  ::Artifact::Scripting::ScriptManager::Instance()
#define PHYSICS_MANAGER() ::Artifact::Physics::PhysicsManager::Instance()
