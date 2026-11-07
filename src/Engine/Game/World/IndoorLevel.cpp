#include "Engine/Game/World/IndoorLevel.hpp"

#include "Engine/Assets/AssetPath.hpp"
#include "Engine/Graphics/Scene/Mesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace Engine::Game::World
{
    namespace
    {
        constexpr float wallThickness = 0.48f;

        [[noreturn]] void parseError(const std::filesystem::path& path, std::size_t line, const std::string& message)
        {
            throw std::runtime_error(path.string() + ":" + std::to_string(line) + ": " + message);
        }

        template <typename T>
        void requireRead(std::istringstream& input, T& value, const std::filesystem::path& path, std::size_t line)
        {
            if (!(input >> value))
                parseError(path, line, "not enough or invalid record fields");
        }

        void requireEnd(std::istringstream& input, const std::filesystem::path& path, std::size_t line)
        {
            std::string extra;
            if (input >> extra)
                parseError(path, line, "unexpected trailing field '" + extra + "'");
        }

        WallSide parseSide(const std::string& value, const std::filesystem::path& path, std::size_t line)
        {
            if (value == "north") return WallSide::North;
            if (value == "east") return WallSide::East;
            if (value == "south") return WallSide::South;
            if (value == "west") return WallSide::West;
            parseError(path, line, "wall side must be north, east, south, or west");
        }

        LampBehavior parseLampBehavior(const std::string& value, const std::filesystem::path& path, std::size_t line)
        {
            if (value == "steady") return LampBehavior::Steady;
            if (value == "flicker") return LampBehavior::Flicker;
            if (value == "sputter") return LampBehavior::Sputter;
            parseError(path, line, "lamp behavior must be steady, flicker, or sputter");
        }

        bool parseOpenState(const std::string& value, const std::filesystem::path& path, std::size_t line)
        {
            if (value == "open") return true;
            if (value == "closed") return false;
            parseError(path, line, "door state must be open or closed");
        }

        std::unordered_map<std::string, std::size_t> indexById(const auto& records)
        {
            std::unordered_map<std::string, std::size_t> result;
            for (std::size_t index = 0; index < records.size(); ++index)
            {
                if (records[index].id.empty() || !result.emplace(records[index].id, index).second)
                    throw std::runtime_error("Indoor level contains a duplicate or empty id");
            }
            return result;
        }

        template <typename Records>
        std::size_t requireId(const std::unordered_map<std::string, std::size_t>& indexes,
            const std::string& id, const Records&, const char* category)
        {
            const auto found = indexes.find(id);
            if (found == indexes.end())
                throw std::runtime_error("Indoor level references unknown " + std::string(category) + " '" + id + "'");
            return found->second;
        }

        std::array<bool, 4> roomOpenings(const IndoorLevelDefinition& definition, std::size_t roomIndex,
            const std::unordered_map<std::string, std::size_t>& rooms)
        {
            std::array<bool, 4> connected{};
            const IndoorRoom& room = definition.rooms[roomIndex];
            for (const IndoorLink& link : definition.links)
            {
                const bool isA = link.roomA == room.id;
                const bool isB = link.roomB == room.id;
                if (!isA && !isB)
                    continue;
                const IndoorRoom& other = definition.rooms[rooms.at(isA ? link.roomB : link.roomA)];
                if (other.x > room.x) connected[1] = true;
                else if (other.x < room.x) connected[3] = true;
                else if (other.z < room.z) connected[0] = true;
                else connected[2] = true;
            }
            return connected;
        }

        Math::Vec3 sideNormal(WallSide side)
        {
            switch (side)
            {
                case WallSide::North: return {0.0f, 0.0f, -1.0f};
                case WallSide::East: return {1.0f, 0.0f, 0.0f};
                case WallSide::South: return {0.0f, 0.0f, 1.0f};
                case WallSide::West: return {-1.0f, 0.0f, 0.0f};
            }
            return {};
        }

        Math::Vec3 sideTangent(WallSide side)
        {
            return side == WallSide::North || side == WallSide::South
                ? Math::Vec3{1.0f, 0.0f, 0.0f} : Math::Vec3{0.0f, 0.0f, 1.0f};
        }

        Math::Vec3 alongSideScale(WallSide side, float tangent, float y, float normal)
        {
            return side == WallSide::North || side == WallSide::South
                ? Math::Vec3{tangent, y, normal} : Math::Vec3{normal, y, tangent};
        }

        Math::Vec3 roomPosition(const IndoorRoom& room, float y = 0.0f)
        {
            return {room.x, y, room.z};
        }

        void addBox(ChunkData& chunk, std::size_t cube, std::size_t material,
            Math::Vec3 position, Math::Vec3 scale, bool collider = false, Math::Vec3 rotation = {})
        {
            chunk.graphics.addObject(cube, material, {position, rotation, scale});
            if (collider)
                chunk.addBoxCollider(position, scale);
        }

        void addWall(ChunkData& chunk, std::size_t cube, std::size_t material,
            Math::Vec3 roomCenter, WallSide side, float roomSize, float hallWidth, float wallHeight, bool connected)
        {
            const Math::Vec3 normal = sideNormal(side);
            const Math::Vec3 tangent = sideTangent(side);
            const Math::Vec3 wallCenter{
                roomCenter.x + normal.x * roomSize * 0.5f,
                wallHeight * 0.5f,
                roomCenter.z + normal.z * roomSize * 0.5f
            };
            const float halfOpening = hallWidth * 0.5f;
            const float sideLength = roomSize * 0.5f - halfOpening;
            const float tangentOffset = halfOpening + sideLength * 0.5f;
            if (connected)
            {
                for (const float sign : {-1.0f, 1.0f})
                {
                    const Math::Vec3 position{
                        wallCenter.x + tangent.x * tangentOffset * sign,
                        wallCenter.y,
                        wallCenter.z + tangent.z * tangentOffset * sign
                    };
                    addBox(chunk, cube, material, position,
                        alongSideScale(side, sideLength, wallHeight, wallThickness), true);
                }
                const float headerHeight = wallHeight - 2.65f;
                addBox(chunk, cube, material,
                    {wallCenter.x, 2.65f + headerHeight * 0.5f, wallCenter.z},
                    alongSideScale(side, hallWidth, headerHeight, wallThickness), true);
            }
            else
                addBox(chunk, cube, material, wallCenter,
                    alongSideScale(side, roomSize, wallHeight, wallThickness), true);
        }

        Math::Vec3 lampMount(const IndoorRoom& room, const IndoorLevelDefinition& definition,
            WallSide side, float offset, float inset = 0.09f)
        {
            const Math::Vec3 normal = sideNormal(side);
            const Math::Vec3 tangent = sideTangent(side);
            const float wallFace = definition.roomSize * 0.5f - wallThickness * 0.5f;
            return {room.x + normal.x * (wallFace - inset) + tangent.x * offset,
                0.0f,
                room.z + normal.z * (wallFace - inset) + tangent.z * offset};
        }

        std::unordered_map<std::string, std::size_t> roomIndexes(const IndoorLevelDefinition& definition)
        {
            std::unordered_map<std::string, std::size_t> result;
            for (std::size_t index = 0; index < definition.rooms.size(); ++index)
                result.emplace(definition.rooms[index].id, index);
            return result;
        }

        std::unordered_map<std::string, std::size_t> doorIndexes(const IndoorLevelDefinition& definition)
        {
            std::unordered_map<std::string, std::size_t> result;
            for (std::size_t index = 0; index < definition.doors.size(); ++index)
                result.emplace(definition.doors[index].id, index);
            return result;
        }

        const IndoorRoom& getRoom(const IndoorLevelDefinition& definition,
            const std::unordered_map<std::string, std::size_t>& indexes, const std::string& id)
        {
            return definition.rooms.at(indexes.at(id));
        }

        std::size_t materialIndex(const std::unordered_map<std::string, std::size_t>& indexes, const std::string& id)
        {
            return indexes.at(id);
        }

        bool isPowered(const IndoorLevelDefinition& definition, const IndoorLevelState& state, const std::string& switchId)
        {
            if (switchId == "-")
                return true;
            for (std::size_t index = 0; index < definition.switches.size(); ++index)
            {
                if (definition.switches[index].id == switchId)
                    return state.switchesOn.at(index);
            }
            return false;
        }
    }

    IndoorLevelDefinition IndoorLevelLoader::load(const std::filesystem::path& assetPath)
    {
        const std::filesystem::path filePath = Assets::resolveAssetPath(assetPath);
        std::ifstream file(filePath);
        if (!file)
            throw std::runtime_error("Could not open indoor level: " + filePath.string());

        IndoorLevelDefinition definition;
        std::string lineText;
        std::size_t lineNumber = 0;
        bool headerRead = false;
        bool nameRead = false;
        bool settingsRead = false;
        bool spawnRead = false;
        while (std::getline(file, lineText))
        {
            ++lineNumber;
            const std::size_t comment = lineText.find('#');
            if (comment != std::string::npos)
                lineText.erase(comment);
            std::istringstream input(lineText);
            std::string record;
            if (!(input >> record))
                continue;
            if (!headerRead)
            {
                int version = 0;
                if (record != "DUMA3D_LEVEL")
                    parseError(filePath, lineNumber, "expected DUMA3D_LEVEL header");
                requireRead(input, version, filePath, lineNumber);
                requireEnd(input, filePath, lineNumber);
                if (version != 1)
                    parseError(filePath, lineNumber, "unsupported level format version");
                headerRead = true;
                continue;
            }

            if (record == "name")
            {
                if (nameRead) parseError(filePath, lineNumber, "duplicate name record");
                requireRead(input, definition.name, filePath, lineNumber);
                nameRead = true;
            }
            else if (record == "settings")
            {
                if (settingsRead) parseError(filePath, lineNumber, "duplicate settings record");
                requireRead(input, definition.roomSize, filePath, lineNumber);
                requireRead(input, definition.roomSpacing, filePath, lineNumber);
                requireRead(input, definition.corridorWidth, filePath, lineNumber);
                requireRead(input, definition.wallHeight, filePath, lineNumber);
                settingsRead = true;
            }
            else if (record == "material")
            {
                LevelMaterial material;
                requireRead(input, material.id, filePath, lineNumber);
                requireRead(input, material.texturePath, filePath, lineNumber);
                requireRead(input, material.tint.x, filePath, lineNumber);
                requireRead(input, material.tint.y, filePath, lineNumber);
                requireRead(input, material.tint.z, filePath, lineNumber);
                if (material.texturePath == "-") material.texturePath.clear();
                definition.materials.push_back(std::move(material));
            }
            else if (record == "room")
            {
                IndoorRoom room;
                requireRead(input, room.id, filePath, lineNumber);
                requireRead(input, room.x, filePath, lineNumber);
                requireRead(input, room.z, filePath, lineNumber);
                definition.rooms.push_back(std::move(room));
            }
            else if (record == "link")
            {
                IndoorLink link;
                requireRead(input, link.roomA, filePath, lineNumber);
                requireRead(input, link.roomB, filePath, lineNumber);
                requireRead(input, link.doorId, filePath, lineNumber);
                definition.links.push_back(std::move(link));
            }
            else if (record == "door")
            {
                IndoorDoor door;
                std::string state;
                requireRead(input, door.id, filePath, lineNumber);
                requireRead(input, state, filePath, lineNumber);
                requireRead(input, door.powerSwitch, filePath, lineNumber);
                requireRead(input, door.material, filePath, lineNumber);
                door.initiallyOpen = parseOpenState(state, filePath, lineNumber);
                definition.doors.push_back(std::move(door));
            }
            else if (record == "lamp")
            {
                IndoorLamp lamp;
                std::string side, behavior;
                requireRead(input, lamp.room, filePath, lineNumber);
                requireRead(input, side, filePath, lineNumber);
                requireRead(input, lamp.offset, filePath, lineNumber);
                requireRead(input, lamp.material, filePath, lineNumber);
                requireRead(input, lamp.lightColor.x, filePath, lineNumber);
                requireRead(input, lamp.lightColor.y, filePath, lineNumber);
                requireRead(input, lamp.lightColor.z, filePath, lineNumber);
                requireRead(input, lamp.intensity, filePath, lineNumber);
                requireRead(input, behavior, filePath, lineNumber);
                lamp.side = parseSide(side, filePath, lineNumber);
                lamp.behavior = parseLampBehavior(behavior, filePath, lineNumber);
                definition.lamps.push_back(std::move(lamp));
            }
            else if (record == "switch")
            {
                IndoorSwitch levelSwitch;
                std::string side;
                requireRead(input, levelSwitch.id, filePath, lineNumber);
                requireRead(input, levelSwitch.room, filePath, lineNumber);
                requireRead(input, side, filePath, lineNumber);
                requireRead(input, levelSwitch.offset, filePath, lineNumber);
                requireRead(input, levelSwitch.poweredDoor, filePath, lineNumber);
                requireRead(input, levelSwitch.bodyMaterial, filePath, lineNumber);
                requireRead(input, levelSwitch.handleMaterial, filePath, lineNumber);
                requireRead(input, levelSwitch.offIndicatorMaterial, filePath, lineNumber);
                requireRead(input, levelSwitch.onIndicatorMaterial, filePath, lineNumber);
                levelSwitch.side = parseSide(side, filePath, lineNumber);
                definition.switches.push_back(std::move(levelSwitch));
            }
            else if (record == "teleporter")
            {
                IndoorTeleporter teleporter;
                requireRead(input, teleporter.id, filePath, lineNumber);
                requireRead(input, teleporter.room, filePath, lineNumber);
                requireRead(input, teleporter.offsetX, filePath, lineNumber);
                requireRead(input, teleporter.offsetZ, filePath, lineNumber);
                requireRead(input, teleporter.destination, filePath, lineNumber);
                requireRead(input, teleporter.material, filePath, lineNumber);
                requireRead(input, teleporter.powerSwitch, filePath, lineNumber);
                definition.teleporters.push_back(std::move(teleporter));
            }
            else if (record == "spawn")
            {
                if (spawnRead) parseError(filePath, lineNumber, "duplicate spawn record");
                requireRead(input, definition.spawn.room, filePath, lineNumber);
                requireRead(input, definition.spawn.offsetX, filePath, lineNumber);
                requireRead(input, definition.spawn.offsetZ, filePath, lineNumber);
                requireRead(input, definition.spawn.yawDegrees, filePath, lineNumber);
                spawnRead = true;
            }
            else
                parseError(filePath, lineNumber, "unknown record '" + record + "'");
            requireEnd(input, filePath, lineNumber);
        }
        if (!headerRead)
            throw std::runtime_error("Indoor level is empty: " + filePath.string());

        const auto materials = indexById(definition.materials);
        const auto rooms = indexById(definition.rooms);
        const auto doors = indexById(definition.doors);
        const auto switches = indexById(definition.switches);
        const auto teleporters = indexById(definition.teleporters);
        if (!nameRead || !settingsRead || !spawnRead || definition.name.empty() || definition.rooms.empty() || definition.links.empty() ||
            definition.materials.empty() || definition.doors.empty())
            throw std::runtime_error("Indoor level requires a name, materials, rooms, links, and doors");
        if (!std::isfinite(definition.roomSize) || !std::isfinite(definition.roomSpacing) ||
            !std::isfinite(definition.corridorWidth) || !std::isfinite(definition.wallHeight) ||
            definition.roomSize <= 0.0f || definition.roomSpacing <= definition.roomSize ||
            definition.corridorWidth <= 0.0f || definition.corridorWidth >= definition.roomSize ||
            definition.wallHeight <= 2.65f)
            throw std::runtime_error("Indoor level has invalid room or corridor settings");

        for (const char* role : {"wall", "floor", "metal", "trim", "warning", "teleporter_on"})
        {
            if (!materials.contains(role))
                throw std::runtime_error("Indoor level is missing conventional material '" + std::string(role) + "'");
        }

        for (const LevelMaterial& material : definition.materials)
        {
            if (!std::isfinite(material.tint.x) || !std::isfinite(material.tint.y) || !std::isfinite(material.tint.z))
                throw std::runtime_error("Indoor level material has a non-finite tint");
        }
        for (const IndoorRoom& room : definition.rooms)
        {
            if (!std::isfinite(room.x) || !std::isfinite(room.z))
                throw std::runtime_error("Indoor level room coordinates must be finite");
        }
        std::unordered_set<std::string> linkedDoors;
        std::unordered_map<std::string, std::array<bool, 4>> openings;
        for (const IndoorLink& link : definition.links)
        {
            const IndoorRoom& a = definition.rooms[requireId(rooms, link.roomA, definition.rooms, "room")];
            const IndoorRoom& b = definition.rooms[requireId(rooms, link.roomB, definition.rooms, "room")];
            requireId(doors, link.doorId, definition.doors, "door");
            if (!linkedDoors.insert(link.doorId).second)
                throw std::runtime_error("Each door must be attached to one link");
            WallSide sideA;
            WallSide sideB;
            if (std::abs(a.x - b.x) < 0.001f && std::abs(std::abs(a.z - b.z) - definition.roomSpacing) < 0.001f)
            {
                sideA = b.z < a.z ? WallSide::North : WallSide::South;
                sideB = b.z < a.z ? WallSide::South : WallSide::North;
            }
            else if (std::abs(a.z - b.z) < 0.001f && std::abs(std::abs(a.x - b.x) - definition.roomSpacing) < 0.001f)
            {
                sideA = b.x > a.x ? WallSide::East : WallSide::West;
                sideB = b.x > a.x ? WallSide::West : WallSide::East;
            }
            else
                throw std::runtime_error("Linked rooms must be axis-aligned neighbors separated by roomSpacing");
            const std::size_t indexA = static_cast<std::size_t>(sideA);
            const std::size_t indexB = static_cast<std::size_t>(sideB);
            if (openings[link.roomA][indexA] || openings[link.roomB][indexB])
                throw std::runtime_error("A room wall side can only have one corridor opening");
            openings[link.roomA][indexA] = true;
            openings[link.roomB][indexB] = true;
        }
        for (const IndoorDoor& door : definition.doors)
        {
            if (!linkedDoors.contains(door.id))
                throw std::runtime_error("Door '" + door.id + "' has no link");
            requireId(materials, door.material, definition.materials, "material");
            if (door.powerSwitch != "-")
                requireId(switches, door.powerSwitch, definition.switches, "switch");
        }
        for (const IndoorLamp& lamp : definition.lamps)
        {
            requireId(rooms, lamp.room, definition.rooms, "room");
            requireId(materials, lamp.material, definition.materials, "material");
            if (!std::isfinite(lamp.lightColor.x) || !std::isfinite(lamp.lightColor.y) ||
                !std::isfinite(lamp.lightColor.z) || !std::isfinite(lamp.intensity) || lamp.intensity < 0.0f || !std::isfinite(lamp.offset) ||
                std::abs(lamp.offset) >= definition.roomSize * 0.5f - 0.6f)
                throw std::runtime_error("Lamp intensity or wall offset is outside its valid range");
            if (openings[lamp.room][static_cast<std::size_t>(lamp.side)])
                throw std::runtime_error("A lamp cannot be attached to a wall with a corridor opening");
        }
        for (const IndoorSwitch& levelSwitch : definition.switches)
        {
            requireId(rooms, levelSwitch.room, definition.rooms, "room");
            requireId(doors, levelSwitch.poweredDoor, definition.doors, "door");
            requireId(materials, levelSwitch.bodyMaterial, definition.materials, "material");
            requireId(materials, levelSwitch.handleMaterial, definition.materials, "material");
            requireId(materials, levelSwitch.offIndicatorMaterial, definition.materials, "material");
            requireId(materials, levelSwitch.onIndicatorMaterial, definition.materials, "material");
            if (std::abs(levelSwitch.offset) >= definition.roomSize * 0.5f - 0.8f)
                throw std::runtime_error("Switch offset is outside its room wall");
            if (openings[levelSwitch.room][static_cast<std::size_t>(levelSwitch.side)])
                throw std::runtime_error("A switch cannot be attached to a wall with a corridor opening");
            if (definition.doors[doors.at(levelSwitch.poweredDoor)].powerSwitch != levelSwitch.id)
                throw std::runtime_error("Switch and door power requirements must reference each other");
        }
        for (const IndoorTeleporter& teleporter : definition.teleporters)
        {
            requireId(rooms, teleporter.room, definition.rooms, "room");
            requireId(teleporters, teleporter.destination, definition.teleporters, "teleporter");
            requireId(materials, teleporter.material, definition.materials, "material");
            if (teleporter.powerSwitch != "-")
                requireId(switches, teleporter.powerSwitch, definition.switches, "switch");
            if (!std::isfinite(teleporter.offsetX) || !std::isfinite(teleporter.offsetZ) ||
                std::abs(teleporter.offsetX) >= definition.roomSize * 0.5f - 1.3f ||
                std::abs(teleporter.offsetZ) >= definition.roomSize * 0.5f - 1.3f)
                throw std::runtime_error("Teleporter pad is outside its room floor");
        }
        requireId(rooms, definition.spawn.room, definition.rooms, "spawn room");
        if (!std::isfinite(definition.spawn.offsetX) || !std::isfinite(definition.spawn.offsetZ) ||
            !std::isfinite(definition.spawn.yawDegrees) ||
            std::abs(definition.spawn.offsetX) >= definition.roomSize * 0.5f - 0.5f ||
            std::abs(definition.spawn.offsetZ) >= definition.roomSize * 0.5f - 0.5f)
            throw std::runtime_error("Spawn point must be finite and inside its room");
        if (definition.lamps.size() + definition.switches.size() + definition.teleporters.size() >
            Graphics::Scene::Scene::maximumPointLights)
            throw std::runtime_error("Indoor level exceeds the engine point-light limit");
        return definition;
    }

    IndoorLevelState IndoorLevelBuilder::initialState(const IndoorLevelDefinition& definition)
    {
        IndoorLevelState state;
        for (const IndoorDoor& door : definition.doors)
        {
            state.doorsOpen.push_back(door.initiallyOpen && door.powerSwitch == "-");
            state.doorOpenAmount.push_back(state.doorsOpen.back() ? 1.0f : 0.0f);
        }
        state.switchesOn.resize(definition.switches.size(), false);
        return state;
    }

    ChunkData IndoorLevelBuilder::build(const IndoorLevelDefinition& definition, const IndoorLevelState& state)
    {
        ChunkData chunk;
        auto& scene = chunk.graphics;
        const std::size_t cube = scene.addMesh(Graphics::Scene::Mesh::cube());
        const std::size_t sphere = scene.addMesh(Graphics::Scene::Mesh::sphere());
        std::unordered_map<std::string, std::size_t> materialIds;
        for (const LevelMaterial& material : definition.materials)
            materialIds.emplace(material.id, scene.addMaterial({material.tint, material.texturePath}));
        const auto rooms = roomIndexes(definition);
        const auto doors = doorIndexes(definition);
        scene.setDirectionalLight({{0.0f, -1.0f, 0.0f}, {0.035f, 0.04f, 0.05f}, {0.018f, 0.02f, 0.024f}});

        for (std::size_t index = 0; index < definition.rooms.size(); ++index)
        {
            const IndoorRoom& room = definition.rooms[index];
            addBox(chunk, cube, materialIndex(materialIds, "floor"),
                {room.x, -0.22f, room.z}, {definition.roomSize, 0.44f, definition.roomSize}, true);
            addBox(chunk, cube, materialIndex(materialIds, "floor"),
                {room.x, definition.wallHeight + 0.12f, room.z},
                {definition.roomSize, 0.24f, definition.roomSize}, true);
            const auto openings = roomOpenings(definition, index, rooms);
            for (std::size_t side = 0; side < openings.size(); ++side)
                addWall(chunk, cube, materialIndex(materialIds, "wall"), roomPosition(room),
                    static_cast<WallSide>(side), definition.roomSize, definition.corridorWidth,
                    definition.wallHeight, openings[side]);
        }

        for (const IndoorLamp& lamp : definition.lamps)
        {
            const IndoorRoom& room = getRoom(definition, rooms, lamp.room);
            const Math::Vec3 normal = sideNormal(lamp.side);
            const Math::Vec3 plate = lampMount(room, definition, lamp.side, lamp.offset);
            const auto towardRoom = [&normal](Math::Vec3 position, float amount)
            {
                position.x -= normal.x * amount;
                position.z -= normal.z * amount;
                return position;
            };
            addBox(chunk, cube, materialIndex(materialIds, "metal"),
                {plate.x, 2.48f, plate.z}, alongSideScale(lamp.side, 1.05f, 0.82f, 0.18f));
            addBox(chunk, cube, materialIndex(materialIds, "metal"),
                towardRoom({plate.x, 2.48f, plate.z}, 0.1f), alongSideScale(lamp.side, 0.78f, 0.56f, 0.16f));
            addBox(chunk, cube, materialIndex(materialIds, lamp.material),
                towardRoom({plate.x, 2.48f, plate.z}, 0.19f), alongSideScale(lamp.side, 0.52f, 0.27f, 0.04f));
            for (const float tangentOffset : {-0.34f, 0.34f})
            {
                const Math::Vec3 tangent = sideTangent(lamp.side);
                Math::Vec3 grille = towardRoom({plate.x + tangent.x * tangentOffset, 2.48f,
                    plate.z + tangent.z * tangentOffset}, 0.21f);
                addBox(chunk, cube, materialIndex(materialIds, "metal"), grille,
                    alongSideScale(lamp.side, 0.06f, 0.56f, 0.05f));
            }
            scene.addPointLight({
                {room.x + normal.x * (definition.roomSize * 0.5f - 0.95f), 2.48f,
                    room.z + normal.z * (definition.roomSize * 0.5f - 0.95f)},
                lamp.lightColor,
                lamp.behavior == LampBehavior::Sputter ? 0.0f : lamp.intensity,
                1.0f, 0.12f, 0.12f
            });
        }

        for (const IndoorLink& link : definition.links)
        {
            const IndoorRoom& a = getRoom(definition, rooms, link.roomA);
            const IndoorRoom& b = getRoom(definition, rooms, link.roomB);
            const bool alongX = std::abs(a.x - b.x) > 0.001f;
            const float length = definition.roomSpacing - definition.roomSize;
            const Math::Vec3 center{(a.x + b.x) * 0.5f, 0.0f, (a.z + b.z) * 0.5f};
            if (alongX)
            {
                addBox(chunk, cube, materialIndex(materialIds, "floor"), {center.x, -0.22f, center.z},
                    {length, 0.44f, definition.corridorWidth}, true);
                addBox(chunk, cube, materialIndex(materialIds, "floor"), {center.x, definition.wallHeight + 0.12f, center.z},
                    {length, 0.24f, definition.corridorWidth}, true);
                for (const float side : {-1.0f, 1.0f})
                    addBox(chunk, cube, materialIndex(materialIds, "wall"),
                        {center.x, definition.wallHeight * 0.5f, center.z + side * definition.corridorWidth * 0.5f},
                        {length, definition.wallHeight, wallThickness}, true);
            }
            else
            {
                addBox(chunk, cube, materialIndex(materialIds, "floor"), {center.x, -0.22f, center.z},
                    {definition.corridorWidth, 0.44f, length}, true);
                addBox(chunk, cube, materialIndex(materialIds, "floor"), {center.x, definition.wallHeight + 0.12f, center.z},
                    {definition.corridorWidth, 0.24f, length}, true);
                for (const float side : {-1.0f, 1.0f})
                    addBox(chunk, cube, materialIndex(materialIds, "wall"),
                        {center.x + side * definition.corridorWidth * 0.5f, definition.wallHeight * 0.5f, center.z},
                        {wallThickness, definition.wallHeight, length}, true);
            }

            const std::size_t doorIndex = doors.at(link.doorId);
            const IndoorDoor& door = definition.doors[doorIndex];
            const float amount = state.doorOpenAmount.at(doorIndex);
            const float slide = 4.0f * amount;
            const Math::Vec3 doorCenter = alongX
                ? Math::Vec3{center.x, 1.31f, center.z + slide}
                : Math::Vec3{center.x + slide, 1.31f, center.z};
            addBox(chunk, cube, materialIndex(materialIds, door.material), doorCenter,
                alongX ? Math::Vec3{0.42f, 2.62f, 3.5f} : Math::Vec3{3.5f, 2.62f, 0.42f},
                amount < 0.98f);
            addBox(chunk, cube, materialIndex(materialIds, "warning"), {center.x, 2.9f, center.z},
                alongX ? Math::Vec3{0.5f, 0.18f, 1.0f} : Math::Vec3{1.0f, 0.18f, 0.5f});
            addBox(chunk, cube, materialIndex(materialIds,
                door.powerSwitch != "-" && isPowered(definition, state, door.powerSwitch) ? "warning" : "trim"),
                {center.x, 0.035f, center.z},
                alongX ? Math::Vec3{0.32f, 0.07f, 3.6f} : Math::Vec3{3.6f, 0.07f, 0.32f});
        }

        for (std::size_t index = 0; index < definition.switches.size(); ++index)
        {
            const IndoorSwitch& levelSwitch = definition.switches[index];
            const IndoorRoom& room = getRoom(definition, rooms, levelSwitch.room);
            const Math::Vec3 normal = sideNormal(levelSwitch.side);
            const Math::Vec3 tangent = sideTangent(levelSwitch.side);
            const Math::Vec3 plate = lampMount(room, definition, levelSwitch.side, levelSwitch.offset);
            const auto inRoom = [&normal](Math::Vec3 position, float amount)
            {
                position.x -= normal.x * amount;
                position.z -= normal.z * amount;
                return position;
            };
            const bool on = state.switchesOn.at(index);
            addBox(chunk, cube, materialIndex(materialIds, levelSwitch.bodyMaterial),
                {plate.x, 1.45f, plate.z}, alongSideScale(levelSwitch.side, 1.35f, 1.55f, 0.18f));
            addBox(chunk, cube, materialIndex(materialIds, levelSwitch.handleMaterial),
                inRoom({plate.x, 1.48f, plate.z}, 0.14f), alongSideScale(levelSwitch.side, 0.92f, 1.1f, 0.12f));
            addBox(chunk, cube, materialIndex(materialIds, "warning"),
                inRoom({plate.x, 1.22f, plate.z}, 0.25f), alongSideScale(levelSwitch.side, 0.54f, 0.72f, 0.12f));
            addBox(chunk, cube, materialIndex(materialIds, levelSwitch.handleMaterial),
                inRoom({plate.x, 1.45f, plate.z}, 0.34f), alongSideScale(levelSwitch.side, 0.14f, 0.53f, 0.14f));
            const float leverAngle = on ? -28.0f : 28.0f;
            const Math::Vec3 lever = inRoom({plate.x, on ? 1.61f : 1.3f, plate.z}, 0.41f);
            addBox(chunk, cube, materialIndex(materialIds, levelSwitch.handleMaterial), lever,
                alongSideScale(levelSwitch.side, 0.16f, 0.48f, 0.16f), false,
                levelSwitch.side == WallSide::North || levelSwitch.side == WallSide::South
                    ? Math::Vec3{leverAngle, 0.0f, 0.0f} : Math::Vec3{0.0f, 0.0f, leverAngle});
            const Math::Vec3 knob = inRoom({plate.x, on ? 1.82f : 1.51f, plate.z}, 0.47f);
            scene.addObject(sphere, materialIndex(materialIds, levelSwitch.handleMaterial),
                {{knob.x, knob.y, knob.z}, {}, {0.23f, 0.23f, 0.23f}});
            for (const float side : {-1.0f, 1.0f})
            {
                Math::Vec3 led = {plate.x + tangent.x * 0.28f * side, 2.0f,
                    plate.z + tangent.z * 0.28f * side};
                led = inRoom(led, 0.29f);
                const std::size_t ledMaterial = side > 0.0f
                    ? materialIndex(materialIds, on ? levelSwitch.onIndicatorMaterial : levelSwitch.offIndicatorMaterial)
                    : materialIndex(materialIds, levelSwitch.offIndicatorMaterial);
                scene.addObject(sphere, ledMaterial, {{led.x, led.y, led.z}, {}, {0.11f, 0.11f, 0.11f}});
            }
            scene.addPointLight({inRoom({plate.x, 2.08f, plate.z}, 0.66f),
                on ? Math::Vec3{0.1f, 0.85f, 0.18f} : Math::Vec3{0.9f, 0.04f, 0.015f},
                on ? 0.62f : 0.18f, 1.0f, 0.1f, 0.08f});
        }

        for (const IndoorTeleporter& teleporter : definition.teleporters)
        {
            const IndoorRoom& room = getRoom(definition, rooms, teleporter.room);
            const bool powered = isPowered(definition, state, teleporter.powerSwitch);
            const std::size_t padMaterial = materialIndex(materialIds, teleporter.material);
            const std::size_t highlight = materialIndex(materialIds, powered ? "teleporter_on" : "trim");
            const Math::Vec3 pad{room.x + teleporter.offsetX, 0.035f, room.z + teleporter.offsetZ};
            addBox(chunk, cube, padMaterial, pad, {2.5f, 0.07f, 2.5f});
            addBox(chunk, cube, highlight, {pad.x, 0.08f, pad.z}, {1.65f, 0.03f, 1.65f});
            if (powered)
                scene.addPointLight({{pad.x, 2.2f, pad.z}, {0.08f, 0.55f, 1.0f}, 1.4f, 1.0f, 0.18f, 0.2f});
        }
        return chunk;
    }

    void IndoorLevelBuilder::registerInteractions(const IndoorLevelDefinition& definition, IndoorLevelState& state,
        InteractionSystem& interactions, Graphics::Camera& camera)
    {
        interactions.clear();
        const auto rooms = roomIndexes(definition);
        const auto doors = doorIndexes(definition);
        for (const IndoorLink& link : definition.links)
        {
            const IndoorRoom& a = getRoom(definition, rooms, link.roomA);
            const IndoorRoom& b = getRoom(definition, rooms, link.roomB);
            const Math::Vec3 point{(a.x + b.x) * 0.5f, 1.3f, (a.z + b.z) * 0.5f};
            const std::size_t doorIndex = doors.at(link.doorId);
            interactions.add({point, 3.2f, L"[E] Tür öffnen / schließen", [&definition, &state, doorIndex]()
            {
                const IndoorDoor& targetDoor = definition.doors[doorIndex];
                if (!isPowered(definition, state, targetDoor.powerSwitch))
                    return InteractionResult{true, false, L"Sicherheitstor ohne Strom", InteractionSound::Denied};
                state.doorsOpen[doorIndex] = !state.doorsOpen[doorIndex];
                return InteractionResult{true, true,
                    state.doorsOpen[doorIndex] ? L"Tür fährt langsam auf" : L"Tür fährt langsam zu",
                    state.doorsOpen[doorIndex] ? InteractionSound::DoorOpen : InteractionSound::DoorClose};
            }});
        }
        for (std::size_t index = 0; index < definition.switches.size(); ++index)
        {
            const IndoorSwitch& levelSwitch = definition.switches[index];
            const IndoorRoom& room = getRoom(definition, rooms, levelSwitch.room);
            const Math::Vec3 mount = lampMount(room, definition, levelSwitch.side, levelSwitch.offset, 0.09f);
            const Math::Vec3 normal = sideNormal(levelSwitch.side);
            const Math::Vec3 target{mount.x - normal.x * 0.8f, 1.4f, mount.z - normal.z * 0.8f};
            interactions.add({target, 2.8f, L"[E] RELAIS / Stromversorgung", [&definition, &state, index]()
            {
                state.switchesOn[index] = !state.switchesOn[index];
                const std::string& switchId = definition.switches[index].id;
                if (!state.switchesOn[index])
                {
                    for (std::size_t door = 0; door < definition.doors.size(); ++door)
                    {
                        if (definition.doors[door].powerSwitch == switchId)
                            state.doorsOpen[door] = false;
                    }
                }
                return InteractionResult{true, true,
                    state.switchesOn[index] ? L"Stromversorgung aktiv" : L"Stromversorgung getrennt",
                    InteractionSound::Switch};
            }});
        }
        for (const IndoorTeleporter& teleporter : definition.teleporters)
        {
            const IndoorRoom& room = getRoom(definition, rooms, teleporter.room);
            const auto destination = std::find_if(definition.teleporters.begin(), definition.teleporters.end(),
                [&teleporter](const IndoorTeleporter& item) { return item.id == teleporter.destination; });
            const IndoorRoom& destinationRoom = getRoom(definition, rooms, destination->room);
            const Math::Vec3 destinationPosition{
                destinationRoom.x + destination->offsetX,
                1.7f,
                destinationRoom.z + destination->offsetZ + 1.5f
            };
            const Math::Vec3 point{room.x + teleporter.offsetX, 0.8f, room.z + teleporter.offsetZ + 0.8f};
            const std::string powerSwitch = teleporter.powerSwitch;
            interactions.add({point, 3.0f, L"[E] Teleporter benutzen", [&definition, &state, &camera,
                teleporter, powerSwitch, destinationPosition]()
            {
                if (!isPowered(definition, state, powerSwitch))
                    return InteractionResult{true, false, L"Teleporter ohne Energie", InteractionSound::Denied};
                camera.setPosition(destinationPosition);
                return InteractionResult{true, false, L"Teleporter aktiviert", InteractionSound::Teleporter};
            }});
        }
    }

    bool IndoorLevelBuilder::advanceDoors(const IndoorLevelDefinition& definition, IndoorLevelState& state, float deltaTime)
    {
        constexpr float slideSpeed = 0.35f;
        bool changed = false;
        for (std::size_t index = 0; index < definition.doors.size(); ++index)
        {
            const float target = state.doorsOpen.at(index) ? 1.0f : 0.0f;
            const float current = state.doorOpenAmount.at(index);
            const float difference = target - current;
            if (std::abs(difference) < 1.0e-5f)
                continue;
            state.doorOpenAmount[index] += std::copysign(
                std::min(std::abs(difference), slideSpeed * deltaTime), difference);
            changed = true;
        }
        return changed;
    }

    bool IndoorLevelBuilder::updateLampFlicker(const IndoorLevelDefinition& definition,
        Graphics::Scene::Scene& scene, float elapsedTime)
    {
        bool sputtering = false;
        for (std::size_t index = 0; index < definition.lamps.size(); ++index)
        {
            const IndoorLamp& lamp = definition.lamps[index];
            const float phase = elapsedTime + static_cast<float>(index) * 1.71f;
            float intensity = lamp.intensity;
            if (lamp.behavior == LampBehavior::Flicker)
            {
                const float pulse = 0.78f + 0.22f * std::abs(std::sin(phase * 3.1f));
                const float irregular = std::sin(phase * 8.7f + 0.7f) * std::sin(phase * 2.4f);
                intensity *= irregular > 0.88f ? 0.08f : pulse;
            }
            else if (lamp.behavior == LampBehavior::Sputter)
            {
                const bool lampSputtering = std::sin(phase * 2.2f) > 0.985f &&
                    std::sin(phase * 13.0f + 0.4f) > 0.45f;
                sputtering = sputtering || lampSputtering;
                intensity = lampSputtering ? std::max(0.35f, lamp.intensity) : 0.0f;
            }
            scene.setPointLightIntensity(index, intensity);
        }
        return sputtering;
    }

    void IndoorLevelBuilder::placeAtSpawn(const IndoorLevelDefinition& definition, Graphics::Camera& camera)
    {
        const auto rooms = roomIndexes(definition);
        const IndoorRoom& room = getRoom(definition, rooms, definition.spawn.room);
        camera.setPosition({room.x + definition.spawn.offsetX, 1.7f, room.z + definition.spawn.offsetZ});
        camera.rotate(definition.spawn.yawDegrees + 90.0f, 0.0f);
    }
}
