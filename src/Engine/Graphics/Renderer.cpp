#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Assets/AssetPath.hpp"

#include <gl/GL.h>

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

using GLsizeiptr = std::ptrdiff_t;

using PFNGLGENVERTEXARRAYSPROC = void (APIENTRY *)(int, unsigned int*);
using PFNGLBINDVERTEXARRAYPROC = void (APIENTRY *)(unsigned int);
using PFNGLDELETEVERTEXARRAYSPROC = void (APIENTRY *)(int, const unsigned int*);
using PFNGLGENBUFFERSPROC = void (APIENTRY *)(int, unsigned int*);
using PFNGLBINDBUFFERPROC = void (APIENTRY *)(unsigned int, unsigned int);
using PFNGLBUFFERDATAPROC = void (APIENTRY *)(unsigned int, GLsizeiptr, const void*, unsigned int);
using PFNGLCREATESHADERPROC = unsigned int (APIENTRY *)(unsigned int);
using PFNGLSHADERSOURCEPROC = void (APIENTRY *)(unsigned int, int, const char* const*, const int*);
using PFNGLCOMPILESHADERPROC = void (APIENTRY *)(unsigned int);
using PFNGLGETSHADERIVPROC = void (APIENTRY *)(unsigned int, unsigned int, int*);
using PFNGLGETSHADERINFOLOGPROC = void (APIENTRY *)(unsigned int, int, int*, char*);
using PFNGLDELETESHADERPROC = void (APIENTRY *)(unsigned int);
using PFNGLCREATEPROGRAMPROC = unsigned int (APIENTRY *)();
using PFNGLATTACHSHADERPROC = void (APIENTRY *)(unsigned int, unsigned int);
using PFNGLLINKPROGRAMPROC = void (APIENTRY *)(unsigned int);
using PFNGLGETPROGRAMIVPROC = void (APIENTRY *)(unsigned int, unsigned int, int*);
using PFNGLGETPROGRAMINFOLOGPROC = void (APIENTRY *)(unsigned int, int, int*, char*);
using PFNGLUSEPROGRAMPROC = void (APIENTRY *)(unsigned int);
using PFNGLDELETEPROGRAMPROC = void (APIENTRY *)(unsigned int);
using PFNGLGETUNIFORMLOCATIONPROC = int (APIENTRY *)(unsigned int, const char*);
using PFNGLUNIFORMMATRIX4FVPROC = void (APIENTRY *)(int, int, unsigned char, const float*);
using PFNGLUNIFORM3FPROC = void (APIENTRY *)(int, float, float, float);
using PFNGLUNIFORM1IPROC = void (APIENTRY *)(int, int);
using PFNGLUNIFORM1FPROC = void (APIENTRY *)(int, float);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void (APIENTRY *)(unsigned int);
using PFNGLVERTEXATTRIBPOINTERPROC = void (APIENTRY *)(unsigned int, int, unsigned int, unsigned char, int, const void*);
using PFNGLDELETEBUFFERSPROC = void (APIENTRY *)(int, const unsigned int*);

static PFNGLGENVERTEXARRAYSPROC glGenVertexArraysPtr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArrayPtr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArraysPtr;
static PFNGLGENBUFFERSPROC glGenBuffersPtr;
static PFNGLBINDBUFFERPROC glBindBufferPtr;
static PFNGLBUFFERDATAPROC glBufferDataPtr;
static PFNGLCREATESHADERPROC glCreateShaderPtr;
static PFNGLSHADERSOURCEPROC glShaderSourcePtr;
static PFNGLCOMPILESHADERPROC glCompileShaderPtr;
static PFNGLGETSHADERIVPROC glGetShaderivPtr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLogPtr;
static PFNGLDELETESHADERPROC glDeleteShaderPtr;
static PFNGLCREATEPROGRAMPROC glCreateProgramPtr;
static PFNGLATTACHSHADERPROC glAttachShaderPtr;
static PFNGLLINKPROGRAMPROC glLinkProgramPtr;
static PFNGLGETPROGRAMIVPROC glGetProgramivPtr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLogPtr;
static PFNGLUSEPROGRAMPROC glUseProgramPtr;
static PFNGLDELETEPROGRAMPROC glDeleteProgramPtr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationPtr;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fvPtr;
static PFNGLUNIFORM3FPROC glUniform3fPtr;
static PFNGLUNIFORM1IPROC glUniform1iPtr;
static PFNGLUNIFORM1FPROC glUniform1fPtr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArrayPtr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointerPtr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersPtr;

