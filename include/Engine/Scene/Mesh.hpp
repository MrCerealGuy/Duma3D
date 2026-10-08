#pragma once

#include "Engine/Math/Math.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace Engine::Scene
{
    struct MeshSection
    {
        std::size_t firstVertex = 0;
        std::size_t vertexCount = 0;
        std::string diffuseTexturePath;
    };

    struct Vertex
    {
        Math::Vec3 position;
        Math::Vec3 normal;
        Math::Vec3 diffuseColor{1.0f, 1.0f, 1.0f};
        Math::Vec2 textureCoordinate{0.0f, 0.0f};
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<MeshSection> sections;

        static Mesh cube();
        static bool loadObj(const std::string& path, Mesh& mesh, std::string& error);
    };
}
