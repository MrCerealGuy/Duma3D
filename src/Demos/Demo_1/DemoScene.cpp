#include "DemoScene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <random>
#include <utility>
#include <vector>

namespace Duma3D::Demos::Demo_1
{
    using Scene = Engine::Scene::Scene;
    using Mesh = Engine::Scene::Mesh;
    namespace Math = Engine::Math;

    namespace
    {
        constexpr float terrainHalfSize = 22.0f;
        constexpr int terrainSegments = 128;
        constexpr float houseCentersX[] = {-7.0f, 7.0f};
        constexpr float houseCenterZ = -4.0f;

        struct HouseGenerationOptions
        {
            float width;
            float depth;
            float wallHeight;
            float roofPeakHeight;
            float entranceOffset;
            float partitionOffset;
            float windowWidth;
            float interiorDoorWidth;
            bool flatRoof;
        };

        class TerrainField
        {
        public:
            TerrainField(std::mt19937& generator, float cellSize)
                : m_cellSize(cellSize), m_values(18 * 18)
            {
                std::uniform_real_distribution<float> value(-1.0f, 1.0f);
                for (float& sample : m_values)
                    sample = value(generator);
            }

            float sample(float x, float z) const
            {
                const float gridX = x / m_cellSize + 9.0f;
                const float gridZ = z / m_cellSize + 9.0f;
                const int cellX = static_cast<int>(std::floor(gridX));
                const int cellZ = static_cast<int>(std::floor(gridZ));
                const float blendX = smooth(gridX - cellX);
                const float blendZ = smooth(gridZ - cellZ);
                const float top = mix(valueAt(cellX, cellZ), valueAt(cellX + 1, cellZ), blendX);
                const float bottom = mix(valueAt(cellX, cellZ + 1), valueAt(cellX + 1, cellZ + 1), blendX);
                return mix(top, bottom, blendZ);
            }

        private:
            static float smooth(float value) { return value * value * (3.0f - 2.0f * value); }
            static float mix(float a, float b, float t) { return a + (b - a) * t; }

            float valueAt(int x, int z) const
            {
                constexpr int side = 18;
                x = ((x % side) + side) % side;
                z = ((z % side) + side) % side;
                return m_values[static_cast<std::size_t>(z * side + x)];
            }

            float m_cellSize;
            std::vector<float> m_values;
        };

        struct GroundPatch
        {
            Math::Vec2 center;
            float radius;
            int surface;
        };

        float terrainHeight(float x, float z)
        {
            float flatten = 1.0f;
            for (const float houseX : houseCentersX)
            {
                const float dx = x - houseX;
                const float dz = z - houseCenterZ;
                const float distance = std::sqrt(dx * dx + dz * dz);
                const float t = std::clamp((distance - 5.8f) / 3.0f, 0.0f, 1.0f);
                flatten = std::min(flatten, t * t * (3.0f - 2.0f * t));
            }

            const float hills = 0.8f * std::sin(x * 0.18f) * std::cos(z * 0.15f) +
                0.35f * std::sin((x + z) * 0.31f);
            return hills * flatten;
        }