namespace
{
    struct ImageData
    {
        int width = 1;
        int height = 1;
        std::vector<unsigned char> rgb{255, 255, 255};
    };

    void* getGLProc(const char* name)
    {
        void* proc = reinterpret_cast<void*>(wglGetProcAddress(name));
        if (proc == nullptr || proc == reinterpret_cast<void*>(0x1) ||
            proc == reinterpret_cast<void*>(0x2) || proc == reinterpret_cast<void*>(0x3) ||
            proc == reinterpret_cast<void*>(-1))
        {
            HMODULE module = GetModuleHandleW(L"opengl32.dll");
            proc = reinterpret_cast<void*>(GetProcAddress(module, name));
        }
        return proc;
    }

    bool loadProc(void** target, const char* name)
    {
        *target = getGLProc(name);
        return *target != nullptr;
    }

    std::string readText(const char* path)
    {
        std::ifstream file(Engine::Assets::resolveAssetPath(path), std::ios::binary);
        if (!file)
            return {};

        std::ostringstream stream;
        stream << file.rdbuf();
        return stream.str();
    }

    bool readPpmToken(std::istream& stream, std::string& token)
    {
        token.clear();
        while (stream)
        {
            const int next = stream.peek();
            if (next == '#')
            {
                stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            else if (next != EOF && std::isspace(static_cast<unsigned char>(next)))
            {
                const char whitespace = static_cast<char>(stream.get());
                if (whitespace == '\r' && stream.peek() == '\n')
                    stream.get();
            }
            else
            {
                break;
            }
        }

        while (stream)
        {
            const int next = stream.peek();
            if (next == EOF || next == '#' || std::isspace(static_cast<unsigned char>(next)))
                break;
            token.push_back(static_cast<char>(stream.get()));
        }
        if (token.empty())
            return false;

        if (stream.peek() != EOF && std::isspace(static_cast<unsigned char>(stream.peek())))
        {
            const char whitespace = static_cast<char>(stream.get());
            if (whitespace == '\r' && stream.peek() == '\n')
                stream.get();
        }
        return true;
    }

    bool loadPpm(const std::string& path, ImageData& image)
    {
        std::ifstream file(Engine::Assets::resolveAssetPath(path), std::ios::binary);
        std::string token;
        if (!file || !readPpmToken(file, token) || (token != "P3" && token != "P6"))
            return false;
        const bool binary = token == "P6";

        const auto readPositiveInteger = [&file](int& value)
        {
            std::string integer;
            if (!readPpmToken(file, integer))
                return false;
            try
            {
                std::size_t parsed = 0;
                value = std::stoi(integer, &parsed);
                return parsed == integer.size() && value > 0;
            }
            catch (...)
            {
                return false;
            }
        };

        int width = 0;
        int height = 0;
        int maxValue = 0;
        if (!readPositiveInteger(width) || !readPositiveInteger(height) ||
            !readPositiveInteger(maxValue) || maxValue > 255)
            return false;

        const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (pixelCount > std::numeric_limits<std::size_t>::max() / 3)
            return false;
        std::vector<unsigned char> pixels(pixelCount * 3);

        if (binary)
        {
            file.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
            if (file.gcount() != static_cast<std::streamsize>(pixels.size()))
                return false;
            if (maxValue != 255)
            {
                for (unsigned char& channel : pixels)
                    channel = static_cast<unsigned char>(static_cast<int>(channel) * 255 / maxValue);
            }
        }
        else
        {
            for (unsigned char& channel : pixels)
            {
                if (!readPpmToken(file, token))
                    return false;
                try
                {
                    std::size_t parsed = 0;
                    const int value = std::stoi(token, &parsed);
                    if (parsed != token.size() || value < 0 || value > maxValue)
                        return false;
                    channel = static_cast<unsigned char>(value * 255 / maxValue);
                }
                catch (...)
                {
                    return false;
                }
            }
        }

        const std::size_t rowSize = static_cast<std::size_t>(width) * 3;
        for (int y = 0; y < height / 2; ++y)
        {
            const std::size_t top = static_cast<std::size_t>(y) * rowSize;
            const std::size_t bottom = static_cast<std::size_t>(height - 1 - y) * rowSize;
            for (std::size_t x = 0; x < rowSize; ++x)
                std::swap(pixels[top + x], pixels[bottom + x]);
        }

        image.width = width;
        image.height = height;
        image.rgb = std::move(pixels);
        return true;
    }
}

namespace Engine::Graphics
{
    bool Renderer::loadOpenGLFunctions()
    {
#define LOAD(function, name) if (!loadProc(reinterpret_cast<void**>(&function), name)) return false
        LOAD(glGenVertexArraysPtr, "glGenVertexArrays");
        LOAD(glBindVertexArrayPtr, "glBindVertexArray");
        LOAD(glDeleteVertexArraysPtr, "glDeleteVertexArrays");
        LOAD(glGenBuffersPtr, "glGenBuffers");
        LOAD(glBindBufferPtr, "glBindBuffer");
        LOAD(glBufferDataPtr, "glBufferData");
        LOAD(glDeleteBuffersPtr, "glDeleteBuffers");
        LOAD(glCreateShaderPtr, "glCreateShader");
        LOAD(glShaderSourcePtr, "glShaderSource");
        LOAD(glCompileShaderPtr, "glCompileShader");
        LOAD(glGetShaderivPtr, "glGetShaderiv");
        LOAD(glGetShaderInfoLogPtr, "glGetShaderInfoLog");
        LOAD(glDeleteShaderPtr, "glDeleteShader");
        LOAD(glCreateProgramPtr, "glCreateProgram");
        LOAD(glAttachShaderPtr, "glAttachShader");
        LOAD(glLinkProgramPtr, "glLinkProgram");
        LOAD(glGetProgramivPtr, "glGetProgramiv");
        LOAD(glGetProgramInfoLogPtr, "glGetProgramInfoLog");
        LOAD(glUseProgramPtr, "glUseProgram");
        LOAD(glDeleteProgramPtr, "glDeleteProgram");
        LOAD(glGetUniformLocationPtr, "glGetUniformLocation");
        LOAD(glUniformMatrix4fvPtr, "glUniformMatrix4fv");
        LOAD(glUniform3fPtr, "glUniform3f");
        LOAD(glUniform1iPtr, "glUniform1i");
        LOAD(glUniform1fPtr, "glUniform1f");
        LOAD(glEnableVertexAttribArrayPtr, "glEnableVertexAttribArray");
        LOAD(glVertexAttribPointerPtr, "glVertexAttribPointer");
#undef LOAD
        return true;
    }

