#pragma once

#include "Engine/Game/World/ChunkWorld.hpp"

#include <cstdint>

namespace Duma3D::Demos::Demo_1
{
    inline constexpr float worldChunkSize = Engine::Game::World::defaultChunkSize;

    Engine::Game::World::ChunkData generateDemoChunk(Engine::Game::World::ChunkCoordinate coordinate, std::uint32_t seed);
    float sampleDemoTerrainHeight(float x, float z, std::uint32_t seed);
}
