#include "DemoScene.hpp"

#include "Engine/Graphics/Scene/Mesh.hpp"

#include <array>
#include <cmath>
#include <string>

namespace Duma3D::Demos::Demo_2
{
    namespace Game = Engine::Game;
    namespace GraphicsScene = Engine::Graphics::Scene;
    using ChunkData = Game::World::ChunkData;
    using Scene = GraphicsScene::Scene;
    using Mesh = GraphicsScene::Mesh;
    using Vec3 = Engine::Math::Vec3;

    namespace
    {
        constexpr float roomSize = 10.0f;
        constexpr float wallHeight = 3.2f;
        constexpr float wallThickness = 0.48f;
        constexpr float hallWidth = 4.0f;
        constexpr float roomSpacing = 14.0f;
        constexpr std::array<Vec3, 6> roomCenters{{
            {0.0f, 0.0f, 0.0f}, {14.0f, 0.0f, 0.0f}, {28.0f, 0.0f, 0.0f},
            {28.0f, 0.0f, -14.0f}, {14.0f, 0.0f, -14.0f}, {0.0f, 0.0f, -14.0f}
        }};

        struct Link { int first; int second; bool alongX; };
        constexpr std::array<Link, 6> links{{
            {0, 1, true}, {1, 2, true}, {2, 3, false},
            {3, 4, true}, {4, 5, true}, {5, 0, false}
        }};

        void addBox(ChunkData& chunk, std::size_t cube, std::size_t material,
            Vec3 position, Vec3 scale, bool collider = false)
        {
            chunk.graphics.addObject(cube, material, {position, {}, scale});
            if (collider)
                chunk.addBoxCollider(position, scale);
        }

        void addRoomWall(ChunkData& chunk, std::size_t cube, std::size_t wall,
            Vec3 roomCenter, bool alongX, bool positiveSide, bool connected)
        {
            const float half = roomSize * 0.5f;
            const float halfOpening = hallWidth * 0.5f;
            const float openingSideOffset = halfOpening + (half - halfOpening) * 0.5f;
            const float sideLength = half - halfOpening;
            const Vec3 wallScale = alongX
                ? Vec3{sideLength, wallHeight, wallThickness}
                : Vec3{wallThickness, wallHeight, sideLength};
            const Vec3 wallCenter = alongX
                ? Vec3{roomCenter.x, wallHeight * 0.5f, roomCenter.z + (positiveSide ? half : -half)}
                : Vec3{roomCenter.x + (positiveSide ? half : -half), wallHeight * 0.5f, roomCenter.z};
            if (connected)
            {
                const Vec3 a = alongX
                    ? Vec3{roomCenter.x - openingSideOffset, wallHeight * 0.5f, wallCenter.z}
                    : Vec3{wallCenter.x, wallHeight * 0.5f, roomCenter.z - openingSideOffset};
                const Vec3 b = alongX
                    ? Vec3{roomCenter.x + openingSideOffset, wallHeight * 0.5f, wallCenter.z}
                    : Vec3{wallCenter.x, wallHeight * 0.5f, roomCenter.z + openingSideOffset};
                addBox(chunk, cube, wall, a, wallScale, true);
                addBox(chunk, cube, wall, b, wallScale, true);
                const float headerHeight = wallHeight - 2.65f;
                const Vec3 headerScale = alongX
                    ? Vec3{hallWidth, headerHeight, wallThickness}
                    : Vec3{wallThickness, headerHeight, hallWidth};
                addBox(chunk, cube, wall,
                    {wallCenter.x, 2.65f + headerHeight * 0.5f, wallCenter.z}, headerScale, true);
            }
            else
                addBox(chunk, cube, wall, wallCenter, alongX
                    ? Vec3{roomSize, wallHeight, wallThickness}
                    : Vec3{wallThickness, wallHeight, roomSize}, true);
        }