    bool Renderer::createShaderProgram()
    {
        const std::string vertexSource = readText("assets/shaders/basic.vert");
        const std::string fragmentSource = readText("assets/shaders/basic.frag");
        if (vertexSource.empty() || fragmentSource.empty())
            return false;

        const auto compile = [](unsigned int type, const std::string& source) -> unsigned int
        {
            const unsigned int shader = glCreateShaderPtr(type);
            const char* sourcePointer = source.c_str();
            glShaderSourcePtr(shader, 1, &sourcePointer, nullptr);
            glCompileShaderPtr(shader);

            int compiled = 0;
            glGetShaderivPtr(shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                char log[2048]{};
                int length = 0;
                glGetShaderInfoLogPtr(shader, sizeof(log), &length, log);
                OutputDebugStringA(log);
                glDeleteShaderPtr(shader);
                return 0;
            }
            return shader;
        };

        const unsigned int vertexShader = compile(GL_VERTEX_SHADER, vertexSource);
        const unsigned int fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource);
        if (!vertexShader || !fragmentShader)
        {
            if (vertexShader)
                glDeleteShaderPtr(vertexShader);
            if (fragmentShader)
                glDeleteShaderPtr(fragmentShader);
            return false;
        }

        m_program = glCreateProgramPtr();
        glAttachShaderPtr(m_program, vertexShader);
        glAttachShaderPtr(m_program, fragmentShader);
        glLinkProgramPtr(m_program);
        glDeleteShaderPtr(vertexShader);
        glDeleteShaderPtr(fragmentShader);

