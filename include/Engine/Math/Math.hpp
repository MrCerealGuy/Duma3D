#pragma once

namespace Engine::Math
{
    struct Vec3
    {
        float x;
        float y;
        float z;
    };

    struct Mat4
    {
        float m[16];
    };

    Vec3 normalize(Vec3 value);
    Vec3 cross(Vec3 a, Vec3 b);
    float dot(Vec3 a, Vec3 b);

    Mat4 identity();
    Mat4 translation(Vec3 offset);
    Mat4 multiply(const Mat4& a, const Mat4& b);
    Mat4 perspective(float fovDegrees, float aspect, float nearPlane, float farPlane);
    Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up);
}
