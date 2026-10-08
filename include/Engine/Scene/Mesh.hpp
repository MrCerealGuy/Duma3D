#pragma once

#include "Engine/Math/Math.hpp"

#include <vector>

namespace Engine::Scene
{
    struct Mesh
    {
        std::vector<Math::Vec3> vertices;

        static Mesh cube();
    };
}