        int linked = 0;
        glGetProgramivPtr(m_program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char log[2048]{};
            int length = 0;
            glGetProgramInfoLogPtr(m_program, sizeof(log), &length, log);
            OutputDebugStringA(log);
            return false;
        }

        m_mvpLocation = glGetUniformLocationPtr(m_program, "uMVP");
        m_modelLocation = glGetUniformLocationPtr(m_program, "uModel");
        m_textureLocation = glGetUniformLocationPtr(m_program, "uDiffuseTexture");
        m_albedoLocation = glGetUniformLocationPtr(m_program, "uAlbedo");
        m_lightDirectionLocation = glGetUniformLocationPtr(m_program, "uLightDirection");
        m_lightColorLocation = glGetUniformLocationPtr(m_program, "uLightColor");
        m_ambientColorLocation = glGetUniformLocationPtr(m_program, "uAmbientColor");
        m_cameraPositionLocation = glGetUniformLocationPtr(m_program, "uCameraPosition");
        m_pointLightCountLocation = glGetUniformLocationPtr(m_program, "uPointLightCount");
        for (std::size_t index = 0; index < Scene::Scene::maximumPointLights; ++index)
        {
            const std::string suffix = "[" + std::to_string(index) + "]";
            m_pointLightPositionLocations[index] = glGetUniformLocationPtr(
                m_program, ("uPointLightPositions" + suffix).c_str()
            );
            m_pointLightColorLocations[index] = glGetUniformLocationPtr(
                m_program, ("uPointLightColors" + suffix).c_str()
            );
            m_pointLightIntensityLocations[index] = glGetUniformLocationPtr(
                m_program, ("uPointLightIntensities" + suffix).c_str()
            );
            m_pointLightAttenuationLocations[index] = glGetUniformLocationPtr(
                m_program, ("uPointLightAttenuations" + suffix).c_str()
            );
        }
        if (m_mvpLocation < 0 || m_modelLocation < 0 || m_textureLocation < 0 || m_albedoLocation < 0 ||
            m_lightDirectionLocation < 0 || m_lightColorLocation < 0 || m_ambientColorLocation < 0 ||
            m_cameraPositionLocation < 0 || m_pointLightCountLocation < 0)
            return false;
        for (std::size_t index = 0; index < Scene::Scene::maximumPointLights; ++index)
        {
            if (m_pointLightPositionLocations[index] < 0 || m_pointLightColorLocations[index] < 0 ||
                m_pointLightIntensityLocations[index] < 0 || m_pointLightAttenuationLocations[index] < 0)
                return false;
        }
        return true;
    }

