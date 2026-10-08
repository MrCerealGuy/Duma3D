#include "Engine/Scene/Scene.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace Engine::Scene
{
    namespace
    {
        constexpr float terrainHalfSize = 22.0f;
        constexpr int terrainSegments = 48;
        constexpr float houseCentersX[] = {-7.0f, 7.0f};
        constexpr float houseCenterZ = -4.0f;

        float terrainHeight(float x, float z)
        {
            float flatten = 1.0f;
            for (const float houseX : houseCentersX)
            {
                const float dx = x - houseX;
                const float dz = z - houseCenterZ;
                const float distance = std::sqrt(dx * dx + dz * dz);
                const float t = std::clamp((distance - 5.0f) / 3.0f, 0.0f, 1.0f);
                flatten = std::min(flatten, t * t * (3.0f - 2.0f * t));
            }

            const float hills = 0.8f * std::sin(x * 0.18f) * std::cos(z * 0.15f) +
                0.35f * std::sin((x + z) * 0.31f);
            return hills * flatten;
        }

        Mesh makeLandscape()
        {
            Mesh mesh;
            constexpr int rowLength = terrainSegments + 1;
            mesh.vertices.reserve(rowLength * rowLength);
            mesh.indices.reserve(terrainSegments * terrainSegments * 6);
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
                    const float grassVariation = std::clamp(0.9f + y * 0.08f, 0.78f, 1.0f);
                    mesh.vertices.push_back({
                        {x, y, z}, normal,
                        {grassVariation, grassVariation, grassVariation},
                        {u * 8.0f, v * 8.0f}
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
                    mesh.indices.insert(mesh.indices.end(), {
                        topLeft, bottomLeft, topRight,
                        topRight, bottomLeft, bottomRight
                    });
                }
            }
            mesh.sections.push_back({0, mesh.indices.size(), {}});
            return mesh;
        }

        Mesh makeGables()
        {
            constexpr float halfWidth = 3.0f;
            constexpr float peakHeight = 1.1f;
            constexpr float halfDepth = 3.0f;
            Mesh mesh;
            mesh.vertices = {
                {{-halfWidth, 0.0f, halfDepth}, {0.0f, 0.0f, 1.0f}},
                {{ halfWidth, 0.0f, halfDepth}, {0.0f, 0.0f, 1.0f}},
                {{0.0f, peakHeight, halfDepth}, {0.0f, 0.0f, 1.0f}},
                {{-halfWidth, 0.0f,-halfDepth}, {0.0f, 0.0f,-1.0f}},
                {{ halfWidth, 0.0f,-halfDepth}, {0.0f, 0.0f,-1.0f}},
                {{0.0f, peakHeight,-halfDepth}, {0.0f, 0.0f,-1.0f}}
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
            Math::Vec3 leftRoomLight,
            Math::Vec3 rightRoomLight
        )
        {
            constexpr float width = 6.0f;
            constexpr float depth = 6.0f;
            constexpr float wallThickness = 0.18f;
            constexpr float floorTop = 0.16f;
            constexpr float wallHeight = 2.7f;
            constexpr float doorWidth = 1.1f;
            constexpr float doorHeight = 2.2f;
            constexpr float roofPeakHeight = 1.1f;
            const float wallCenterY = floorTop + wallHeight * 0.5f;
            const float wallTop = floorTop + wallHeight;
            const float frontZ = center.z + depth * 0.5f - wallThickness * 0.5f;
            const float backZ = center.z - depth * 0.5f + wallThickness * 0.5f;

            addBox(scene, cubeMesh, floorMaterial,
                {center.x, floorTop * 0.5f, center.z}, {width, floorTop, depth});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x, wallCenterY, backZ}, {width, wallHeight, wallThickness});

            const float frontSegmentWidth = (width - doorWidth) * 0.5f;
            addWall(scene, cubeMesh, wallMaterial,
                {center.x - doorWidth * 0.5f - frontSegmentWidth * 0.5f, wallCenterY, frontZ},
                {frontSegmentWidth, wallHeight, wallThickness});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x + doorWidth * 0.5f + frontSegmentWidth * 0.5f, wallCenterY, frontZ},
                {frontSegmentWidth, wallHeight, wallThickness});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x, floorTop + (wallHeight + doorHeight) * 0.5f, frontZ},
                {doorWidth, wallHeight - doorHeight, wallThickness});

            constexpr float interiorDoorWidth = 1.0f;
            const float partitionBack = center.z - depth * 0.5f + wallThickness;
            const float partitionFront = center.z + depth * 0.5f - wallThickness;
            const float interiorDoorCenter = center.z + 1.0f;
            const float doorStart = interiorDoorCenter - interiorDoorWidth * 0.5f;
            const float doorEnd = interiorDoorCenter + interiorDoorWidth * 0.5f;
            const float rearPartitionLength = doorStart - partitionBack;
            const float frontPartitionLength = partitionFront - doorEnd;
            addWall(scene, cubeMesh, wallMaterial,
                {center.x, wallCenterY, (partitionBack + doorStart) * 0.5f},
                {wallThickness, wallHeight, rearPartitionLength});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x, wallCenterY, (doorEnd + partitionFront) * 0.5f},
                {wallThickness, wallHeight, frontPartitionLength});
            addWall(scene, cubeMesh, wallMaterial,
                {center.x, floorTop + (wallHeight + doorHeight) * 0.5f, interiorDoorCenter},
                {wallThickness, wallHeight - doorHeight, interiorDoorWidth});

            constexpr float windowCenterZOffset = -0.4f;
            constexpr float windowWidth = 1.25f;
            constexpr float windowBottom = 1.0f;
            constexpr float windowTop = 2.05f;
            const float windowCenterZ = center.z + windowCenterZOffset;
            const float windowSegmentLength = (depth - windowWidth) * 0.5f;
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
                    {sideX, windowCenterY, windowCenterZ - (windowWidth + windowSegmentLength) * 0.5f},
                    {wallThickness, windowTop - windowBottom, windowSegmentLength});
                addWall(scene, cubeMesh, wallMaterial,
                    {sideX, windowCenterY, windowCenterZ + (windowWidth + windowSegmentLength) * 0.5f},
                    {wallThickness, windowTop - windowBottom, windowSegmentLength});
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

            const float roofAngle = std::atan2(roofPeakHeight, width * 0.5f) * 180.0f / 3.14159265359f;
            const float roofPanelLength = std::sqrt(
                width * width * 0.25f + roofPeakHeight * roofPeakHeight
            ) + 0.2f;
            const float roofCenterY = wallTop + roofPeakHeight * 0.5f;
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

            scene.addPointLight({
                {center.x - 1.5f, floorTop + 2.0f, center.z - 0.5f}, leftRoomLight,
                3.2f, 1.0f, 0.12f, 0.035f
            });
            scene.addPointLight({
                {center.x + 1.5f, floorTop + 2.0f, center.z - 0.5f}, rightRoomLight,
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

    Scene makeDemoScene()
    {
        Scene scene;
        const std::size_t ground = scene.addMesh(makeLandscape());
        const std::size_t cube = scene.addMesh(Mesh::cube());
        const std::size_t sphere = scene.addMesh(Mesh::sphere());
        const std::size_t gable = scene.addMesh(makeGables());
        Mesh pyramid;
        std::string loadError;
        const bool loadedPyramid = Mesh::loadObj("assets/models/pyramid.obj", pyramid, loadError);
        const std::size_t importedMesh = scene.addMesh(loadedPyramid ? std::move(pyramid) : Mesh::cube());
        const std::size_t groundMaterial = scene.addMaterial({{0.5f, 0.72f, 0.38f}});
        const std::size_t wallA = scene.addMaterial({{0.78f, 0.7f, 0.58f}});
        const std::size_t wallB = scene.addMaterial({{0.62f, 0.72f, 0.78f}});
        const std::size_t floor = scene.addMaterial({{0.52f, 0.32f, 0.18f}});
        const std::size_t roofA = scene.addMaterial({{0.42f, 0.16f, 0.1f}});
        const std::size_t roofB = scene.addMaterial({{0.18f, 0.25f, 0.32f}});
        const std::size_t trim = scene.addMaterial({{0.3f, 0.18f, 0.1f}});
        const std::size_t path = scene.addMaterial({{0.5f, 0.48f, 0.4f}});
        const std::size_t bark = scene.addMaterial({{0.28f, 0.16f, 0.08f}});
        const std::size_t foliage = scene.addMaterial({{0.24f, 0.52f, 0.2f}});
        const std::size_t white = scene.addMaterial({{1.0f, 1.0f, 1.0f}});
        const std::size_t fallbackOrange = scene.addMaterial({{1.0f, 0.38f, 0.12f}});
        const std::size_t green = scene.addMaterial({{0.18f, 0.72f, 0.3f}});

        scene.addObject(ground, groundMaterial);
        addHouse(scene, cube, gable, wallA, floor, roofA, trim,
            {-7.0f, 0.0f, houseCenterZ}, {1.0f, 0.48f, 0.2f}, {0.2f, 0.42f, 1.0f});
        addHouse(scene, cube, gable, wallB, floor, roofB, trim,
            {7.0f, 0.0f, houseCenterZ}, {0.25f, 0.9f, 0.3f}, {1.0f, 0.2f, 0.5f});

        for (const float houseX : houseCentersX)
        {
            for (int step = 0; step < 7; ++step)
            {
                const float z = -0.4f + static_cast<float>(step) * 0.9f;
                addBox(scene, cube, path,
                    {houseX, terrainHeight(houseX, z) + 0.06f, z},
                    {0.9f, 0.12f, 0.68f});
            }
        }

        const Math::Vec3 treeLocations[] = {
            {-15.0f, 0.0f, -2.0f}, {-13.0f, 0.0f, 9.0f},
            {14.0f, 0.0f, 1.0f}, {12.0f, 0.0f, 12.0f}, {0.0f, 0.0f, 15.0f}
        };
        for (const Math::Vec3 location : treeLocations)
        {
            const float groundHeight = terrainHeight(location.x, location.z);
            addBox(scene, cube, bark,
                {location.x, groundHeight + 0.8f, location.z}, {0.38f, 1.6f, 0.38f});
            scene.addObject(sphere, foliage,
                {{location.x, groundHeight + 2.0f, location.z}, {0.0f, 0.0f, 0.0f}, {1.5f, 1.8f, 1.5f}});
        }

        scene.addObject(importedMesh, loadedPyramid ? white : fallbackOrange,
            {{0.0f, terrainHeight(0.0f, -15.0f), -15.0f}, {8.0f, -18.0f, 12.0f}, {1.5f, 1.5f, 1.5f}});
        scene.addObject(sphere, green,
            {{0.0f, terrainHeight(0.0f, 3.0f) + 0.9f, 3.0f}, {-15.0f, -28.0f, 6.0f}, {0.9f, 0.9f, 0.9f}});
        return scene;
    }
}
