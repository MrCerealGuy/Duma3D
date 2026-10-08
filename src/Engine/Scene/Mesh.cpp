#include "Engine/Scene/Mesh.hpp"
#include "Engine/Assets/AssetPath.hpp"

#include <algorithm>
#include <fstream>
#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    struct ObjFaceVertex
    {
        std::size_t positionIndex = 0;
        std::size_t textureCoordinateIndex = 0;
        std::size_t normalIndex = 0;
        bool hasTextureCoordinate = false;
        bool hasNormal = false;
    };

    struct ObjMaterial
    {
        Engine::Math::Vec3 diffuseColor{1.0f, 1.0f, 1.0f};
        Engine::Math::Vec3 specularColor{0.04f, 0.04f, 0.04f};
        Engine::Math::Vec3 emissiveColor{0.0f, 0.0f, 0.0f};
        float shininess = 32.0f;
        std::string diffuseTexturePath;
    };

    using Materials = std::unordered_map<std::string, ObjMaterial>;

    struct SmoothNormalKey
    {
        std::size_t positionIndex = 0;
        std::size_t smoothingGroup = 0;

        bool operator==(const SmoothNormalKey&) const = default;
    };

    struct SmoothNormalKeyHash
    {
        std::size_t operator()(const SmoothNormalKey& key) const
        {
            return key.positionIndex ^ (key.smoothingGroup + 0x9e3779b9 +
                (key.positionIndex << 6) + (key.positionIndex >> 2));
        }
    };

    Engine::Math::Vec3 add(Engine::Math::Vec3 a, Engine::Math::Vec3 b)
    {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    struct VertexKey
    {
        std::size_t positionIndex;
        std::size_t textureCoordinateIndex;
        std::uint32_t normalX;
        std::uint32_t normalY;
        std::uint32_t normalZ;
        std::uint32_t colorX;
        std::uint32_t colorY;
        std::uint32_t colorZ;
        std::uint32_t specularX;
        std::uint32_t specularY;
        std::uint32_t specularZ;
        std::uint32_t emissiveX;
        std::uint32_t emissiveY;
        std::uint32_t emissiveZ;
        std::uint32_t shininess;
        std::size_t smoothingGroup;

        bool operator==(const VertexKey&) const = default;
    };

    struct VertexKeyHash
    {
        std::size_t operator()(const VertexKey& key) const
        {
            std::size_t hash = key.positionIndex;
            const auto combine = [&hash](std::size_t value)
            {
                hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
            };
            combine(key.textureCoordinateIndex);
            combine(key.normalX);
            combine(key.normalY);
            combine(key.normalZ);
            combine(key.colorX);
            combine(key.colorY);
            combine(key.colorZ);
            combine(key.specularX);
            combine(key.specularY);
            combine(key.specularZ);
            combine(key.emissiveX);
            combine(key.emissiveY);
            combine(key.emissiveZ);
            combine(key.shininess);
            combine(key.smoothingGroup);
            return hash;
        }
    };

    void loadMaterialLibrary(const std::filesystem::path& path, Materials& materials)
    {
        std::ifstream file(path);
        if (!file)
            return;

        std::string currentMaterial;
        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream lineStream(line);
            std::string record;
            lineStream >> record;
            if (record == "newmtl")
            {
                lineStream >> currentMaterial;
            }
            else if (record == "Kd" && !currentMaterial.empty())
            {
                Engine::Math::Vec3 color{};
                if (lineStream >> color.x >> color.y >> color.z)
                    materials[currentMaterial].diffuseColor = color;
            }
            else if (record == "Ks" && !currentMaterial.empty())
            {
                Engine::Math::Vec3 color{};
                if (lineStream >> color.x >> color.y >> color.z)
                    materials[currentMaterial].specularColor = color;
            }
            else if (record == "Ke" && !currentMaterial.empty())
            {
                Engine::Math::Vec3 color{};
                if (lineStream >> color.x >> color.y >> color.z)
                    materials[currentMaterial].emissiveColor = color;
            }
            else if (record == "Ns" && !currentMaterial.empty())
            {
                float shininess = 0.0f;
                if (lineStream >> shininess)
                    materials[currentMaterial].shininess = std::max(shininess, 1.0f);
            }
            else if (record == "map_Kd" && !currentMaterial.empty())
            {
                std::string texturePath;
                std::getline(lineStream >> std::ws, texturePath);
                if (!texturePath.empty())
                {
                    materials[currentMaterial].diffuseTexturePath =
                        std::filesystem::absolute(path.parent_path() / texturePath).lexically_normal().string();
                }
            }
        }
    }

    bool parseObjIndex(const std::string& text, std::size_t count, std::size_t& index)
    {
        if (text.empty())
            return false;

        std::size_t parsedCharacters = 0;
        int objIndex = 0;
        try
        {
            objIndex = std::stoi(text, &parsedCharacters);
        }
        catch (...)
        {
            return false;
        }
        if (parsedCharacters != text.size() || objIndex == 0)
            return false;

        const long long resolved = objIndex > 0
            ? static_cast<long long>(objIndex) - 1
            : static_cast<long long>(count) + objIndex;
        if (resolved < 0 || static_cast<unsigned long long>(resolved) >= count)
            return false;

        index = static_cast<std::size_t>(resolved);
        return true;
    }

    bool parseFaceVertex(
        const std::string& token,
        std::size_t positionCount,
        std::size_t textureCoordinateCount,
        std::size_t normalCount,
        ObjFaceVertex& vertex
    )
    {
        const std::size_t firstSlash = token.find('/');
        const std::size_t secondSlash = firstSlash == std::string::npos
            ? std::string::npos
            : token.find('/', firstSlash + 1);
        const std::string position = token.substr(0, firstSlash);

        if (!parseObjIndex(position, positionCount, vertex.positionIndex))
            return false;

        if (firstSlash != std::string::npos)
        {
            const std::size_t textureEnd = secondSlash == std::string::npos ? token.size() : secondSlash;
            const std::string textureCoordinate = token.substr(firstSlash + 1, textureEnd - firstSlash - 1);
            if (!textureCoordinate.empty())
            {
                if (!parseObjIndex(textureCoordinate, textureCoordinateCount, vertex.textureCoordinateIndex))
                    return false;
                vertex.hasTextureCoordinate = true;
            }
        }

        if (secondSlash != std::string::npos && secondSlash + 1 < token.size())
        {
            const std::string normal = token.substr(secondSlash + 1);
            if (!parseObjIndex(normal, normalCount, vertex.normalIndex))
                return false;
            vertex.hasNormal = true;
        }
        return true;
    }

    Engine::Math::Vec3 subtract(Engine::Math::Vec3 a, Engine::Math::Vec3 b)
    {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

}

