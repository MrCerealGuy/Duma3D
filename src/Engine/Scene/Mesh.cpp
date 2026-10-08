#include "Engine/Scene/Mesh.hpp"

namespace Engine::Scene
{
    Mesh Mesh::cube()
    {
        Mesh mesh;
        const auto addFace = [&mesh](Math::Vec3 normal, const Math::Vec3 (&positions)[6])
        {
            for (const Math::Vec3 position : positions)
                mesh.vertices.push_back({position, normal});
        };

        const Math::Vec3 back[] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
            { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}
        };
        addFace({0.0f, 0.0f, -1.0f}, back);

        const Math::Vec3 front[] = {
            {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}
        };
        addFace({0.0f, 0.0f, 1.0f}, front);

        const Math::Vec3 left[] = {
            {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
            {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
        };
        addFace({-1.0f, 0.0f, 0.0f}, left);

        const Math::Vec3 right[] = {
            { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
            { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}
        };
        addFace({1.0f, 0.0f, 0.0f}, right);

        const Math::Vec3 bottom[] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
            { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f}
        };
        addFace({0.0f, -1.0f, 0.0f}, bottom);

        const Math::Vec3 top[] = {
            {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}
        };
        addFace({0.0f, 1.0f, 0.0f}, top);
        return mesh;
    }
}
