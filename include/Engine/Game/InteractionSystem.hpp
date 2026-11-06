#pragma once

#include "Engine/Graphics/Camera.hpp"

#include <functional>
#include <string>
#include <vector>

namespace Engine::Game
{
    struct InteractionResult
    {
        bool activated = false;
        bool worldChanged = false;
        std::wstring message;
    };

    struct Interactable
    {
        Math::Vec3 position{};
        float range = 2.4f;
        std::wstring prompt;
        std::function<InteractionResult()> activate;
    };

    // Selects the nearest usable target in front of the player and invokes it.
    class InteractionSystem
    {
    public:
        void clear() { m_targets.clear(); }
        void add(Interactable interactable);
        const Interactable* target(const Graphics::Camera& camera) const;
        InteractionResult interact(const Graphics::Camera& camera) const;

    private:
        std::vector<Interactable> m_targets;
    };
}
