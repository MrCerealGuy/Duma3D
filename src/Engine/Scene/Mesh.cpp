#include "Engine/Scene/Mesh.hpp"

#include <fstream>
#include <filesystem>
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
        std::size_t normalIndex = 0;
        bool hasNormal = false;
    };

    using MaterialColors = std::unordered_map<std::string, Engine::Math::Vec3>;

    void loadMaterialLibrary(const std::filesystem::path& path, MaterialColors& materialColors)
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
                    materialColors[currentMaterial] = color;
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

    Engine::Math::Vec3 faceNormal(
        Engine::Math::Vec3 a,
        Engine::Math::Vec3 b,
        Engine::Math::Vec3 c
    )
    {
        return Engine::Math::normalize(
            Engine::Math::cross(subtract(b, a), subtract(c, a))
        );
    }
}

namespace Engine::Scene
{
    Mesh Mesh::cube()
    {
        Mesh mesh;
        const auto addFace = [&mesh](Math::Vec3 normal, const Math::Vec3 (&positions)[6])
        {
            for (const Math::Vec3 position : positions)
                mesh.vertices.push_back({position, normal, {1.0f, 1.0f, 1.0f}});
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
        return mesh;
    }

    bool Mesh::loadObj(const std::string& path, Mesh& mesh, std::string& error)
    {
        std::ifstream file(path);
        if (!file)
        {
            error = "Could not open OBJ file: " + path;
            return false;
        }

        std::vector<Math::Vec3> positions;
        std::vector<Math::Vec3> normals;
        Mesh loadedMesh;
        MaterialColors materialColors;
        Math::Vec3 currentDiffuseColor{1.0f, 1.0f, 1.0f};
        const std::filesystem::path objPath(path);
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
            else if (record == "mtllib")
            {
                std::string libraryName;
                while (lineStream >> libraryName)
                    loadMaterialLibrary(objPath.parent_path() / libraryName, materialColors);
            }
            else if (record == "usemtl")
            {
                std::string materialName;
                lineStream >> materialName;
                const auto material = materialColors.find(materialName);
                currentDiffuseColor = material == materialColors.end()
                    ? Math::Vec3{1.0f, 1.0f, 1.0f}
                    : material->second;
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
                    if (!parseFaceVertex(token, positions.size(), normals.size(), vertex))
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

                for (std::size_t i = 1; i + 1 < face.size(); ++i)
                {
                    const ObjFaceVertex triangle[] = {face[0], face[i], face[i + 1]};
                    const Math::Vec3 triangleNormal = faceNormal(
                        positions[triangle[0].positionIndex],
                        positions[triangle[1].positionIndex],
                        positions[triangle[2].positionIndex]
                    );
                    for (const ObjFaceVertex& vertex : triangle)
                    {
                        const Math::Vec3 normal = vertex.hasNormal
                            ? normals[vertex.normalIndex]
                            : triangleNormal;
                        loadedMesh.vertices.push_back({positions[vertex.positionIndex], normal, currentDiffuseColor});
                    }
                }
            }
        }

        if (loadedMesh.vertices.empty())
        {
            error = "OBJ file contains no renderable faces: " + path;
            return false;
        }

        mesh = std::move(loadedMesh);
        error.clear();
        return true;
    }
}
