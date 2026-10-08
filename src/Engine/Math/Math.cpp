#include "Engine/Math/Math.hpp"

#include <cmath>

namespace Engine::Math
{
    Vec3 normalize(Vec3 value)
    {
        const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
        if (length <= 0.0f)
            return {0.0f, 0.0f, 0.0f};

        return {value.x / length, value.y / length, value.z / length};
    }

    Vec3 cross(Vec3 a, Vec3 b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    float dot(Vec3 a, Vec3 b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Mat4 identity()
    {
        Mat4 result{};
        result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
        return result;
    }

    Mat4 translation(Vec3 offset)
    {
        Mat4 result = identity();
        result.m[12] = offset.x;
        result.m[13] = offset.y;
        result.m[14] = offset.z;
        return result;
    }

    Mat4 scaling(Vec3 factors)
    {
        Mat4 result{};
        result.m[0] = factors.x;
        result.m[5] = factors.y;
        result.m[10] = factors.z;
        result.m[15] = 1.0f;
        return result;
    }

    Mat4 rotationEulerDegrees(Vec3 angles)
    {
        constexpr float degreesToRadians = 3.14159265359f / 180.0f;
        const float x = angles.x * degreesToRadians;
        const float y = angles.y * degreesToRadians;
        const float z = angles.z * degreesToRadians;
        const float cx = std::cos(x);
        const float sx = std::sin(x);
        const float cy = std::cos(y);
        const float sy = std::sin(y);
        const float cz = std::cos(z);
        const float sz = std::sin(z);

        Mat4 rotationX = identity();
        rotationX.m[5] = cx;
        rotationX.m[6] = sx;
        rotationX.m[9] = -sx;
        rotationX.m[10] = cx;

        Mat4 rotationY = identity();
        rotationY.m[0] = cy;
        rotationY.m[2] = -sy;
        rotationY.m[8] = sy;
        rotationY.m[10] = cy;

        Mat4 rotationZ = identity();
        rotationZ.m[0] = cz;
        rotationZ.m[1] = sz;
        rotationZ.m[4] = -sz;
        rotationZ.m[5] = cz;

        return multiply(rotationZ, multiply(rotationY, rotationX));
    }

    Mat4 composeTransform(Vec3 position, Vec3 rotationDegrees, Vec3 scaleFactors)
    {
        return multiply(
            translation(position),
            multiply(rotationEulerDegrees(rotationDegrees), scaling(scaleFactors))
        );
    }

    Mat4 multiply(const Mat4& a, const Mat4& b)
    {
        Mat4 result{};
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                for (int k = 0; k < 4; ++k)
                    result.m[column * 4 + row] += a.m[k * 4 + row] * b.m[column * 4 + k];
            }
        }
        return result;
    }

    Mat4 perspective(float fovDegrees, float aspect, float nearPlane, float farPlane)
    {
        constexpr float pi = 3.14159265359f;
        const float f = 1.0f / std::tan(fovDegrees * pi / 360.0f);
        Mat4 result{};
        result.m[0] = f / aspect;
        result.m[5] = f;
        result.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
        result.m[11] = -1.0f;
        result.m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
        return result;
    }

    Mat4 orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        Mat4 result = identity();
        result.m[0] = 2.0f / (right - left);
        result.m[5] = 2.0f / (top - bottom);
        result.m[10] = -2.0f / (farPlane - nearPlane);
        result.m[12] = -(right + left) / (right - left);
        result.m[13] = -(top + bottom) / (top - bottom);
        result.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
        return result;
    }

    Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up)
    {
        const Vec3 forward = normalize({center.x - eye.x, center.y - eye.y, center.z - eye.z});
        const Vec3 side = normalize(cross(forward, up));
        const Vec3 correctedUp = cross(side, forward);

        Mat4 result = identity();
        result.m[0] = side.x;
        result.m[4] = side.y;
        result.m[8] = side.z;
        result.m[1] = correctedUp.x;
        result.m[5] = correctedUp.y;
        result.m[9] = correctedUp.z;
        result.m[2] = -forward.x;
        result.m[6] = -forward.y;
        result.m[10] = -forward.z;
        result.m[12] = -dot(side, eye);
        result.m[13] = -dot(correctedUp, eye);
        result.m[14] = dot(forward, eye);
        return result;
    }
}
