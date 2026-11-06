#include "Engine/Game/InteractionSystem.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace Engine::Game
{
    void InteractionSystem::add(Interactable interactable)
    {
        if (interactable.range <= 0.0f || !std::isfinite(interactable.range) || !interactable.activate)
            return;
        m_targets.push_back(std::move(interactable));
    }

    const Interactable* InteractionSystem::target(const Graphics::Camera& camera) const
    {
        const Math::Vec3 position = camera.position();
        const Math::Vec3 forward = camera.front();
        const Interactable* closest = nullptr;
        float closestDistance = std::numeric_limits<float>::max();
        for (const Interactable& interactable : m_targets)
        {
            const Math::Vec3 offset{
                interactable.position.x - position.x,
                interactable.position.y - position.y,
                interactable.position.z - position.z
            };
            const float distanceSquared = Math::dot(offset, offset);
            if (distanceSquared <= 1.0e-6f || distanceSquared > interactable.range * interactable.range)
                continue;
            const float distance = std::sqrt(distanceSquared);
            const float facing = Math::dot(forward, offset) / distance;
            if (facing < 0.45f || distance >= closestDistance)
                continue;
            closest = &interactable;
            closestDistance = distance;
        }
        return closest;
    }

    InteractionResult InteractionSystem::interact(const Graphics::Camera& camera) const
    {
        const Interactable* selected = target(camera);
        return selected ? selected->activate() : InteractionResult{};
    }
}
