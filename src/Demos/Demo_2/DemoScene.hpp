#pragma once

#include "Engine/Game/InteractionSystem.hpp"
#include "Engine/Game/World/ChunkData.hpp"

#include <array>

namespace Duma3D::Demos::Demo_2
{
    struct LevelState
    {
        std::array<bool, 6> doorsOpen{true, false, true, true, true, true};
        bool powerOn = false;
    };

    Engine::Game::World::ChunkData generateLevel(const LevelState& state);
    void registerInteractions(Engine::Game::InteractionSystem& interactions, LevelState& state,
        Engine::Graphics::Camera& camera);
}
