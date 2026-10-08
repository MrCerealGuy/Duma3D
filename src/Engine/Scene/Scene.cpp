#include "Engine/Scene/Scene.hpp"

#include <utility>

namespace Engine::Scene
{
    std::size_t Scene::addMesh(Mesh mesh)
    {
        m_meshes.push_back(std::move(mesh));
        return m_meshes.size() - 1;
    }

    bool Scene::addObject(std::size_t meshIndex, Transform transform)
    {
        if (meshIndex >= m_meshes.size())
            return false;

        m_objects.push_back({meshIndex, transform});
        return true;
    }

    Scene makeDemoScene()
    {
        Scene scene;
        const std::size_t cube = scene.addMesh(Mesh::cube());
        scene.addObject(cube, {{-1.25f, 0.0f, 0.0f}});
        scene.addObject(cube, {{0.0f, 0.0f, 0.0f}});
        scene.addObject(cube, {{1.25f, 0.0f, 0.0f}});
        return scene;
    }
}