    bool Renderer::initialize(HDC deviceContext, const Scene::Scene& scene)
    {
        m_deviceContext = deviceContext;
        if (!loadOpenGLFunctions() || !createShaderProgram())
            return false;

        m_meshes.reserve(scene.meshes().size());
        for (const Scene::Mesh& mesh : scene.meshes())
        {
            GpuMesh gpuMesh;
            glGenVertexArraysPtr(1, &gpuMesh.vertexArray);
            glBindVertexArrayPtr(gpuMesh.vertexArray);
            glGenBuffersPtr(1, &gpuMesh.vertexBuffer);
            glBindBufferPtr(GL_ARRAY_BUFFER, gpuMesh.vertexBuffer);
            glBufferDataPtr(
                GL_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Scene::Vertex)),
                mesh.vertices.data(),
                GL_STATIC_DRAW
            );
            std::vector<std::uint32_t> sequentialIndices;
            const std::vector<std::uint32_t>* indices = &mesh.indices;
            if (indices->empty())
            {
                sequentialIndices.reserve(mesh.vertices.size());
                for (std::size_t index = 0; index < mesh.vertices.size(); ++index)
                    sequentialIndices.push_back(static_cast<std::uint32_t>(index));
                indices = &sequentialIndices;
            }
            glGenBuffersPtr(1, &gpuMesh.indexBuffer);
            glBindBufferPtr(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.indexBuffer);
            glBufferDataPtr(
                GL_ELEMENT_ARRAY_BUFFER,
                static_cast<GLsizeiptr>(indices->size() * sizeof(std::uint32_t)),
                indices->data(),
                GL_STATIC_DRAW
            );
            glVertexAttribPointerPtr(
                0, 3, GL_FLOAT, GL_FALSE, sizeof(Scene::Vertex),
                reinterpret_cast<const void*>(offsetof(Scene::Vertex, position))
            );
            glEnableVertexAttribArrayPtr(0);
            glVertexAttribPointerPtr(
                1, 3, GL_FLOAT, GL_FALSE, sizeof(Scene::Vertex),
                reinterpret_cast<const void*>(offsetof(Scene::Vertex, normal))
            );
            glEnableVertexAttribArrayPtr(1);
            glVertexAttribPointerPtr(
                2, 3, GL_FLOAT, GL_FALSE, sizeof(Scene::Vertex),
                reinterpret_cast<const void*>(offsetof(Scene::Vertex, diffuseColor))
            );
            glEnableVertexAttribArrayPtr(2);
            glVertexAttribPointerPtr(
                3, 2, GL_FLOAT, GL_FALSE, sizeof(Scene::Vertex),
                reinterpret_cast<const void*>(offsetof(Scene::Vertex, textureCoordinate))
            );
            glEnableVertexAttribArrayPtr(3);
            glVertexAttribPointerPtr(
                4, 3, GL_FLOAT, GL_FALSE, sizeof(Scene::Vertex),
                reinterpret_cast<const void*>(offsetof(Scene::Vertex, specularColor))
            );
            glEnableVertexAttribArrayPtr(4);
            glVertexAttribPointerPtr(
                5, 1, GL_FLOAT, GL_FALSE, sizeof(Scene::Vertex),
                reinterpret_cast<const void*>(offsetof(Scene::Vertex, shininess))
            );
            glEnableVertexAttribArrayPtr(5);

            const auto createSection = [&gpuMesh](const Scene::MeshSection& section)
            {
                ImageData image;
                if (!section.diffuseTexturePath.empty() && !loadPpm(section.diffuseTexturePath, image))
                {
                    OutputDebugStringA(("Could not load PPM texture: " + section.diffuseTexturePath + "\n").c_str());
                }
                GpuSection gpuSection;
                gpuSection.firstIndex = static_cast<int>(section.firstIndex);
                gpuSection.indexCount = static_cast<int>(section.indexCount);
                glGenTextures(1, &gpuSection.texture);
                glBindTexture(GL_TEXTURE_2D, gpuSection.texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0,
                    GL_RGB, GL_UNSIGNED_BYTE, image.rgb.data()
                );
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                gpuMesh.sections.push_back(gpuSection);
            };

