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

    Scene makeDemoScene()
    {
        Scene scene;
        const std::size_t cube = scene.addMesh(Mesh::cube());
        const std::size_t sphere = scene.addMesh(Mesh::sphere());
        Mesh pyramid;
        std::string loadError;
        const bool loadedPyramid = Mesh::loadObj("assets/models/pyramid.obj", pyramid, loadError);
        const std::size_t importedMesh = scene.addMesh(loadedPyramid ? std::move(pyramid) : Mesh::cube());
        const std::size_t blue = scene.addMaterial({{0.15f, 0.65f, 1.0f}});
        const std::size_t white = scene.addMaterial({{1.0f, 1.0f, 1.0f}});
        const std::size_t fallbackOrange = scene.addMaterial({{1.0f, 0.38f, 0.12f}});
        const std::size_t green = scene.addMaterial({{0.18f, 0.85f, 0.48f}});
        scene.addObject(cube, blue, {{-1.25f, 0.0f, 0.0f}, {18.0f, 25.0f, -8.0f}, {0.9f, 0.9f, 0.9f}});
        scene.addObject(importedMesh, loadedPyramid ? white : fallbackOrange,
            {{0.0f, 0.0f, 0.0f}, {8.0f, -18.0f, 12.0f}, {1.1f, 1.1f, 1.1f}});
        scene.addObject(sphere, green, {{1.25f, 0.0f, 0.0f}, {-15.0f, -28.0f, 6.0f}, {0.85f, 0.85f, 0.85f}});
        return scene;
    }
}
