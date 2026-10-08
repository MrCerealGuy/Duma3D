#include "Engine/Scene/Scene.hpp"

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

    bool Scene::addObject(std::size_t meshIndex, std::size_t materialIndex, Transform transform)
    {
        if (meshIndex >= m_meshes.size() || materialIndex >= m_materials.size())
            return false;

        m_objects.push_back({meshIndex, materialIndex, transform});
        return true;
    }

    Scene makeDemoScene()
    {
        Scene scene;
        const std::size_t cube = scene.addMesh(Mesh::cube());
        const std::size_t blue = scene.addMaterial({{0.15f, 0.65f, 1.0f}});
        const std::size_t orange = scene.addMaterial({{1.0f, 0.38f, 0.12f}});
        const std::size_t green = scene.addMaterial({{0.18f, 0.85f, 0.48f}});
        scene.addObject(cube, blue, {{-1.25f, 0.0f, 0.0f}});
        scene.addObject(cube, orange, {{0.0f, 0.0f, 0.0f}});
        scene.addObject(cube, green, {{1.25f, 0.0f, 0.0f}});
        return scene;
    }
}