        void buildRoom(ChunkData& chunk, std::size_t cube, std::size_t floor, std::size_t wall,
            Vec3 center, bool north, bool east, bool south, bool west,
            std::size_t metal, std::size_t lens, Vec3 lightColor, float lightIntensity, bool lampOnNorth)
        {
            addBox(chunk, cube, floor, {center.x, -0.22f, center.z}, {roomSize, 0.44f, roomSize}, true);
            addBox(chunk, cube, floor, {center.x, wallHeight + 0.12f, center.z},
                {roomSize, 0.24f, roomSize}, true);
            addRoomWall(chunk, cube, wall, center, true, false, north);
            addRoomWall(chunk, cube, wall, center, false, true, east);
            addRoomWall(chunk, cube, wall, center, true, true, south);
            addRoomWall(chunk, cube, wall, center, false, false, west);

            // Layered wall luminaire, with its backplate fixed to the inner wall face.
            const float wallDirection = lampOnNorth ? -1.0f : 1.0f;
            const float z = center.z + wallDirection * 4.67f;
            addBox(chunk, cube, metal, {center.x, 2.48f, z}, {1.05f, 0.82f, 0.18f});
            addBox(chunk, cube, metal, {center.x, 2.48f, z - wallDirection * 0.1f}, {0.78f, 0.56f, 0.16f});
            addBox(chunk, cube, lens, {center.x, 2.48f, z - wallDirection * 0.19f}, {0.52f, 0.27f, 0.04f});
            for (const float xOffset : {-0.34f, 0.34f})
                addBox(chunk, cube, metal, {center.x + xOffset, 2.48f, z - wallDirection * 0.21f},
                    {0.06f, 0.56f, 0.05f});
            chunk.graphics.addPointLight({
                {center.x, 2.48f, center.z + wallDirection * 4.05f}, lightColor, lightIntensity,
                1.0f, 0.12f, 0.12f
            });
        }

        Vec3 linkCenter(const Link& link)
        {
            const Vec3 a = roomCenters[link.first];
            const Vec3 b = roomCenters[link.second];
            return {(a.x + b.x) * 0.5f, 0.0f, (a.z + b.z) * 0.5f};
        }

        void buildCorridor(ChunkData& chunk, std::size_t cube, std::size_t floor,
            std::size_t wall, const Link& link)
        {
            const Vec3 center = linkCenter(link);
            const float length = roomSpacing - roomSize;
            if (link.alongX)
            {
                addBox(chunk, cube, floor, {center.x, -0.22f, center.z}, {length, 0.44f, hallWidth}, true);
                addBox(chunk, cube, floor, {center.x, wallHeight + 0.12f, center.z},
                    {length, 0.24f, hallWidth}, true);
                for (const float side : {-1.0f, 1.0f})
                    addBox(chunk, cube, wall,
                        {center.x, wallHeight * 0.5f, center.z + side * (hallWidth * 0.5f)},
                        {length, wallHeight, wallThickness}, true);
            }
            else
            {
                addBox(chunk, cube, floor, {center.x, -0.22f, center.z}, {hallWidth, 0.44f, length}, true);
                addBox(chunk, cube, floor, {center.x, wallHeight + 0.12f, center.z},
                    {hallWidth, 0.24f, length}, true);
                for (const float side : {-1.0f, 1.0f})
                    addBox(chunk, cube, wall,
                        {center.x + side * (hallWidth * 0.5f), wallHeight * 0.5f, center.z},
                        {wallThickness, wallHeight, length}, true);
            }
        }
    }

