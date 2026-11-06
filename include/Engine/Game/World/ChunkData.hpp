#pragma once

#include "Engine/Game/Collision/CollisionBox.hpp"
#include "Engine/Graphics/Scene/Scene.hpp"

#include <vector>
#include <cmath>

namespace Engine::Game::World
{
    // A generated piece of game world: renderable content plus game collision data.
    struct ChunkData
    {
        Graphics::Scene::Scene graphics;
        std::vector<Collision::Box> colliders;

        void addBoxCollider(Math::Vec3 center, Math::Vec3 size)
        {
            const Math::Vec3 half{std::abs(size.x) * 0.5f, std::abs(size.y) * 0.5f, std::abs(size.z) * 0.5f};
            colliders.push_back({
                {center.x - half.x, center.y - half.y, center.z - half.z},
                {center.x + half.x, center.y + half.y, center.z + half.z}
            });
        }
    };
}
