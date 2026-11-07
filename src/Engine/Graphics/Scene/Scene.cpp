#include "Engine/Graphics/Scene/Scene.hpp"

#include <cmath>
#include <utility>

namespace Engine::Graphics::Scene
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

    bool Scene::setPointLightIntensity(std::size_t index, float intensity)
    {
        if (index >= m_pointLights.size() || !std::isfinite(intensity) || intensity < 0.0f)
            return false;
        m_pointLights[index].intensity = intensity;
        return true;
    }

    bool Scene::setPointLight(std::size_t index, PointLight light)
    {
        if (index >= m_pointLights.size() || !std::isfinite(light.intensity) || light.intensity < 0.0f ||
            !std::isfinite(light.constantAttenuation) || light.constantAttenuation <= 0.0f ||
            !std::isfinite(light.linearAttenuation) || light.linearAttenuation < 0.0f ||
            !std::isfinite(light.quadraticAttenuation) || light.quadraticAttenuation < 0.0f)
            return false;
        m_pointLights[index] = light;
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

    bool Scene::setObjectTransform(std::size_t index, Transform transform)
    {
        constexpr float minimumScale = 1.0e-6f;
        const bool hasSingularScale =
            std::abs(transform.scale.x) < minimumScale ||
            std::abs(transform.scale.y) < minimumScale ||
            std::abs(transform.scale.z) < minimumScale;
        if (index >= m_objects.size() || hasSingularScale)
            return false;
        m_objects[index].transform = transform;
        return true;
    }

}