    ChunkData generateLevel(const LevelState& state)
    {
        ChunkData chunk;
        Scene& scene = chunk.graphics;
        const std::size_t cube = scene.addMesh(Mesh::cube());
        const std::size_t concrete = scene.addMaterial({{0.44f, 0.43f, 0.42f}, "assets/textures/rock.ppm"});
        const std::size_t trim = scene.addMaterial({{0.38f, 0.39f, 0.41f}, "assets/textures/roof_slate.ppm"});
        const std::size_t floor = scene.addMaterial({{0.53f, 0.47f, 0.4f}, "assets/textures/cobblestone.ppm"});
        const std::size_t metal = scene.addMaterial({{0.46f, 0.48f, 0.49f}, "assets/textures/dark_wood.ppm"});
        const std::size_t panel = scene.addMaterial({{0.75f, 0.18f, 0.04f}, "assets/textures/roof_terracotta.ppm"});
        const std::size_t warmLens = scene.addMaterial({{1.0f, 0.62f, 0.2f}, {}});
        const std::size_t redLens = scene.addMaterial({{1.0f, 0.1f, 0.035f}, {}});
        const std::size_t blueLens = scene.addMaterial({{0.12f, 0.58f, 1.0f}, {}});
        const std::size_t indicatorGreen = scene.addMaterial({{0.12f, 0.95f, 0.25f}, {}});
        const std::size_t indicatorRed = scene.addMaterial({{1.0f, 0.06f, 0.025f}, {}});
        const std::size_t sphere = scene.addMesh(Mesh::sphere());
        scene.setDirectionalLight({{0.0f, -1.0f, 0.0f}, {0.035f, 0.04f, 0.05f}, {0.018f, 0.02f, 0.024f}});

        buildRoom(chunk, cube, floor, concrete, roomCenters[0], true, true, false, false,
            metal, warmLens, {1.0f, 0.36f, 0.12f}, 2.1f, false);
        buildRoom(chunk, cube, floor, concrete, roomCenters[1], false, true, false, true,
            metal, warmLens, {1.0f, 0.32f, 0.1f}, 0.85f, true);
        buildRoom(chunk, cube, floor, concrete, roomCenters[2], true, false, false, true,
            metal, blueLens, {0.08f, 0.32f, 1.0f}, 2.5f, false);
        buildRoom(chunk, cube, floor, concrete, roomCenters[3], false, false, true, true,
            metal, redLens, {1.0f, 0.07f, 0.025f}, 0.62f, true);
        buildRoom(chunk, cube, floor, concrete, roomCenters[4], false, true, false, true,
            metal, warmLens, {1.0f, 0.26f, 0.07f}, 2.0f, true);
        buildRoom(chunk, cube, floor, concrete, roomCenters[5], false, true, true, false,
            metal, redLens, {1.0f, 0.08f, 0.025f}, 0.0f, true);

        for (std::size_t index = 0; index < links.size(); ++index)
        {
            const Link& link = links[index];
            const Vec3 center = linkCenter(link);
            buildCorridor(chunk, cube, floor, concrete, link);
            const Vec3 doorScale = link.alongX ? Vec3{0.42f, 2.62f, 3.5f} : Vec3{3.5f, 2.62f, 0.42f};
            const float doorOffset = 4.0f * state.doorOpenAmount[index];
            const Vec3 doorPosition = link.alongX
                ? Vec3{center.x, 1.31f, center.z + doorOffset}
                : Vec3{center.x + doorOffset, 1.31f, center.z};
            addBox(chunk, cube, metal, doorPosition, doorScale, state.doorOpenAmount[index] < 0.98f);
            addBox(chunk, cube, panel, {center.x, 2.9f, center.z},
                link.alongX ? Vec3{0.5f, 0.18f, 1.0f} : Vec3{1.0f, 0.18f, 0.5f});
            // Red guide strips mark the door threshold in the otherwise dim halls.
            addBox(chunk, cube, index == 1 && state.powerOn ? panel : trim,
                {center.x, 0.035f, center.z}, link.alongX ? Vec3{0.32f, 0.07f, 3.6f} : Vec3{3.6f, 0.07f, 0.32f});
        }

        // Wall-mounted relay panel with a large lever and separate status lamps.
        addBox(chunk, cube, metal, {-2.0f, 1.45f, -18.68f}, {1.35f, 1.55f, 0.18f});
        addBox(chunk, cube, trim, {-2.0f, 1.48f, -18.53f}, {0.92f, 1.1f, 0.12f});
        addBox(chunk, cube, panel, {-2.0f, 1.22f, -18.43f}, {0.54f, 0.72f, 0.12f});
        addBox(chunk, cube, metal, {-2.0f, 1.45f, -18.34f}, {0.14f, 0.53f, 0.14f});
        scene.addObject(cube, metal, {{-2.0f, state.powerOn ? 1.61f : 1.3f, -18.29f},
            {state.powerOn ? -28.0f : 28.0f, 0.0f, 0.0f}, {0.16f, 0.48f, 0.16f}});
        scene.addObject(sphere, metal, {{-2.0f, state.powerOn ? 1.82f : 1.51f, -18.24f},
            {}, {0.23f, 0.23f, 0.23f}});
        scene.addObject(sphere, indicatorRed, {{-2.28f, 2.0f, -18.39f}, {}, {0.11f, 0.11f, 0.11f}});
        scene.addObject(sphere, state.powerOn ? indicatorGreen : indicatorRed,
            {{-1.72f, 2.0f, -18.39f}, {}, {0.11f, 0.11f, 0.11f}});
        scene.addPointLight({{-2.0f, 2.08f, -18.15f}, state.powerOn ? Vec3{0.1f, 0.85f, 0.18f} : Vec3{0.9f, 0.04f, 0.015f},
            state.powerOn ? 0.62f : 0.18f, 1.0f, 0.1f, 0.08f});
        // Paired floor pads identify the teleporter endpoints.
        for (const Vec3 point : {Vec3{28.0f, 0.035f, -14.0f}, Vec3{0.0f, 0.035f, -14.0f}})
            addBox(chunk, cube, state.powerOn ? panel : trim, point, {2.5f, 0.07f, 2.5f});
        if (state.powerOn)
        {
            scene.addPointLight({{28.0f, 2.2f, -14.0f}, {0.08f, 0.55f, 1.0f}, 1.4f, 1.0f, 0.18f, 0.2f});
            scene.addPointLight({{0.0f, 2.2f, -14.0f}, {0.08f, 0.55f, 1.0f}, 1.4f, 1.0f, 0.18f, 0.2f});
        }

        return chunk;
    }

