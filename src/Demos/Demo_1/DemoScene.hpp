#pragma once

#include "Engine/Scene/Scene.hpp"

#include <cstdint>

namespace Duma3D::Demos::Demo_1
{
    constexpr float worldChunkSize = 64.0f;

    Engine::Scene::Scene makeDemoWorld(int centerChunkX, int centerChunkZ, int radius, std::uint32_t seed);
    float sampleDemoTerrainHeight(float x, float z, std::uint32_t seed);
}
