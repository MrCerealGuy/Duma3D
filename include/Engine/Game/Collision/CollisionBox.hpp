#pragma once

#include "Engine/Math/Math.hpp"

namespace Engine::Game::Collision
{
    struct Box
    {
        Math::Vec3 minimum;
        Math::Vec3 maximum;
    };
}
