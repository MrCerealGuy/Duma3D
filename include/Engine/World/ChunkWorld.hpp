#pragma once

#include "Engine/Scene/Scene.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <utility>

namespace Engine::World
{
    inline constexpr float defaultChunkSize = 64.0f;

    struct ChunkCoordinate
    {
        int x = 0;
        int z = 0;

        bool operator==(const ChunkCoordinate&) const = default;
    };

    struct ChunkWorldConfig
    {
        float chunkSize = defaultChunkSize;
        int loadRadius = 2;
        std::uint32_t seed = 0;
    };

    using ChunkGenerator = std::function<Scene::Scene(ChunkCoordinate, std::uint32_t)>;

    // Keeps a square area of generated scene chunks around the supplied world position.
    // Chunk geometry and content are provided by the client through ChunkGenerator.
    class ChunkWorld
    {
    public:
        ChunkWorld(ChunkWorldConfig config, ChunkGenerator generator);

        // Returns true when the center chunk changes and the loaded world scene is rebuilt.
        bool updateForPosition(float worldX, float worldZ);

        const Scene::Scene& scene() const { return m_scene; }
        ChunkCoordinate center() const { return m_center; }
        const ChunkWorldConfig& config() const { return m_config; }

        static int coordinateForPosition(float position, float chunkSize);

    private:
        using ChunkKey = std::pair<int, int>;

        void rebuildScene();

        ChunkWorldConfig m_config;
        ChunkGenerator m_generator;
        std::map<ChunkKey, Scene::Scene> m_chunks;
        Scene::Scene m_scene;
        ChunkCoordinate m_center{};
        bool m_hasCenter = false;
    };
}
