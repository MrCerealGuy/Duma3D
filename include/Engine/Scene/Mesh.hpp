#pragma once

#include "Engine/Math/Math.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Engine::Scene
{
    struct MeshSection
    {
        std::size_t firstIndex = 0;
        std::size_t indexCount = 0;
        std::string diffuseTexturePath;
    };

    struct Vertex
    {
        Math::Vec3 position;
        Math::Vec3 normal;
        Math::Vec3 diffuseColor{1.0f, 1.0f, 1.0f};
        Math::Vec2 textureCoordinate{0.0f, 0.0f};
        Math::Vec3 specularColor{0.04f, 0.04f, 0.04f};
        float shininess = 32.0f;
        Math::Vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    };

    struct Mesh
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<MeshSection> sections;

        static Mesh cube();
        static Mesh plane(float size = 20.0f);
        static Mesh sphere();
        static bool loadObj(const std::string& path, Mesh& mesh, std::string& error);
    };
}
