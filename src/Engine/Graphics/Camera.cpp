#include "Engine/Graphics/Camera.hpp"

#include <algorithm>
#include <cmath>

namespace Engine::Graphics
{
    Camera::Camera(Math::Vec3 position)
        : m_position(position)
    {
    }

    void Camera::moveForward(float amount)
    {
        m_position.x += m_front.x * amount;
        m_position.y += m_front.y * amount;
        m_position.z += m_front.z * amount;
    }

    void Camera::moveRight(float amount)
    {
        const Math::Vec3 right = Math::normalize(Math::cross(m_front, m_up));
        m_position.x += right.x * amount;
        m_position.y += right.y * amount;
        m_position.z += right.z * amount;
    }

    void Camera::moveUp(float amount)
    {
        m_position.y += amount;
    }

    void Camera::rotate(float yawDelta, float pitchDelta)
    {
        m_yaw += yawDelta;
        m_pitch = std::clamp(m_pitch + pitchDelta, -89.0f, 89.0f);
        updateFront();
    }

    Math::Mat4 Camera::viewMatrix() const
    {
        const Math::Vec3 center{
            m_position.x + m_front.x,
            m_position.y + m_front.y,
            m_position.z + m_front.z
        };
        return Math::lookAt(m_position, center, m_up);
    }

    void Camera::updateFront()
    {
        constexpr float degreesToRadians = 3.14159265359f / 180.0f;
        const float yaw = m_yaw * degreesToRadians;
        const float pitch = m_pitch * degreesToRadians;
        m_front = {
            std::cos(yaw) * std::cos(pitch),
            std::sin(pitch),
            std::sin(yaw) * std::cos(pitch)
        };
    }
}
