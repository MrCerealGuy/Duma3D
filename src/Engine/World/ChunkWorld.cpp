#include "Engine/World/ChunkWorld.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Engine::World
{
    ChunkWorld::ChunkWorld(ChunkWorldConfig config, ChunkGenerator generator)
        : m_config(config), m_generator(std::move(generator))
    {
        if (!std::isfinite(m_config.chunkSize) || m_config.chunkSize <= 0.0f)
            throw std::invalid_argument("ChunkWorld chunkSize must be finite and positive");
        if (m_config.loadRadius < 0)
            throw std::invalid_argument("ChunkWorld loadRadius cannot be negative");
        if (!m_generator)
            throw std::invalid_argument("ChunkWorld requires a chunk generator");
    }

    int ChunkWorld::coordinateForPosition(float position, float chunkSize)
    {
        if (!std::isfinite(position) || !std::isfinite(chunkSize) || chunkSize <= 0.0f)
            throw std::invalid_argument("Chunk coordinate requires finite position and positive size");
        return static_cast<int>(std::floor((position + chunkSize * 0.5f) / chunkSize));
    }

    bool ChunkWorld::updateForPosition(float worldX, float worldZ)
    {
        const ChunkCoordinate nextCenter{
            coordinateForPosition(worldX, m_config.chunkSize),
            coordinateForPosition(worldZ, m_config.chunkSize)
        };
        if (m_hasCenter && nextCenter == m_center)
            return false;

        m_center = nextCenter;
        m_hasCenter = true;
        for (int z = m_center.z - m_config.loadRadius; z <= m_center.z + m_config.loadRadius; ++z)
        {
            for (int x = m_center.x - m_config.loadRadius; x <= m_center.x + m_config.loadRadius; ++x)
            {
                const ChunkKey key{x, z};
                if (m_chunks.find(key) == m_chunks.end())
                    m_chunks.emplace(key, m_generator({x, z}, m_config.seed));
            }
        }

        for (auto chunk = m_chunks.begin(); chunk != m_chunks.end();)
        {
            const bool outsideX = std::abs(chunk->first.first - m_center.x) > m_config.loadRadius;
            const bool outsideZ = std::abs(chunk->first.second - m_center.z) > m_config.loadRadius;
            if (outsideX || outsideZ)
                chunk = m_chunks.erase(chunk);
            else
                ++chunk;
        }

        rebuildScene();
        return true;
    }

    void ChunkWorld::rebuildScene()
    {
        std::vector<const std::pair<const ChunkKey, Scene::Scene>*> orderedChunks;
        orderedChunks.reserve(m_chunks.size());
        for (const auto& chunk : m_chunks)
            orderedChunks.push_back(&chunk);
        std::sort(orderedChunks.begin(), orderedChunks.end(), [this](const auto* a, const auto* b)
        {
            const int distanceA = (a->first.first - m_center.x) * (a->first.first - m_center.x) +
                (a->first.second - m_center.z) * (a->first.second - m_center.z);
            const int distanceB = (b->first.first - m_center.x) * (b->first.first - m_center.x) +
                (b->first.second - m_center.z) * (b->first.second - m_center.z);
            return distanceA < distanceB;
        });

        Scene::Scene combined;
        for (const auto* chunkEntry : orderedChunks)
        {
            const auto& [x, z] = chunkEntry->first;
            const Scene::Scene& chunk = chunkEntry->second;
            const float offsetX = static_cast<float>(x) * m_config.chunkSize;
            const float offsetZ = static_cast<float>(z) * m_config.chunkSize;
            std::vector<std::size_t> meshIndices;
            std::vector<std::size_t> materialIndices;
            meshIndices.reserve(chunk.meshes().size());
            materialIndices.reserve(chunk.materials().size());
            for (const Scene::Material& material : chunk.materials())
                materialIndices.push_back(combined.addMaterial(material));
            for (const Scene::Mesh& mesh : chunk.meshes())
                meshIndices.push_back(combined.addMesh(mesh));
            for (const Scene::MeshInstance& object : chunk.objects())
            {
                if (object.meshIndex >= meshIndices.size() || object.materialIndex >= materialIndices.size())
                    continue;
                Scene::Transform transform = object.transform;
                transform.position.x += offsetX;
                transform.position.z += offsetZ;
                combined.addObject(meshIndices[object.meshIndex], materialIndices[object.materialIndex], transform);
            }
            for (const Scene::CollisionBox& collider : chunk.colliders())
            {
                const Math::Vec3 center{
                    (collider.minimum.x + collider.maximum.x) * 0.5f + offsetX,
                    (collider.minimum.y + collider.maximum.y) * 0.5f,
                    (collider.minimum.z + collider.maximum.z) * 0.5f + offsetZ
                };
                const Math::Vec3 size{
                    collider.maximum.x - collider.minimum.x,
                    collider.maximum.y - collider.minimum.y,
                    collider.maximum.z - collider.minimum.z
                };
                combined.addBoxCollider(center, size);
            }
            for (Scene::PointLight light : chunk.pointLights())
            {
                light.position.x += offsetX;
                light.position.z += offsetZ;
                if (!combined.addPointLight(light))
                    break;
            }
        }
        m_scene = std::move(combined);
    }
}