        Mesh makeLandscape(std::mt19937& generator)
        {
            const TerrainField broadNoise(generator, 8.0f);
            const TerrainField mediumNoise(generator, 3.0f);
            const TerrainField detailNoise(generator, 1.1f);
            std::uniform_real_distribution<float> patchPosition(-19.0f, 19.0f);
            std::uniform_real_distribution<float> soilRadius(1.8f, 4.2f);
            std::uniform_real_distribution<float> rockRadius(1.2f, 3.2f);
            std::uniform_real_distribution<float> pavingRadius(1.4f, 2.8f);
            std::vector<GroundPatch> patches;
            for (int index = 0; index < 9; ++index)
                patches.push_back({{patchPosition(generator), patchPosition(generator)}, soilRadius(generator), 1});
            for (int index = 0; index < 6; ++index)
                patches.push_back({{patchPosition(generator), patchPosition(generator)}, rockRadius(generator), 2});
            for (int index = 0; index < 5; ++index)
                patches.push_back({{patchPosition(generator), patchPosition(generator)}, pavingRadius(generator), 3});

            Mesh mesh;
            constexpr int rowLength = terrainSegments + 1;
            mesh.vertices.reserve(rowLength * rowLength);
            std::array<std::vector<std::uint32_t>, 4> surfaceIndices;
            for (auto& indices : surfaceIndices)
                indices.reserve(terrainSegments * terrainSegments * 2);
            for (int zIndex = 0; zIndex <= terrainSegments; ++zIndex)
            {
                const float v = static_cast<float>(zIndex) / terrainSegments;
                const float z = -terrainHalfSize + 2.0f * terrainHalfSize * v;
                for (int xIndex = 0; xIndex <= terrainSegments; ++xIndex)
                {
                    const float u = static_cast<float>(xIndex) / terrainSegments;
                    const float x = -terrainHalfSize + 2.0f * terrainHalfSize * u;
                    const float y = terrainHeight(x, z);
                    constexpr float sampleOffset = 0.05f;
                    const float slopeX = (terrainHeight(x + sampleOffset, z) -
                        terrainHeight(x - sampleOffset, z)) / (2.0f * sampleOffset);
                    const float slopeZ = (terrainHeight(x, z + sampleOffset) -
                        terrainHeight(x, z - sampleOffset)) / (2.0f * sampleOffset);
                    const Math::Vec3 normal = Math::normalize({-slopeX, 1.0f, -slopeZ});
                    const float variation = std::clamp(
                        0.88f + broadNoise.sample(x, z) * 0.07f + detailNoise.sample(x, z) * 0.04f + y * 0.05f,
                        0.72f, 1.0f);
                    mesh.vertices.push_back({
                        {x, y, z}, normal,
                        {variation, variation, variation},
                        {u * 44.0f, v * 44.0f}
                    });
                }
            }

            for (int zIndex = 0; zIndex < terrainSegments; ++zIndex)
            {
                for (int xIndex = 0; xIndex < terrainSegments; ++xIndex)
                {
                    const std::uint32_t topLeft = static_cast<std::uint32_t>(zIndex * rowLength + xIndex);
                    const std::uint32_t topRight = topLeft + 1;
                    const std::uint32_t bottomLeft = topLeft + rowLength;
                    const std::uint32_t bottomRight = bottomLeft + 1;
                    const float x = -terrainHalfSize + (xIndex + 0.5f) *
                        (2.0f * terrainHalfSize / terrainSegments);
                    const float z = -terrainHalfSize + (zIndex + 0.5f) *
                        (2.0f * terrainHalfSize / terrainSegments);
                    const float broad = broadNoise.sample(x, z);
                    const float medium = mediumNoise.sample(x, z);
                    const float detail = detailNoise.sample(x, z);
                    const float slopeX = (terrainHeight(x + 0.2f, z) - terrainHeight(x - 0.2f, z)) / 0.4f;
                    const float slopeZ = (terrainHeight(x, z + 0.2f) - terrainHeight(x, z - 0.2f)) / 0.4f;
                    int surface = broad + medium * 0.35f + detail * 0.18f > 0.30f ? 1 : 0;
                    if (broad < -0.55f && std::sqrt(slopeX * slopeX + slopeZ * slopeZ) > 0.12f)
                        surface = 2;

                    float closestPatchDistance = std::numeric_limits<float>::max();
                    for (const GroundPatch& patch : patches)
                    {
                        const float dx = x - patch.center.x;
                        const float dz = z - patch.center.y;
                        const float edgeWarp = (medium * 0.42f + detail * 0.16f) * patch.radius * 0.28f;
                        const float distance = std::sqrt(dx * dx + dz * dz) + edgeWarp;
                        if (distance < patch.radius && distance < closestPatchDistance)
                        {
                            closestPatchDistance = distance;
                            surface = patch.surface;
                        }
                    }

                    std::vector<std::uint32_t>& indices = surfaceIndices[static_cast<std::size_t>(surface)];
                    indices.insert(indices.end(), {
                        topLeft, bottomLeft, topRight,
                        topRight, bottomLeft, bottomRight
                    });
                }
            }
            const std::array<const char*, 4> surfaceTextures = {
                "assets/textures/grass.ppm",
                "assets/textures/soil.ppm",
                "assets/textures/rock.ppm",
                "assets/textures/cobblestone.ppm"
            };
            for (std::size_t surface = 0; surface < surfaceIndices.size(); ++surface)
            {
                if (surfaceIndices[surface].empty())
                    continue;
                const std::size_t firstIndex = mesh.indices.size();
                mesh.indices.insert(mesh.indices.end(), surfaceIndices[surface].begin(), surfaceIndices[surface].end());
                mesh.sections.push_back({firstIndex, surfaceIndices[surface].size(), surfaceTextures[surface]});
            }
            return mesh;
        }

