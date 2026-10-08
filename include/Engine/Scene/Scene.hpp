#pragma once

#include "Engine/Scene/Mesh.hpp"

#include <cstddef>
#include <vector>

namespace Engine::Scene
{
    struct Transform
    {
        Math::Vec3 position{0.0f, 0.0f, 0.0f};
    };

    struct MeshInstance
    {
        std::size_t meshIndex;
        Transform transform;
    };

    class Scene
    {
    public:
        std::size_t addMesh(Mesh mesh);
        bool addObject(std::size_t meshIndex, Transform transform = {});

        const std::vector<Mesh>& meshes() const { return m_meshes; }
        const std::vector<MeshInstance>& objects() const { return m_objects; }

    private:
        std::vector<Mesh> m_meshes;
        std::vector<MeshInstance> m_objects;
    };

    Scene makeDemoScene();
}
