#pragma once

#include "Engine/Game/Physics/CharacterController.hpp"

#include <vector>

namespace Engine::Game
{
    enum class MovementMode { Walk, Fly };

    struct FirstPersonInput
    {
        float forward = 0.0f;
        float right = 0.0f;
        float vertical = 0.0f;
        bool sprint = false;
        bool jumpPressed = false;
    };

    struct FirstPersonControllerConfig
    {
        float moveSpeed = 3.0f;
        float sprintMultiplier = 2.0f;
    };

    // Reusable game-side first-person movement. Platform input is translated to
    // FirstPersonInput by the host application; rendering remains in Graphics.
    class FirstPersonController
    {
    public:
        explicit FirstPersonController(FirstPersonControllerConfig config = {});

        void update(
            const std::vector<Collision::Box>& colliders,
            Graphics::Camera& camera,
            const FirstPersonInput& input,
            float deltaTime,
            const Physics::GroundHeightSampler& sampleGroundHeight
        );
        void toggleMode(Graphics::Camera& camera, const Physics::GroundHeightSampler& sampleGroundHeight);
        void placeOnGround(Graphics::Camera& camera, const Physics::GroundHeightSampler& sampleGroundHeight) const;
        MovementMode mode() const { return m_mode; }

    private:
        FirstPersonControllerConfig m_config;
        Physics::CharacterController m_character;
        MovementMode m_mode = MovementMode::Walk;
    };
}