        Mesh makeGables(float width, float peakHeight, float depth)
        {
            const float halfWidth = width * 0.5f;
            const float halfDepth = depth * 0.5f;
            Mesh mesh;
            mesh.vertices = {
                {{-halfWidth, 0.0f, halfDepth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
                {{ halfWidth, 0.0f, halfDepth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
                {{0.0f, peakHeight, halfDepth}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 1.0f}},
                {{-halfWidth, 0.0f,-halfDepth}, {0.0f, 0.0f,-1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
                {{ halfWidth, 0.0f,-halfDepth}, {0.0f, 0.0f,-1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
                {{0.0f, peakHeight,-halfDepth}, {0.0f, 0.0f,-1.0f}, {1.0f, 1.0f, 1.0f}, {0.5f, 1.0f}}
            };
            mesh.indices = {0, 1, 2, 3, 5, 4};
            mesh.sections.push_back({0, mesh.indices.size(), {}});
            return mesh;
        }

        void addBox(
            Scene& scene,
            std::size_t cubeMesh,
            std::size_t material,
            Math::Vec3 position,
            Math::Vec3 scale,
            Math::Vec3 rotation = {0.0f, 0.0f, 0.0f}
        )
        {
            scene.addObject(cubeMesh, material, {position, rotation, scale});
        }

        void addWall(
            Scene& scene,
            std::size_t cubeMesh,
            std::size_t material,
            Math::Vec3 position,
            Math::Vec3 scale
        )
        {
            addBox(scene, cubeMesh, material, position, scale);
            scene.addBoxCollider(position, scale);
        }

        void addHouse(
            Scene& scene,
            std::size_t cubeMesh,
            std::size_t gableMesh,
            std::size_t wallMaterial,
            std::size_t floorMaterial,
            std::size_t roofMaterial,
            std::size_t trimMaterial,
            Math::Vec3 center,
            Math::Vec3 frontRoomLight,
            Math::Vec3 rearRoomLight,
            const HouseGenerationOptions& options
        )
        {
            const float width = options.width;
            const float depth = options.depth;
            constexpr float wallThickness = 0.18f;
            constexpr float floorTop = 0.16f;
            const float wallHeight = options.wallHeight;
            const float doorWidth = 1.05f;
            const float doorHeight = std::min(2.2f, wallHeight - 0.3f);
            const float roofPeakHeight = options.roofPeakHeight;
            const float wallCenterY = floorTop + wallHeight * 0.5f;
            const float wallTop = floorTop + wallHeight;
            const float frontZ = center.z + depth * 0.5f - wallThickness * 0.5f;
            const float backZ = center.z - depth * 0.5f + wallThickness * 0.5f;

            addBox(scene, cubeMesh, floorMaterial,
                {center.x, floorTop * 0.5f, center.z}, {width, floorTop, depth});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x, wallCenterY, backZ}, {width, wallHeight, wallThickness});

            const float entranceStart = options.entranceOffset - doorWidth * 0.5f;
            const float entranceEnd = options.entranceOffset + doorWidth * 0.5f;
            const float leftFrontWidth = entranceStart + width * 0.5f;
            const float rightFrontWidth = width * 0.5f - entranceEnd;
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + (-width * 0.5f + entranceStart) * 0.5f, wallCenterY, frontZ},
                {leftFrontWidth, wallHeight, wallThickness});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + (entranceEnd + width * 0.5f) * 0.5f, wallCenterY, frontZ},
                {rightFrontWidth, wallHeight, wallThickness});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + options.entranceOffset, floorTop + (wallHeight + doorHeight) * 0.5f, frontZ},
                {doorWidth, wallHeight - doorHeight, wallThickness});

