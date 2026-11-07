#pragma once

#include "Engine/Game/InteractionSystem.hpp"
#include "Engine/Game/World/ChunkData.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace Engine::Game::World
{
    enum class WallSide { North, East, South, West };
    enum class LampBehavior { Steady, Flicker, Sputter };

    struct LevelMaterial
    {
        std::string id;
        std::string texturePath;
        Math::Vec3 tint{1.0f, 1.0f, 1.0f};
    };

    struct IndoorRoom
    {
        std::string id;
        float x = 0.0f;
        float z = 0.0f;
    };

    struct IndoorLink
    {
        std::string roomA;
        std::string roomB;
        std::string doorId;
    };

    struct IndoorDoor
    {
        std::string id;
        bool initiallyOpen = false;
        std::string powerSwitch;
        std::string material;
    };

    struct IndoorLamp
    {
        std::string room;
        WallSide side = WallSide::North;
        float offset = 0.0f;
        std::string material;
        Math::Vec3 lightColor{1.0f, 1.0f, 1.0f};
        float intensity = 1.0f;
        LampBehavior behavior = LampBehavior::Steady;
    };

    struct IndoorSwitch
    {
        std::string id;
        std::string room;
        WallSide side = WallSide::North;
        float offset = 0.0f;
        std::string poweredDoor;
        std::string bodyMaterial;
        std::string handleMaterial;
        std::string offIndicatorMaterial;
        std::string onIndicatorMaterial;
    };

    struct IndoorTeleporter
    {
        std::string id;
        std::string room;
        float offsetX = 0.0f;
        float offsetZ = 0.0f;
        std::string destination;
        std::string material;
        std::string powerSwitch;
    };

    struct IndoorEnemy
    {
        std::string id;
        std::string room;
        float offsetX = 0.0f;
        float offsetZ = 0.0f;
        std::string type;
    };

    struct IndoorSpawn
    {
        std::string room;
        float offsetX = 0.0f;
        float offsetZ = 0.0f;
        float yawDegrees = -90.0f;
    };

    struct IndoorLevelDefinition
    {
        std::string name;
        float roomSize = 10.0f;
        float roomSpacing = 14.0f;
        float corridorWidth = 4.0f;
        float wallHeight = 3.2f;
        std::vector<LevelMaterial> materials;
        std::vector<IndoorRoom> rooms;
        std::vector<IndoorLink> links;
        std::vector<IndoorDoor> doors;
        std::vector<IndoorLamp> lamps;
        std::vector<IndoorSwitch> switches;
        std::vector<IndoorTeleporter> teleporters;
        std::vector<IndoorEnemy> enemies;
        IndoorSpawn spawn;
    };

    struct IndoorLevelState
    {
        std::vector<bool> doorsOpen;
        std::vector<float> doorOpenAmount;
        std::vector<bool> switchesOn;
    };

    class IndoorLevelLoader
    {
    public:
        // Loads and validates the Duma3D Indoor Level Format, version 1.
        static IndoorLevelDefinition load(const std::filesystem::path& assetPath);
    };

    class IndoorLevelBuilder
    {
    public:
        static IndoorLevelState initialState(const IndoorLevelDefinition& definition);
        static ChunkData build(const IndoorLevelDefinition& definition, const IndoorLevelState& state);
        static void registerInteractions(const IndoorLevelDefinition& definition, IndoorLevelState& state,
            InteractionSystem& interactions, Graphics::Camera& camera);
        static bool advanceDoors(const IndoorLevelDefinition& definition, IndoorLevelState& state, float deltaTime);
        static bool updateLampFlicker(const IndoorLevelDefinition& definition,
            Graphics::Scene::Scene& scene, float elapsedTime);
        static void placeAtSpawn(const IndoorLevelDefinition& definition, Graphics::Camera& camera);
    };
}
