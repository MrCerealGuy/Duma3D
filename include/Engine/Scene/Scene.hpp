#pragma once

#include "Engine/Scene/Mesh.hpp"

#include <cstddef>
#include <vector>

namespace Engine::Scene
{
    struct Transform
    {
        Math::Vec3 position{0.0f, 0.0f, 0.0f};
        Math::Vec3 rotationDegrees{0.0f, 0.0f, 0.0f};
        Math::Vec3 scale{1.0f, 1.0f, 1.0f};
    };

    struct Material
    {
        Math::Vec3 baseColor{1.0f, 1.0f, 1.0f};
    };

    struct DirectionalLight
    {
        Math::Vec3 direction{-0.4f, -1.0f, -0.6f};
        Math::Vec3 color{1.0f, 1.0f, 1.0f};
        Math::Vec3 ambientColor{0.22f, 0.22f, 0.22f};
    };

    struct MeshInstance
    {
        std::size_t meshIndex;
        std::size_t materialIndex;
        Transform transform;
    };

    class Scene
    {
    public:
        std::size_t addMesh(Mesh mesh);
        std::size_t addMaterial(Material material);
        bool addObject(std::size_t meshIndex, std::size_t materialIndex, Transform transform = {});

        const std::vector<Mesh>& meshes() const { return m_meshes; }
        const std::vector<Material>& materials() const { return m_materials; }
        const std::vector<MeshInstance>& objects() const { return m_objects; }
        const DirectionalLight& directionalLight() const { return m_directionalLight; }

    private:
        std::vector<Mesh> m_meshes;
        std::vector<Material> m_materials;
        std::vector<MeshInstance> m_objects;
        DirectionalLight m_directionalLight;
    };

    Scene makeDemoScene();
}
