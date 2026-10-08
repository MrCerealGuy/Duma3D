#include "Engine/Scene/Scene.hpp"

#include <cmath>
#include <utility>

namespace Engine::Scene
{
    std::size_t Scene::addMesh(Mesh mesh)
    {
        m_meshes.push_back(std::move(mesh));
        return m_meshes.size() - 1;
    }

    std::size_t Scene::addMaterial(Material material)
    {
        m_materials.push_back(material);
        return m_materials.size() - 1;
    }

    bool Scene::addPointLight(PointLight light)
    {
        if (m_pointLights.size() >= maximumPointLights || light.intensity < 0.0f ||
            light.constantAttenuation <= 0.0f || light.linearAttenuation < 0.0f ||
            light.quadraticAttenuation < 0.0f)
            return false;

        m_pointLights.push_back(light);
        return true;
    }

    bool Scene::addObject(std::size_t meshIndex, std::size_t materialIndex, Transform transform)
    {
        constexpr float minimumScale = 1.0e-6f;
        const bool hasSingularScale =
            std::abs(transform.scale.x) < minimumScale ||
            std::abs(transform.scale.y) < minimumScale ||
            std::abs(transform.scale.z) < minimumScale;
        if (meshIndex >= m_meshes.size() || materialIndex >= m_materials.size() || hasSingularScale)
            return false;

        m_objects.push_back({meshIndex, materialIndex, transform});
        return true;
    }

    void Scene::addBoxCollider(Math::Vec3 center, Math::Vec3 size)
    {
        const Math::Vec3 halfSize{
            std::abs(size.x) * 0.5f,
            std::abs(size.y) * 0.5f,
            std::abs(size.z) * 0.5f
        };
        m_colliders.push_back({
            {center.x - halfSize.x, center.y - halfSize.y, center.z - halfSize.z},
            {center.x + halfSize.x, center.y + halfSize.y, center.z + halfSize.z}
        });
    }


}
