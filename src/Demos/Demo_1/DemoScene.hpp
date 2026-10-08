#pragma once

#include "Engine/World/ChunkWorld.hpp"

#include <cstdint>

namespace Duma3D::Demos::Demo_1
{
    inline constexpr float worldChunkSize = Engine::World::defaultChunkSize;

    Engine::Scene::Scene generateDemoChunk(Engine::World::ChunkCoordinate coordinate, std::uint32_t seed);
    float sampleDemoTerrainHeight(float x, float z, std::uint32_t seed);
}
