#pragma once

#include "Engine/Math/Math.hpp"

namespace Engine::Graphics
{
    class Camera
    {
    public:
        explicit Camera(Math::Vec3 position = {0.0f, 0.0f, 3.0f});

        void moveForward(float amount);
        void moveRight(float amount);
        void moveUp(float amount);
        void rotate(float yawDelta, float pitchDelta);

        Math::Mat4 viewMatrix() const;
        const Math::Vec3& position() const { return m_position; }
        const Math::Vec3& front() const { return m_front; }

    private:
        void updateFront();

        Math::Vec3 m_position;
        Math::Vec3 m_front{0.0f, 0.0f, -1.0f};
        Math::Vec3 m_up{0.0f, 1.0f, 0.0f};
        float m_yaw = -90.0f;
        float m_pitch = 0.0f;
    };
}
