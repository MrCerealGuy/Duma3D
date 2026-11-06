#include "Engine/Game/FirstPersonController.hpp"

#include <cmath>
#include <stdexcept>

namespace Engine::Game
{
    FirstPersonController::FirstPersonController(FirstPersonControllerConfig config)
        : m_config(config)
    {
        if (!std::isfinite(config.moveSpeed) || config.moveSpeed <= 0.0f ||
            !std::isfinite(config.sprintMultiplier) || config.sprintMultiplier < 1.0f)
            throw std::invalid_argument("FirstPersonController received an invalid configuration");
    }

    void FirstPersonController::update(
        const std::vector<Collision::Box>& colliders,
        Graphics::Camera& camera,
        const FirstPersonInput& input,
        float deltaTime,
        const Physics::GroundHeightSampler& sampleGroundHeight
    )
    {
        if (!std::isfinite(deltaTime) || deltaTime < 0.0f)
            return;
        float forward = input.forward;
        float right = input.right;
        float vertical = input.vertical;
        const float inputLength = std::sqrt(forward * forward + right * right +
            (m_mode == MovementMode::Fly ? vertical * vertical : 0.0f));
        if (inputLength > 1.0f)
        {
            forward /= inputLength;
            right /= inputLength;
            vertical /= inputLength;
        }
        const float speed = m_config.moveSpeed *
            (input.sprint ? m_config.sprintMultiplier : 1.0f) * deltaTime;
        if (m_mode == MovementMode::Fly)
        {
            camera.moveForward(forward * speed);
            camera.moveRight(right * speed);
            camera.moveUp(vertical * speed);
            return;
        }
        m_character.update(colliders, camera, forward * speed, right * speed, deltaTime,
            input.jumpPressed, sampleGroundHeight);
    }

    void FirstPersonController::toggleMode(
        Graphics::Camera& camera,
        const Physics::GroundHeightSampler& sampleGroundHeight
    )
    {
        m_mode = m_mode == MovementMode::Walk ? MovementMode::Fly : MovementMode::Walk;
        m_character.resetVerticalVelocity();
        if (m_mode == MovementMode::Walk)
            m_character.placeOnGround(camera, sampleGroundHeight);
    }

    void FirstPersonController::placeOnGround(
        Graphics::Camera& camera,
        const Physics::GroundHeightSampler& sampleGroundHeight
    ) const
    {
        if (m_mode == MovementMode::Walk)
            m_character.placeOnGround(camera, sampleGroundHeight);
    }
}
