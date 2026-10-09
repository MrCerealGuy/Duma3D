#include "Engine/Physics/CharacterController.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Engine::Physics
{
    CharacterController::CharacterController(CharacterControllerConfig config)
        : m_config(config)
    {
        if (!std::isfinite(m_config.radius) || m_config.radius <= 0.0f ||
            !std::isfinite(m_config.height) || m_config.height <= 0.0f ||
            !std::isfinite(m_config.eyeHeight) || m_config.eyeHeight < 0.0f ||
            m_config.eyeHeight > m_config.height ||
            !std::isfinite(m_config.gravity) || m_config.gravity < 0.0f ||
            !std::isfinite(m_config.jumpSpeed) || m_config.jumpSpeed < 0.0f)
            throw std::invalid_argument("CharacterController received an invalid configuration");
    }

    bool CharacterController::collidesWithScene(const Scene::Scene& scene, Math::Vec3 position) const
    {
        const float feet = position.y - m_config.eyeHeight;
        const float head = feet + m_config.height;
        for (const Scene::CollisionBox& box : scene.colliders())
        {
            if (head <= box.minimum.y || feet >= box.maximum.y)
                continue;

            const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
            const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
            const float offsetX = position.x - closestX;
            const float offsetZ = position.z - closestZ;
            if (offsetX * offsetX + offsetZ * offsetZ < m_config.radius * m_config.radius)
                return true;
        }
        return false;
    }

    void CharacterController::update(
        const Scene::Scene& scene,
        Graphics::Camera& camera,
        float forwardDistance,
        float rightDistance,
        float deltaTime,
        bool jumpRequested,
        const GroundHeightSampler& sampleGroundHeight
    )
    {
        if (!sampleGroundHeight || !std::isfinite(deltaTime) || deltaTime < 0.0f)
            return;

        const Math::Vec3 startingPosition = camera.position();
        camera.moveOnGround(forwardDistance, 0.0f);
        if (collidesWithScene(scene, camera.position()))
            camera.setPosition(startingPosition);

        const Math::Vec3 afterForward = camera.position();
        camera.moveOnGround(0.0f, rightDistance);
        if (collidesWithScene(scene, camera.position()))
            camera.setPosition(afterForward);

        Math::Vec3 position = camera.position();
        const float groundHeight = sampleGroundHeight(position.x, position.z);
        float feet = position.y - m_config.eyeHeight;
        const bool onGround = feet <= groundHeight + 0.02f && m_verticalVelocity <= 0.0f;
        if (jumpRequested && onGround)
            m_verticalVelocity = m_config.jumpSpeed;

        m_verticalVelocity -= m_config.gravity * deltaTime;
        float nextFeet = feet + m_verticalVelocity * deltaTime;
        bool hitCeiling = false;
        if (m_verticalVelocity > 0.0f)
        {
            for (const Scene::CollisionBox& box : scene.colliders())
            {
                const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
                const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
                const float offsetX = position.x - closestX;
                const float offsetZ = position.z - closestZ;
                if (offsetX * offsetX + offsetZ * offsetZ >= m_config.radius * m_config.radius ||
                    feet + m_config.height > box.minimum.y ||
                    nextFeet + m_config.height <= box.minimum.y)
                    continue;

                nextFeet = box.minimum.y - m_config.height;
                m_verticalVelocity = 0.0f;
                hitCeiling = true;
                break;
            }
        }
        if (!hitCeiling && nextFeet <= groundHeight)
        {
            nextFeet = groundHeight;
            m_verticalVelocity = 0.0f;
        }

        position.y = nextFeet + m_config.eyeHeight;
        camera.setPosition(position);
    }

    void CharacterController::placeOnGround(
        Graphics::Camera& camera,
        const GroundHeightSampler& sampleGroundHeight
    ) const
    {
        if (!sampleGroundHeight)
            return;
        Math::Vec3 position = camera.position();
        position.y = sampleGroundHeight(position.x, position.z) + m_config.eyeHeight;
        camera.setPosition(position);
    }
}
