#pragma once

#include "Engine/Math/Math.hpp"

#include <vector>

namespace Engine::Scene
{
    struct Vertex
    {
        Math::Vec3 position;
        Math::Vec3 normal;
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;

        static Mesh cube();
    };
}