namespace Engine::Scene
{
    Mesh Mesh::cube()
    {
        Mesh mesh;
        const auto addFace = [&mesh](Math::Vec3 normal, const Math::Vec3 (&positions)[6])
        {
            const std::uint32_t firstVertex = static_cast<std::uint32_t>(mesh.vertices.size());
            mesh.vertices.push_back({positions[0], normal, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}});
            mesh.vertices.push_back({positions[1], normal, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}});
            mesh.vertices.push_back({positions[2], normal, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}});
            mesh.vertices.push_back({positions[4], normal, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}});
            mesh.indices.insert(mesh.indices.end(), {
                firstVertex, firstVertex + 1, firstVertex + 2,
                firstVertex + 2, firstVertex + 3, firstVertex
            });
        };

        const Math::Vec3 back[] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
            { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}
        };
        addFace({0.0f, 0.0f, -1.0f}, back);

        const Math::Vec3 front[] = {
            {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}
        };
        addFace({0.0f, 0.0f, 1.0f}, front);

        const Math::Vec3 left[] = {
            {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
            {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
        };
        addFace({-1.0f, 0.0f, 0.0f}, left);

        const Math::Vec3 right[] = {
            { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
            { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}
        };
        addFace({1.0f, 0.0f, 0.0f}, right);

        const Math::Vec3 bottom[] = {
            {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
            { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f}
        };
        addFace({0.0f, -1.0f, 0.0f}, bottom);

        const Math::Vec3 top[] = {
            {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
            { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}
        };
        addFace({0.0f, 1.0f, 0.0f}, top);
        mesh.sections.push_back({0, mesh.indices.size(), {}});
        return mesh;
    }

    Mesh Mesh::plane(float size)
    {
        const float halfSize = size * 0.5f;
        Mesh mesh;
        mesh.vertices = {
            {{-halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
            {{ halfSize, 0.0f, -halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {size, 0.0f}},
            {{ halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {size, size}},
            {{-halfSize, 0.0f,  halfSize}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, size}}
        };
        mesh.indices = {0, 2, 1, 0, 3, 2};
        mesh.sections.push_back({0, mesh.indices.size(), {}});
        return mesh;
    }

    Mesh Mesh::sphere()
    {
        constexpr int longitudeSegments = 24;
        constexpr int latitudeSegments = 16;
        constexpr float pi = 3.14159265359f;
        constexpr int rowLength = longitudeSegments + 1;

        Mesh mesh;
        mesh.vertices.reserve((latitudeSegments + 1) * rowLength);
        mesh.indices.reserve(latitudeSegments * longitudeSegments * 6);
        for (int latitude = 0; latitude <= latitudeSegments; ++latitude)
        {
            const float v = static_cast<float>(latitude) / latitudeSegments;
            const float phi = v * pi;
            const float ringRadius = std::sin(phi);
            const float y = std::cos(phi);
            for (int longitude = 0; longitude <= longitudeSegments; ++longitude)
            {
                const float u = static_cast<float>(longitude) / longitudeSegments;
                const float theta = u * 2.0f * pi;
                const Math::Vec3 normal{
                    ringRadius * std::cos(theta),
                    y,
                    ringRadius * std::sin(theta)
                };
                mesh.vertices.push_back({normal, normal, {1.0f, 1.0f, 1.0f}, {u, 1.0f - v}});
            }
        }

        for (int latitude = 0; latitude < latitudeSegments; ++latitude)
        {
            for (int longitude = 0; longitude < longitudeSegments; ++longitude)
            {
                const std::uint32_t topLeft = static_cast<std::uint32_t>(latitude * rowLength + longitude);
                const std::uint32_t bottomLeft = topLeft + rowLength;
                const std::uint32_t topRight = topLeft + 1;
                const std::uint32_t bottomRight = bottomLeft + 1;
                if (latitude > 0)
                    mesh.indices.insert(mesh.indices.end(), {topLeft, topRight, bottomLeft});
                if (latitude + 1 < latitudeSegments)
                    mesh.indices.insert(mesh.indices.end(), {topRight, bottomRight, bottomLeft});
            }
        }
        mesh.sections.push_back({0, mesh.indices.size(), {}});
        return mesh;
    }

    bool Mesh::loadObj(const std::string& path, Mesh& mesh, std::string& error)
    {
        const std::filesystem::path objPath = Assets::resolveAssetPath(path);
        std::ifstream file(objPath);
        if (!file)
        {
            error = "Could not open OBJ file: " + path;
            return false;
        }

        std::vector<Math::Vec3> positions;
        std::vector<Math::Vec2> textureCoordinates;
        std::vector<Math::Vec3> normals;
        Mesh loadedMesh;
        Materials materials;
        std::unordered_map<VertexKey, std::uint32_t, VertexKeyHash> vertexLookup;
        std::unordered_map<std::string, std::size_t> smoothingGroups;
        std::unordered_map<SmoothNormalKey, Math::Vec3, SmoothNormalKeyHash> smoothNormalSums;
        std::vector<SmoothNormalKey> vertexSmoothNormalKeys;
        Math::Vec3 currentDiffuseColor{1.0f, 1.0f, 1.0f};
        Math::Vec3 currentSpecularColor{0.04f, 0.04f, 0.04f};
        Math::Vec3 currentEmissiveColor{0.0f, 0.0f, 0.0f};
        float currentShininess = 32.0f;
        std::size_t currentSmoothingGroup = 0;
        std::size_t nextSmoothingGroup = 1;
        std::string currentDiffuseTexturePath;
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(file, line))
        {
            ++lineNumber;
            std::istringstream lineStream(line);
            std::string record;
            lineStream >> record;

            if (record == "v")
            {
                Math::Vec3 position{};
                if (!(lineStream >> position.x >> position.y >> position.z))
                {
                    error = "Invalid vertex at " + path + ":" + std::to_string(lineNumber);
                    return false;
                }
                positions.push_back(position);
            }
            else if (record == "vn")
            {
                Math::Vec3 normal{};
                if (!(lineStream >> normal.x >> normal.y >> normal.z))
                {
                    error = "Invalid normal at " + path + ":" + std::to_string(lineNumber);
                    return false;
                }
                normals.push_back(Math::normalize(normal));
            }
            else if (record == "vt")
            {
                Math::Vec2 textureCoordinate{};
                if (!(lineStream >> textureCoordinate.x >> textureCoordinate.y))
                {
                    error = "Invalid texture coordinate at " + path + ":" + std::to_string(lineNumber);
                    return false;
                }
                textureCoordinates.push_back(textureCoordinate);
            }
            else if (record == "mtllib")
            {
                std::string libraryName;
                while (lineStream >> libraryName)
                    loadMaterialLibrary(objPath.parent_path() / libraryName, materials);
            }
            else if (record == "usemtl")
            {
                std::string materialName;
                lineStream >> materialName;
                const auto material = materials.find(materialName);
                currentDiffuseColor = material == materials.end()
                    ? Math::Vec3{1.0f, 1.0f, 1.0f}
                    : material->second.diffuseColor;
                currentSpecularColor = material == materials.end()
                    ? Math::Vec3{0.04f, 0.04f, 0.04f}
                    : material->second.specularColor;
                currentEmissiveColor = material == materials.end()
                    ? Math::Vec3{0.0f, 0.0f, 0.0f}
                    : material->second.emissiveColor;
                currentShininess = material == materials.end()
                    ? 32.0f
                    : material->second.shininess;
                currentDiffuseTexturePath = material == materials.end()
                    ? std::string{}
                    : material->second.diffuseTexturePath;
            }
            else if (record == "s")
            {
                std::string groupName;
                lineStream >> groupName;
                if (groupName.empty() || groupName == "off" || groupName == "0")
                {
                    currentSmoothingGroup = 0;
                }
                else
                {
                    if (groupName == "on")
                        groupName = "1";
                    auto [entry, inserted] = smoothingGroups.try_emplace(groupName, nextSmoothingGroup);
                    if (inserted)
                        ++nextSmoothingGroup;
                    currentSmoothingGroup = entry->second;
                }
            }
            else if (record == "f")
            {
                std::vector<ObjFaceVertex> face;
                std::string token;
                while (lineStream >> token)
                {
                    if (token.starts_with('#'))
                        break;

                    ObjFaceVertex vertex;
                    if (!parseFaceVertex(token, positions.size(), textureCoordinates.size(), normals.size(), vertex))
                    {
                        error = "Invalid face index at " + path + ":" + std::to_string(lineNumber);
                        return false;
                    }
                    face.push_back(vertex);
                }
                if (face.size() < 3)
                {
                    error = "Face has fewer than three vertices at " + path + ":" + std::to_string(lineNumber);
                    return false;
                }

                const std::size_t sectionStart = loadedMesh.indices.size();
                const bool canExtendSection = !loadedMesh.sections.empty() &&
                    loadedMesh.sections.back().firstIndex + loadedMesh.sections.back().indexCount == sectionStart &&
                    loadedMesh.sections.back().diffuseTexturePath == currentDiffuseTexturePath;
                if (!canExtendSection)
                    loadedMesh.sections.push_back({sectionStart, 0, currentDiffuseTexturePath});

                for (std::size_t i = 1; i + 1 < face.size(); ++i)
                {
                    const ObjFaceVertex triangle[] = {face[0], face[i], face[i + 1]};
                    const Math::Vec3 faceNormalVector = Math::cross(
                        subtract(positions[triangle[1].positionIndex], positions[triangle[0].positionIndex]),
                        subtract(positions[triangle[2].positionIndex], positions[triangle[0].positionIndex])
                    );
                    const Math::Vec3 triangleNormal = Math::normalize(faceNormalVector);
                    for (const ObjFaceVertex& vertex : triangle)
                    {
                        const bool hasSmoothGeneratedNormal = !vertex.hasNormal && currentSmoothingGroup != 0;
                        const Math::Vec3 normal = vertex.hasNormal
                            ? normals[vertex.normalIndex]
                            : hasSmoothGeneratedNormal ? Math::Vec3{0.0f, 0.0f, 0.0f} : triangleNormal;
                        const SmoothNormalKey smoothKey{
                            vertex.positionIndex,
                            hasSmoothGeneratedNormal ? currentSmoothingGroup : 0
                        };
                        const Math::Vec2 textureCoordinate = vertex.hasTextureCoordinate
                            ? textureCoordinates[vertex.textureCoordinateIndex]
                            : Math::Vec2{0.0f, 0.0f};
                        const std::size_t textureCoordinateKey = vertex.hasTextureCoordinate
                            ? vertex.textureCoordinateIndex
                            : std::numeric_limits<std::size_t>::max();
                        const VertexKey key{
                            vertex.positionIndex,
                            textureCoordinateKey,
                            std::bit_cast<std::uint32_t>(normal.x),
                            std::bit_cast<std::uint32_t>(normal.y),
                            std::bit_cast<std::uint32_t>(normal.z),
                            std::bit_cast<std::uint32_t>(currentDiffuseColor.x),
                            std::bit_cast<std::uint32_t>(currentDiffuseColor.y),
                            std::bit_cast<std::uint32_t>(currentDiffuseColor.z),
                            std::bit_cast<std::uint32_t>(currentSpecularColor.x),
                            std::bit_cast<std::uint32_t>(currentSpecularColor.y),
                            std::bit_cast<std::uint32_t>(currentSpecularColor.z),
                            std::bit_cast<std::uint32_t>(currentEmissiveColor.x),
                            std::bit_cast<std::uint32_t>(currentEmissiveColor.y),
                            std::bit_cast<std::uint32_t>(currentEmissiveColor.z),
                            std::bit_cast<std::uint32_t>(currentShininess),
                            smoothKey.smoothingGroup
                        };
                        auto [entry, inserted] = vertexLookup.try_emplace(
                            key, static_cast<std::uint32_t>(loadedMesh.vertices.size())
                        );
                        if (inserted)
                        {
                            loadedMesh.vertices.push_back({
                                positions[vertex.positionIndex], normal, currentDiffuseColor, textureCoordinate,
                                currentSpecularColor, currentShininess, currentEmissiveColor
                            });
                            vertexSmoothNormalKeys.push_back(smoothKey);
                        }
                        loadedMesh.indices.push_back(entry->second);
                        loadedMesh.sections.back().indexCount += 1;
                        if (hasSmoothGeneratedNormal)
                        {
                            auto& sum = smoothNormalSums[smoothKey];
                            sum = add(sum, faceNormalVector);
                        }
                    }
                }
            }
        }

        if (loadedMesh.vertices.empty())
        {
            error = "OBJ file contains no renderable faces: " + path;
            return false;
        }

        for (std::size_t index = 0; index < loadedMesh.vertices.size(); ++index)
        {
            const SmoothNormalKey& key = vertexSmoothNormalKeys[index];
            if (key.smoothingGroup != 0)
                loadedMesh.vertices[index].normal = Math::normalize(smoothNormalSums[key]);
        }

        mesh = std::move(loadedMesh);
        error.clear();
        return true;
    }
}