            if (mesh.sections.empty())
                createSection({0, indices->size(), {}});
            else
                for (const Scene::MeshSection& section : mesh.sections)
                    createSection(section);
            m_meshes.push_back(gpuMesh);
        }

        m_initialized = true;
        return true;
    }

    void Renderer::render(const Scene::Scene& scene, const Camera& camera, int width, int height)
    {
        if (!m_initialized || width <= 0 || height <= 0)
            return;

        glViewport(0, 0, width, height);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Math::Mat4 view = camera.viewMatrix();
        const Math::Mat4 projection = Math::perspective(
            70.0f,
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            100.0f
        );

        glUseProgramPtr(m_program);
        glUniform1iPtr(m_textureLocation, 0);
        const Scene::DirectionalLight& light = scene.directionalLight();
        glUniform3fPtr(m_lightDirectionLocation, light.direction.x, light.direction.y, light.direction.z);
        glUniform3fPtr(m_lightColorLocation, light.color.x, light.color.y, light.color.z);
        glUniform3fPtr(m_ambientColorLocation, light.ambientColor.x, light.ambientColor.y, light.ambientColor.z);
        const Math::Vec3& cameraPosition = camera.position();
        glUniform3fPtr(m_cameraPositionLocation, cameraPosition.x, cameraPosition.y, cameraPosition.z);
        const std::size_t pointLightCount = std::min(
            scene.pointLights().size(), Scene::Scene::maximumPointLights
        );
        glUniform1iPtr(m_pointLightCountLocation, static_cast<int>(pointLightCount));
        for (std::size_t index = 0; index < pointLightCount; ++index)
        {
            const Scene::PointLight& pointLight = scene.pointLights()[index];
            glUniform3fPtr(
                m_pointLightPositionLocations[index],
                pointLight.position.x, pointLight.position.y, pointLight.position.z
            );
            glUniform3fPtr(
                m_pointLightColorLocations[index],
                pointLight.color.x, pointLight.color.y, pointLight.color.z
            );
            glUniform1fPtr(m_pointLightIntensityLocations[index], pointLight.intensity);
            glUniform3fPtr(
                m_pointLightAttenuationLocations[index],
                pointLight.constantAttenuation,
                pointLight.linearAttenuation,
                pointLight.quadraticAttenuation
            );
        }
        for (const Scene::MeshInstance& object : scene.objects())
        {
            if (object.meshIndex >= m_meshes.size() || object.materialIndex >= scene.materials().size())
                continue;

            const Math::Mat4 model = Math::composeTransform(
                object.transform.position,
                object.transform.rotationDegrees,
                object.transform.scale
            );
            const Math::Mat4 mvp = Math::multiply(projection, Math::multiply(view, model));
            const GpuMesh& mesh = m_meshes[object.meshIndex];
            const Scene::Material& material = scene.materials()[object.materialIndex];
            glUniformMatrix4fvPtr(m_mvpLocation, 1, GL_FALSE, mvp.m);
            glUniformMatrix4fvPtr(m_modelLocation, 1, GL_FALSE, model.m);
            glUniform3fPtr(m_albedoLocation, material.baseColor.x, material.baseColor.y, material.baseColor.z);
            glBindVertexArrayPtr(mesh.vertexArray);
            for (const GpuSection& section : mesh.sections)
            {
                glBindTexture(GL_TEXTURE_2D, section.texture);
                const std::uintptr_t indexOffset =
                    static_cast<std::uintptr_t>(section.firstIndex) * sizeof(std::uint32_t);
                glDrawElements(
                    GL_TRIANGLES,
                    section.indexCount,
                    GL_UNSIGNED_INT,
                    reinterpret_cast<const void*>(indexOffset)
                );
            }
        }

        SwapBuffers(m_deviceContext);
    }

    void Renderer::destroyMeshes()
    {
        if (glDeleteBuffersPtr)
        {
            for (const GpuMesh& mesh : m_meshes)
            {
                if (mesh.vertexBuffer)
                    glDeleteBuffersPtr(1, &mesh.vertexBuffer);
                if (mesh.indexBuffer)
                    glDeleteBuffersPtr(1, &mesh.indexBuffer);
            }
        }
        if (glDeleteVertexArraysPtr)
        {
            for (const GpuMesh& mesh : m_meshes)
            {
                if (mesh.vertexArray)
                    glDeleteVertexArraysPtr(1, &mesh.vertexArray);
            }
        }
        for (const GpuMesh& mesh : m_meshes)
        {
            for (const GpuSection& section : mesh.sections)
            {
                if (section.texture)
                    glDeleteTextures(1, &section.texture);
            }
        }
        m_meshes.clear();
    }

    void Renderer::shutdown()
    {
        destroyMeshes();
        if (m_program && glDeleteProgramPtr)
            glDeleteProgramPtr(m_program);
        m_program = 0;
        m_mvpLocation = -1;
        m_modelLocation = -1;
        m_textureLocation = -1;
        m_albedoLocation = -1;
        m_lightDirectionLocation = -1;
        m_lightColorLocation = -1;
        m_ambientColorLocation = -1;
        m_cameraPositionLocation = -1;
        m_pointLightCountLocation = -1;
        m_pointLightPositionLocations.fill(-1);
        m_pointLightColorLocations.fill(-1);
        m_pointLightIntensityLocations.fill(-1);
        m_pointLightAttenuationLocations.fill(-1);
        m_initialized = false;
        m_deviceContext = nullptr;
    }
}
