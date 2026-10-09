#pragma once

#include "Engine/Graphics/Camera.hpp"
#include "Engine/Scene/Scene.hpp"

#include <functional>

namespace Engine::Physics
{
    struct CharacterControllerConfig
    {
        float radius = 0.28f;
        float height = 1.8f;
        float eyeHeight = 1.7f;
        float gravity = 18.0f;
        float jumpSpeed = 6.0f;
    };

    using GroundHeightSampler = std::function<float(float worldX, float worldZ)>;

    // Kinematic first-person character movement over sampled terrain and scene box colliders.
    class CharacterController
    {
    public:
        explicit CharacterController(CharacterControllerConfig config = {});

        void update(
            const Scene::Scene& scene,
            Graphics::Camera& camera,
            float forwardDistance,
            float rightDistance,
            float deltaTime,
            bool jumpRequested,
            const GroundHeightSampler& sampleGroundHeight
        );

        void placeOnGround(Graphics::Camera& camera, const GroundHeightSampler& sampleGroundHeight) const;
        void resetVerticalVelocity() { m_verticalVelocity = 0.0f; }

    private:
        bool collidesWithScene(const Scene::Scene& scene, Math::Vec3 position) const;

        CharacterControllerConfig m_config;
        float m_verticalVelocity = 0.0f;
    };
}