    void registerInteractions(Game::InteractionSystem& interactions, LevelState& state,
        Engine::Graphics::Camera& camera)
    {
        interactions.clear();
        const std::array<Vec3, 6> doorPositions{{
            {7.0f, 1.3f, 0.0f}, {21.0f, 1.3f, 0.0f}, {28.0f, 1.3f, -7.0f},
            {21.0f, 1.3f, -14.0f}, {7.0f, 1.3f, -14.0f}, {0.0f, 1.3f, -7.0f}
        }};
        for (std::size_t index = 0; index < doorPositions.size(); ++index)
        {
            interactions.add({doorPositions[index], 3.2f, L"[E] Tür öffnen / schließen", [&state, index]()
            {
                if (index == 1 && !state.powerOn)
                    return Game::InteractionResult{true, false, L"Sicherheitstor ohne Strom"};
                state.doorsOpen[index] = !state.doorsOpen[index];
                return Game::InteractionResult{true, true,
                    state.doorsOpen[index] ? L"Tür fährt langsam auf" : L"Tür fährt langsam zu"};
            }});
        }
        interactions.add({{-2.0f, 1.4f, -18.1f}, 2.8f, L"[E] RELAIS / Stromversorgung", [&state]()
        {
            state.powerOn = !state.powerOn;
            if (!state.powerOn)
                state.doorsOpen[1] = false;
            return Game::InteractionResult{true, true,
                state.powerOn ? L"Stromversorgung aktiv" : L"Stromversorgung getrennt"};
        }});
        interactions.add({{28.0f, 0.8f, -14.0f}, 3.0f, L"[E] Teleporter benutzen", [&state, &camera]()
        {
            if (!state.powerOn)
                return Game::InteractionResult{true, false, L"Teleporter ohne Energie"};
            camera.setPosition({0.0f, 1.7f, -12.0f});
            return Game::InteractionResult{true, false, L"Teleporter: Relaiskammer"};
        }});
        interactions.add({{0.0f, 0.8f, -14.0f}, 3.0f, L"[E] Teleporter benutzen", [&state, &camera]()
        {
            if (!state.powerOn)
                return Game::InteractionResult{true, false, L"Teleporter ohne Energie"};
            camera.setPosition({28.0f, 1.7f, -12.0f});
            return Game::InteractionResult{true, false, L"Teleporter: Kernkammer"};
        }});
    }

    bool advanceDoors(LevelState& state, float deltaTime)
    {
        constexpr float slideSpeed = 0.35f;
        bool changed = false;
        for (std::size_t index = 0; index < state.doorsOpen.size(); ++index)
        {
            const float target = state.doorsOpen[index] ? 1.0f : 0.0f;
            const float current = state.doorOpenAmount[index];
            const float difference = target - current;
            if (std::abs(difference) < 1.0e-5f)
                continue;
            state.doorOpenAmount[index] += std::copysign(
                std::min(std::abs(difference), slideSpeed * deltaTime), difference);
            changed = true;
        }
        return changed;
    }

    void updateLampFlicker(GraphicsScene::Scene& scene, float elapsedTime)
    {
        const float pulse = 0.78f + 0.22f * std::abs(std::sin(elapsedTime * 3.1f));
        const float irregular = std::sin(elapsedTime * 8.7f + 0.7f) * std::sin(elapsedTime * 2.4f);
        const float hallwayLamp = irregular > 0.88f ? 0.07f : 0.85f * pulse;
        const float furnaceLamp = std::sin(elapsedTime * 5.3f + 1.1f) > 0.94f ? 0.035f : 0.62f * pulse;
        const bool deadBulbSputter = std::sin(elapsedTime * 2.2f) > 0.985f &&
            std::sin(elapsedTime * 13.0f + 0.4f) > 0.45f;
        scene.setPointLightIntensity(1, hallwayLamp);
        scene.setPointLightIntensity(3, furnaceLamp);
        scene.setPointLightIntensity(5, deadBulbSputter ? 0.48f : 0.0f);
    }
}