            const float interiorDoorWidth = options.interiorDoorWidth;
            const float partitionZ = center.z + options.partitionOffset;
            const float partitionDoorStart = options.entranceOffset - interiorDoorWidth * 0.5f;
            const float partitionDoorEnd = options.entranceOffset + interiorDoorWidth * 0.5f;
            const float partitionLeftWidth = partitionDoorStart + width * 0.5f;
            const float partitionRightWidth = width * 0.5f - partitionDoorEnd;
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + (-width * 0.5f + partitionDoorStart) * 0.5f,
                    wallCenterY, partitionZ},
                {partitionLeftWidth, wallHeight, wallThickness});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + (partitionDoorEnd + width * 0.5f) * 0.5f,
                    wallCenterY, partitionZ},
                {partitionRightWidth, wallHeight, wallThickness});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + options.entranceOffset, floorTop + (wallHeight + doorHeight) * 0.5f, partitionZ},
                {interiorDoorWidth, wallHeight - doorHeight, wallThickness});

            constexpr float windowBottom = 1.0f;
            const float windowTop = std::min(2.05f, wallHeight - 0.35f);
            const float windowWidth = options.windowWidth;
            const float frontRoomCenterZ = (center.z + depth * 0.5f + partitionZ) * 0.5f;
            const float windowCenterZ = frontRoomCenterZ;
            const float rearWindowSegmentLength = windowCenterZ - windowWidth * 0.5f - (center.z - depth * 0.5f);
            const float frontWindowSegmentLength = (center.z + depth * 0.5f) -
                (windowCenterZ + windowWidth * 0.5f);
            for (const float side : {-1.0f, 1.0f})
            {
                const float sideX = center.x + side * (width * 0.5f - wallThickness * 0.5f);
                const float windowCenterY = floorTop + (windowBottom + windowTop) * 0.5f;
                addWall(scene, cubeMesh, wallMaterial,
                    {sideX, floorTop + windowBottom * 0.5f, center.z},
                    {wallThickness, windowBottom, depth});
                addWall(scene, cubeMesh, wallMaterial,
                    {sideX, floorTop + (wallHeight + windowTop) * 0.5f, center.z},
                    {wallThickness, wallHeight - windowTop, depth});
                addWall(scene, cubeMesh, wallMaterial,
                    {sideX, windowCenterY,
                        (center.z - depth * 0.5f + windowCenterZ - windowWidth * 0.5f) * 0.5f},
                    {wallThickness, windowTop - windowBottom, rearWindowSegmentLength});
                addWall(scene, cubeMesh, wallMaterial,
                    {sideX, windowCenterY,
                        (windowCenterZ + windowWidth * 0.5f + center.z + depth * 0.5f) * 0.5f},
                    {wallThickness, windowTop - windowBottom, frontWindowSegmentLength});
                addBox(scene, cubeMesh, trimMaterial,
                    {center.x + side * (width * 0.5f + 0.03f), windowBottom + floorTop, windowCenterZ},
                    {0.12f, 0.08f, windowWidth + 0.16f});
                addBox(scene, cubeMesh, trimMaterial,
                    {center.x + side * (width * 0.5f + 0.03f), windowCenterY, windowCenterZ - windowWidth * 0.5f},
                    {0.1f, windowTop - windowBottom, 0.08f});
                addBox(scene, cubeMesh, trimMaterial,
                    {center.x + side * (width * 0.5f + 0.03f), windowCenterY, windowCenterZ + windowWidth * 0.5f},
                    {0.1f, windowTop - windowBottom, 0.08f});
            }

            const float roofCenterY = wallTop + roofPeakHeight * 0.5f;
            if (options.flatRoof)
            {
                constexpr float parapetHeight = 0.38f;
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x, wallTop + 0.1f, center.z}, {width + 0.4f, 0.2f, depth + 0.4f});
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x, wallTop + 0.2f + parapetHeight * 0.5f, center.z - depth * 0.5f},
                    {width + 0.4f, parapetHeight, wallThickness});
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x, wallTop + 0.2f + parapetHeight * 0.5f, center.z + depth * 0.5f},
                    {width + 0.4f, parapetHeight, wallThickness});
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x - width * 0.5f, wallTop + 0.2f + parapetHeight * 0.5f, center.z},
                    {wallThickness, parapetHeight, depth});
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x + width * 0.5f, wallTop + 0.2f + parapetHeight * 0.5f, center.z},
                    {wallThickness, parapetHeight, depth});
                scene.addBoxCollider(
                    {center.x, wallTop + 0.3f, center.z},
                    {width + 0.4f, 0.8f, depth + 0.4f}
                );
            }
            else
            {
                const float roofAngle = std::atan2(roofPeakHeight, width * 0.5f) *
                    180.0f / 3.14159265359f;
                const float roofPanelLength = std::sqrt(
                    width * width * 0.25f + roofPeakHeight * roofPeakHeight
                ) + 0.2f;
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x - width * 0.25f, roofCenterY, center.z},
                    {roofPanelLength, 0.18f, depth + 0.4f}, {0.0f, 0.0f, roofAngle});
                addBox(scene, cubeMesh, roofMaterial,
                    {center.x + width * 0.25f, roofCenterY, center.z},
                    {roofPanelLength, 0.18f, depth + 0.4f}, {0.0f, 0.0f, -roofAngle});
                scene.addBoxCollider(
                    {center.x, wallTop + roofPeakHeight * 0.5f, center.z},
                    {width + 0.4f, roofPeakHeight + 0.2f, depth + 0.4f}
                );
                scene.addObject(gableMesh, roofMaterial,
                    {{center.x, wallTop, center.z}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}});
            }

            const float rearRoomCenterZ = (partitionZ + center.z - depth * 0.5f) * 0.5f;
            const float roomLightY = floorTop + wallHeight * 0.72f;
            scene.addPointLight({
                {center.x + options.entranceOffset, roomLightY, frontRoomCenterZ}, frontRoomLight,
                3.2f, 1.0f, 0.12f, 0.035f
            });
            scene.addPointLight({
                {center.x + options.entranceOffset, roomLightY, rearRoomCenterZ}, rearRoomLight,
                3.2f, 1.0f, 0.12f, 0.035f
            });
        }
    }

    float sampleDemoTerrainHeight(float x, float z)
    {
        constexpr float houseFloorTop = 0.16f;
        for (const float houseX : houseCentersX)
        {
            if (std::abs(x - houseX) < 3.0f && std::abs(z - houseCenterZ) < 3.0f)
                return houseFloorTop;
        }
        return terrainHeight(x, z);
    }

    Scene makeDemoScene()
    {
        Scene scene;
        std::mt19937 generator(std::random_device{}());
        const std::size_t ground = scene.addMesh(makeLandscape(generator));
        const std::size_t cube = scene.addMesh(Mesh::cube());
        const std::size_t sphere = scene.addMesh(Mesh::sphere());
        const std::size_t groundMaterial = scene.addMaterial({{0.92f, 0.92f, 0.92f}, {}});
        const std::array<std::size_t, 3> wallMaterials = {
            scene.addMaterial({{0.95f, 0.92f, 0.88f}, "assets/textures/plaster_ochre.ppm"}),
            scene.addMaterial({{0.87f, 0.91f, 0.95f}, "assets/textures/plaster_blue.ppm"}),
            scene.addMaterial({{0.87f, 0.93f, 0.85f}, "assets/textures/plaster_olive.ppm"})
        };
        const std::array<std::size_t, 3> roofMaterials = {
            scene.addMaterial({{0.95f, 0.91f, 0.88f}, "assets/textures/roof_terracotta.ppm"}),
            scene.addMaterial({{0.9f, 0.93f, 0.96f}, "assets/textures/roof_slate.ppm"}),
            scene.addMaterial({{0.94f, 0.91f, 0.82f}, "assets/textures/roof_wood.ppm"})
        };
        const std::size_t floor = scene.addMaterial({{0.94f, 0.91f, 0.85f}, "assets/textures/wood_floor.ppm"});
        const std::size_t trim = scene.addMaterial({{0.86f, 0.84f, 0.8f}, "assets/textures/dark_wood.ppm"});
        const std::size_t path = scene.addMaterial({{0.93f, 0.92f, 0.88f}, "assets/textures/cobblestone.ppm"});
        const std::size_t bark = scene.addMaterial({{0.92f, 0.88f, 0.82f}, "assets/textures/bark.ppm"});
        const std::array<std::size_t, 3> foliageMaterials = {
            scene.addMaterial({{0.86f, 0.96f, 0.82f}, "assets/textures/foliage.ppm"}),
            scene.addMaterial({{0.97f, 0.88f, 0.72f}, "assets/textures/foliage.ppm"}),
            scene.addMaterial({{0.78f, 0.91f, 0.86f}, "assets/textures/foliage.ppm"})
        };
        const std::size_t green = scene.addMaterial({{0.18f, 0.72f, 0.3f}, {}});
        const std::array<Math::Vec3, 6> roomLightPalette = {{
            {1.0f, 0.48f, 0.2f}, {0.2f, 0.42f, 1.0f},
            {0.25f, 0.9f, 0.3f}, {1.0f, 0.2f, 0.5f},
            {1.0f, 0.72f, 0.3f}, {0.35f, 0.9f, 0.85f}
        }};

        scene.addObject(ground, groundMaterial);
        std::array<Math::Vec3, 6> shuffledRoomLights = roomLightPalette;
        std::shuffle(shuffledRoomLights.begin(), shuffledRoomLights.end(), generator);
        std::uniform_int_distribution<std::size_t> styleChoice(0, wallMaterials.size() - 1);
        std::uniform_real_distribution<float> houseWidth(5.5f, 6.4f);
        std::uniform_real_distribution<float> houseDepth(5.5f, 6.4f);
        std::uniform_real_distribution<float> houseWallHeight(2.5f, 3.05f);
        std::uniform_real_distribution<float> houseRoofHeight(0.8f, 1.35f);
        std::uniform_real_distribution<float> doorOffset(-0.65f, 0.65f);
        std::uniform_real_distribution<float> partitionOffset(-0.45f, 0.45f);
        std::uniform_real_distribution<float> windowWidth(1.0f, 1.5f);
        std::uniform_real_distribution<float> interiorDoorWidth(1.0f, 1.35f);
        std::uniform_real_distribution<float> housePositionX(-0.35f, 0.35f);
        std::uniform_real_distribution<float> housePositionZ(-0.45f, 0.45f);
        std::uniform_int_distribution<int> roofStyle(0, 3);
        std::array<Math::Vec3, 2> houseCenters{};
        std::array<HouseGenerationOptions, 2> generatedHouseOptions{};
        for (std::size_t houseIndex = 0; houseIndex < std::size(houseCentersX); ++houseIndex)
        {
            const HouseGenerationOptions options{
                houseWidth(generator),
                houseDepth(generator),
                houseWallHeight(generator),
                houseRoofHeight(generator),
                doorOffset(generator),
                partitionOffset(generator),
                windowWidth(generator),
                interiorDoorWidth(generator),
                roofStyle(generator) == 0
            };
            const float houseX = houseCentersX[houseIndex] + housePositionX(generator);
            const float houseZ = houseCenterZ + housePositionZ(generator);
            houseCenters[houseIndex] = {houseX, 0.0f, houseZ};
            generatedHouseOptions[houseIndex] = options;
            const std::size_t gable = options.flatRoof
                ? 0
                : scene.addMesh(makeGables(options.width, options.roofPeakHeight, options.depth));
            addHouse(
                scene, cube, gable,
                wallMaterials[styleChoice(generator)], floor,
                roofMaterials[styleChoice(generator)], trim,
                houseCenters[houseIndex],
                shuffledRoomLights[houseIndex * 2], shuffledRoomLights[houseIndex * 2 + 1],
                options
            );
        }

        for (std::size_t houseIndex = 0; houseIndex < houseCenters.size(); ++houseIndex)
        {
            const Math::Vec3 houseCenter = houseCenters[houseIndex];
            const HouseGenerationOptions& options = generatedHouseOptions[houseIndex];
            const float entryX = houseCenter.x + options.entranceOffset;
            const float frontZ = houseCenter.z + options.depth * 0.5f;
            for (int step = 0; step < 7; ++step)
            {
                const float z = frontZ + 0.55f + static_cast<float>(step) * 0.85f;
                addBox(scene, cube, path,
                    {entryX, sampleDemoTerrainHeight(entryX, z) + 0.06f, z},
                    {0.9f, 0.12f, 0.68f});
            }
        }

        std::uniform_real_distribution<float> landscapePosition(-19.0f, 19.0f);
        std::uniform_real_distribution<float> treeScale(0.8f, 1.35f);
        std::uniform_int_distribution<int> treeCountChoice(10, 17);
        std::uniform_int_distribution<std::size_t> foliageChoice(0, foliageMaterials.size() - 1);
        std::vector<Math::Vec2> treeLocations;
        const int treeCount = treeCountChoice(generator);
        for (int attempt = 0; attempt < treeCount * 30 &&
             treeLocations.size() < static_cast<std::size_t>(treeCount); ++attempt)
        {
            const float x = landscapePosition(generator);
            const float z = landscapePosition(generator);
            bool tooClose = std::sqrt(x * x + (z - 9.0f) * (z - 9.0f)) < 4.5f;
            for (const float houseX : houseCentersX)
            {
                const float dx = x - houseX;
                const float dz = z - houseCenterZ;
                tooClose = tooClose || std::sqrt(dx * dx + dz * dz) < 5.5f;
            }
            for (const Math::Vec2 existing : treeLocations)
            {
                const float dx = x - existing.x;
                const float dz = z - existing.y;
                tooClose = tooClose || dx * dx + dz * dz < 12.0f;
            }
            if (!tooClose)
                treeLocations.push_back({x, z});
        }

        for (const Math::Vec2 location : treeLocations)
        {
            const float scale = treeScale(generator);
            const float groundHeight = terrainHeight(location.x, location.y);
            addBox(scene, cube, bark,
                {location.x, groundHeight + 0.8f * scale, location.y},
                {0.38f * scale, 1.6f * scale, 0.38f * scale});
            scene.addObject(sphere, foliageMaterials[foliageChoice(generator)],
                {{location.x, groundHeight + 2.0f * scale, location.y},
                    {0.0f, 0.0f, 0.0f}, {1.5f * scale, 1.8f * scale, 1.5f * scale}});
        }

        const float monumentHeight = terrainHeight(0.0f, -15.0f);
        addBox(scene, cube, path, {0.0f, monumentHeight + 0.25f, -15.0f}, {1.8f, 0.5f, 1.8f});
        addBox(scene, cube, trim, {0.0f, monumentHeight + 1.2f, -15.0f}, {0.55f, 1.9f, 0.55f});
        scene.addObject(sphere, path,
            {{0.0f, monumentHeight + 2.35f, -15.0f}, {0.0f, 0.0f, 0.0f}, {0.75f, 0.75f, 0.75f}});
        scene.addObject(sphere, green,
            {{0.0f, terrainHeight(0.0f, 3.0f) + 0.9f, 3.0f}, {-15.0f, -28.0f, 6.0f}, {0.9f, 0.9f, 0.9f}});
        return scene;
    }

}
