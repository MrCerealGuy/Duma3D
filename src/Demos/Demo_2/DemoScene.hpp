#pragma once

#include "Engine/Game/InteractionSystem.hpp"
#include "Engine/Game/World/ChunkData.hpp"

#include <array>

namespace Duma3D::Demos::Demo_2
{
    struct LevelState
    {
        std::array<bool, 6> doorsOpen{true, false, true, true, true, true};
        std::array<float, 6> doorOpenAmount{1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f};
        bool powerOn = false;
    };

    Engine::Game::World::ChunkData generateLevel(const LevelState& state);
    bool advanceDoors(LevelState& state, float deltaTime);
    void updateLampFlicker(Engine::Graphics::Scene::Scene& scene, float elapsedTime);
    void registerInteractions(Engine::Game::InteractionSystem& interactions, LevelState& state,
        Engine::Graphics::Camera& camera);
}
