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

    struct PointLight
    {
        Math::Vec3 position{0.0f, 0.0f, 0.0f};
        Math::Vec3 color{1.0f, 1.0f, 1.0f};
        float intensity = 1.0f;
        float constantAttenuation = 1.0f;
        float linearAttenuation = 0.09f;
        float quadraticAttenuation = 0.032f;
    };

    struct MeshInstance
    {
        std::size_t meshIndex;
        std::size_t materialIndex;
        Transform transform;
    };

    struct CollisionBox
    {
        Math::Vec3 minimum;
        Math::Vec3 maximum;
    };

    class Scene
    {
    public:
        static constexpr std::size_t maximumPointLights = 4;

        std::size_t addMesh(Mesh mesh);
        std::size_t addMaterial(Material material);
        bool addPointLight(PointLight light);
        bool addObject(std::size_t meshIndex, std::size_t materialIndex, Transform transform = {});
        void addBoxCollider(Math::Vec3 center, Math::Vec3 size);

        const std::vector<Mesh>& meshes() const { return m_meshes; }
        const std::vector<Material>& materials() const { return m_materials; }
        const std::vector<MeshInstance>& objects() const { return m_objects; }
        const std::vector<CollisionBox>& colliders() const { return m_colliders; }
        const DirectionalLight& directionalLight() const { return m_directionalLight; }
        const std::vector<PointLight>& pointLights() const { return m_pointLights; }

    private:
        std::vector<Mesh> m_meshes;
        std::vector<Material> m_materials;
        std::vector<MeshInstance> m_objects;
        std::vector<CollisionBox> m_colliders;
        DirectionalLight m_directionalLight;
        std::vector<PointLight> m_pointLights;
    };

    Scene makeDemoScene();
    float sampleDemoTerrainHeight(float x, float z);
}
