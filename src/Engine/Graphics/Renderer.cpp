#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Assets/AssetPath.hpp"

#include <gl/GL.h>
#include <wincodec.h>

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_TEXTURE0
#define GL_TEXTURE0 0x84C0
#endif
#ifndef GL_TEXTURE1
#define GL_TEXTURE1 0x84C1
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_LINEAR_MIPMAP_LINEAR
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#endif
#ifndef GL_SRGB8
#define GL_SRGB8 0x8C41
#endif
#ifndef GL_NONE
#define GL_NONE 0
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
using PFNGLGENFRAMEBUFFERSPROC = void (APIENTRY *)(int, unsigned int*);
using PFNGLBINDFRAMEBUFFERPROC = void (APIENTRY *)(unsigned int, unsigned int);
using PFNGLFRAMEBUFFERTEXTURE2DPROC = void (APIENTRY *)(unsigned int, unsigned int, unsigned int, unsigned int, int);
using PFNGLCHECKFRAMEBUFFERSTATUSPROC = unsigned int (APIENTRY *)(unsigned int);
using PFNGLDELETEFRAMEBUFFERSPROC = void (APIENTRY *)(int, const unsigned int*);
using PFNGLACTIVETEXTUREPROC = void (APIENTRY *)(unsigned int);
using PFNGLGENERATEMIPMAPPROC = void (APIENTRY *)(unsigned int);

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
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffersPtr;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebufferPtr;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2DPtr;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatusPtr;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffersPtr;
static PFNGLACTIVETEXTUREPROC glActiveTexturePtr;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmapPtr;

namespace
{
    struct FrustumPlane
    {
        Engine::Math::Vec3 normal;
        float distance;
    };

    using Frustum = std::array<FrustumPlane, 6>;

    Frustum extractFrustum(const Engine::Math::Mat4& matrix)
    {
        const auto makePlane = [&matrix](float sign, int row)
        {
            const int row3 = 3;
            const float x = matrix.m[row3] + sign * matrix.m[row];
            const float y = matrix.m[4 + row3] + sign * matrix.m[4 + row];
            const float z = matrix.m[8 + row3] + sign * matrix.m[8 + row];
            const float distance = matrix.m[12 + row3] + sign * matrix.m[12 + row];
            const float length = std::sqrt(x * x + y * y + z * z);
            if (length <= 1.0e-6f)
                return FrustumPlane{{0.0f, 0.0f, 0.0f}, std::numeric_limits<float>::infinity()};
            return FrustumPlane{{x / length, y / length, z / length}, distance / length};
        };

        return {
            makePlane(1.0f, 0), makePlane(-1.0f, 0),
            makePlane(1.0f, 1), makePlane(-1.0f, 1),
            makePlane(1.0f, 2), makePlane(-1.0f, 2)
        };
    }

    bool sphereOutsideFrustum(const Frustum& frustum, Engine::Math::Vec3 center, float radius)
    {
        for (const FrustumPlane& plane : frustum)
        {
            const float distance = Engine::Math::dot(plane.normal, center) + plane.distance;
            if (distance < -radius)
                return true;
        }
        return false;
    }

    Engine::Math::Vec3 transformPoint(const Engine::Math::Mat4& matrix, Engine::Math::Vec3 point)
    {
        return {
            matrix.m[0] * point.x + matrix.m[4] * point.y + matrix.m[8] * point.z + matrix.m[12],
            matrix.m[1] * point.x + matrix.m[5] * point.y + matrix.m[9] * point.z + matrix.m[13],
            matrix.m[2] * point.x + matrix.m[6] * point.y + matrix.m[10] * point.z + matrix.m[14]
        };
    }

    template <typename Interface>
    class ComPtr
    {
    public:
        ~ComPtr()
        {
            if (m_value)
                m_value->Release();
        }

        Interface** put() { return &m_value; }
        Interface* operator->() const { return m_value; }

    private:
        Interface* m_value = nullptr;
    };

    class ComApartment
    {
    public:
        explicit ComApartment(bool shouldUninitialize) : m_shouldUninitialize(shouldUninitialize) {}
        ~ComApartment()
        {
            if (m_shouldUninitialize)
                CoUninitialize();
        }

    private:
        bool m_shouldUninitialize;
    };

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

    bool loadWithWindowsImagingComponent(const std::string& path, ImageData& image)
    {
        const HRESULT initialization = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        const bool uninitialize = SUCCEEDED(initialization);
        if (FAILED(initialization) && initialization != RPC_E_CHANGED_MODE)
            return false;
        ComApartment apartment(uninitialize);

        ComPtr<IWICImagingFactory> factory;
        HRESULT result = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory, reinterpret_cast<void**>(factory.put())
        );
        if (FAILED(result))
            return false;

        ComPtr<IWICBitmapDecoder> decoder;
        result = factory->CreateDecoderFromFilename(
            Engine::Assets::resolveAssetPath(path).c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnLoad, decoder.put()
        );
        if (FAILED(result))
            return false;

        ComPtr<IWICBitmapFrameDecode> frame;
        result = decoder->GetFrame(0, frame.put());
        if (FAILED(result))
            return false;

        ComPtr<IWICFormatConverter> converter;
        result = factory->CreateFormatConverter(converter.put());
        if (FAILED(result) || FAILED(converter->Initialize(
                frame.operator->(), GUID_WICPixelFormat24bppRGB, WICBitmapDitherTypeNone,
                nullptr, 0.0, WICBitmapPaletteTypeCustom)))
            return false;

        UINT width = 0;
        UINT height = 0;
        result = converter->GetSize(&width, &height);
        if (FAILED(result) || width == 0 || height == 0 ||
            width > static_cast<UINT>(std::numeric_limits<int>::max()) ||
            height > static_cast<UINT>(std::numeric_limits<int>::max()))
            return false;

        const std::size_t rowSize = static_cast<std::size_t>(width) * 3;
        if (rowSize > std::numeric_limits<UINT>::max() ||
            static_cast<std::size_t>(height) > std::numeric_limits<std::size_t>::max() / rowSize)
            return false;
        const std::size_t pixelDataSize = rowSize * static_cast<std::size_t>(height);
        if (pixelDataSize > std::numeric_limits<UINT>::max())
            return false;

        std::vector<unsigned char> pixels(pixelDataSize);
        result = converter->CopyPixels(
            nullptr, static_cast<UINT>(rowSize), static_cast<UINT>(pixelDataSize), pixels.data()
        );
        if (FAILED(result))
            return false;

        for (UINT y = 0; y < height / 2; ++y)
        {
            const std::size_t top = static_cast<std::size_t>(y) * rowSize;
            const std::size_t bottom = static_cast<std::size_t>(height - 1 - y) * rowSize;
            for (std::size_t x = 0; x < rowSize; ++x)
                std::swap(pixels[top + x], pixels[bottom + x]);
        }

        image.width = static_cast<int>(width);
        image.height = static_cast<int>(height);
        image.rgb = std::move(pixels);
        return true;
    }

    bool loadImage(const std::string& path, ImageData& image)
    {
        return loadPpm(path, image) || loadWithWindowsImagingComponent(path, image);
    }

    std::wstring textureCacheKey(const std::string& path)
    {
        if (path.empty())
            return L"<default-white>";

        std::wstring key = Engine::Assets::resolveAssetPath(path).lexically_normal().wstring();
        std::transform(key.begin(), key.end(), key.begin(), [](wchar_t character)
        {
            return static_cast<wchar_t>(std::towlower(character));
        });
        return key;
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
        LOAD(glGenFramebuffersPtr, "glGenFramebuffers");
        LOAD(glBindFramebufferPtr, "glBindFramebuffer");
        LOAD(glFramebufferTexture2DPtr, "glFramebufferTexture2D");
        LOAD(glCheckFramebufferStatusPtr, "glCheckFramebufferStatus");
        LOAD(glDeleteFramebuffersPtr, "glDeleteFramebuffers");
        LOAD(glActiveTexturePtr, "glActiveTexture");
        LOAD(glGenerateMipmapPtr, "glGenerateMipmap");
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
        m_lightSpaceLocation = glGetUniformLocationPtr(m_program, "uLightSpaceMatrix");
        m_shadowTextureLocation = glGetUniformLocationPtr(m_program, "uShadowMap");
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
        if (m_lightSpaceLocation < 0 || m_shadowTextureLocation < 0)
            return false;
        for (std::size_t index = 0; index < Scene::Scene::maximumPointLights; ++index)
        {
            if (m_pointLightPositionLocations[index] < 0 || m_pointLightColorLocations[index] < 0 ||
                m_pointLightIntensityLocations[index] < 0 || m_pointLightAttenuationLocations[index] < 0)
                return false;
        }
        return true;
    }

    bool Renderer::createShadowShaderProgram()
    {
        const std::string vertexSource = readText("assets/shaders/shadow.vert");
        const std::string fragmentSource = readText("assets/shaders/shadow.frag");
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

        m_shadowProgram = glCreateProgramPtr();
        glAttachShaderPtr(m_shadowProgram, vertexShader);
        glAttachShaderPtr(m_shadowProgram, fragmentShader);
        glLinkProgramPtr(m_shadowProgram);
        glDeleteShaderPtr(vertexShader);
        glDeleteShaderPtr(fragmentShader);

        int linked = 0;
        glGetProgramivPtr(m_shadowProgram, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            char log[2048]{};
            int length = 0;
            glGetProgramInfoLogPtr(m_shadowProgram, sizeof(log), &length, log);
            OutputDebugStringA(log);
            return false;
        }

        m_shadowMvpLocation = glGetUniformLocationPtr(m_shadowProgram, "uLightMVP");
        return m_shadowMvpLocation >= 0;
    }

    bool Renderer::createShadowMap()
    {
        constexpr int shadowMapSize = 1024;
        glGenTextures(1, &m_shadowTexture);
        glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowMapSize, shadowMapSize, 0,
            GL_DEPTH_COMPONENT, GL_FLOAT, nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glGenFramebuffersPtr(1, &m_shadowFramebuffer);
        glBindFramebufferPtr(GL_FRAMEBUFFER, m_shadowFramebuffer);
        glFramebufferTexture2DPtr(
            GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowTexture, 0
        );
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        const bool complete = glCheckFramebufferStatusPtr(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebufferPtr(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        return complete;
    }

    bool Renderer::initialize(HDC deviceContext, const Scene::Scene& scene)
    {
        m_deviceContext = deviceContext;
        if (!loadOpenGLFunctions() || !createShaderProgram() ||
            !createShadowShaderProgram() || !createShadowMap())
            return false;

        m_meshes.reserve(scene.meshes().size());
        std::unordered_map<std::wstring, unsigned int> textureCache;
        for (const Scene::Mesh& mesh : scene.meshes())
        {
            GpuMesh gpuMesh;
            if (!mesh.vertices.empty())
            {
                Math::Vec3 minimum = mesh.vertices.front().position;
                Math::Vec3 maximum = minimum;
                for (const Scene::Vertex& vertex : mesh.vertices)
                {
                    minimum.x = std::min(minimum.x, vertex.position.x);
                    minimum.y = std::min(minimum.y, vertex.position.y);
                    minimum.z = std::min(minimum.z, vertex.position.z);
                    maximum.x = std::max(maximum.x, vertex.position.x);
                    maximum.y = std::max(maximum.y, vertex.position.y);
                    maximum.z = std::max(maximum.z, vertex.position.z);
                }
                gpuMesh.boundsCenter = {
                    (minimum.x + maximum.x) * 0.5f,
                    (minimum.y + maximum.y) * 0.5f,
                    (minimum.z + maximum.z) * 0.5f
                };
                for (const Scene::Vertex& vertex : mesh.vertices)
                {
                    const float x = vertex.position.x - gpuMesh.boundsCenter.x;
                    const float y = vertex.position.y - gpuMesh.boundsCenter.y;
                    const float z = vertex.position.z - gpuMesh.boundsCenter.z;
                    gpuMesh.boundsRadius = std::max(
                        gpuMesh.boundsRadius, std::sqrt(x * x + y * y + z * z)
                    );
                }
                gpuMesh.hasBounds = true;
            }
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
            gpuMesh.indexCount = static_cast<int>(indices->size());
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

            const auto createSection = [this, &gpuMesh, &textureCache](const Scene::MeshSection& section)
            {
                const std::wstring key = textureCacheKey(section.diffuseTexturePath);
                const auto cachedTexture = textureCache.find(key);
                GpuSection gpuSection;
                gpuSection.firstIndex = static_cast<int>(section.firstIndex);
                gpuSection.indexCount = static_cast<int>(section.indexCount);
                if (cachedTexture != textureCache.end())
                {
                    gpuSection.texture = cachedTexture->second;
                    gpuMesh.sections.push_back(gpuSection);
                    return;
                }

                ImageData image;
                if (!section.diffuseTexturePath.empty() && !loadImage(section.diffuseTexturePath, image))
                {
                    OutputDebugStringA(("Could not load texture: " + section.diffuseTexturePath + "\n").c_str());
                }
                glGenTextures(1, &gpuSection.texture);
                m_textures.push_back(gpuSection.texture);
                glBindTexture(GL_TEXTURE_2D, gpuSection.texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage2D(
                    GL_TEXTURE_2D, 0, GL_SRGB8, image.width, image.height, 0,
                    GL_RGB, GL_UNSIGNED_BYTE, image.rgb.data()
                );
                glGenerateMipmapPtr(GL_TEXTURE_2D);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                textureCache.emplace(key, gpuSection.texture);
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

    void Renderer::renderShadowMap(const Scene::Scene& scene, const Math::Mat4& lightSpaceMatrix)
    {
        constexpr int shadowMapSize = 1024;
        glViewport(0, 0, shadowMapSize, shadowMapSize);
        glBindFramebufferPtr(GL_FRAMEBUFFER, m_shadowFramebuffer);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgramPtr(m_shadowProgram);
        const Frustum lightFrustum = extractFrustum(lightSpaceMatrix);
        for (const Scene::MeshInstance& object : scene.objects())
        {
            if (object.meshIndex >= m_meshes.size())
                continue;

            const Math::Mat4 model = Math::composeTransform(
                object.transform.position,
                object.transform.rotationDegrees,
                object.transform.scale
            );
            const GpuMesh& mesh = m_meshes[object.meshIndex];
            if (mesh.hasBounds)
            {
                const Math::Vec3 worldCenter = transformPoint(model, mesh.boundsCenter);
                const float maximumScale = std::max({
                    std::abs(object.transform.scale.x),
                    std::abs(object.transform.scale.y),
                    std::abs(object.transform.scale.z)
                });
                if (sphereOutsideFrustum(lightFrustum, worldCenter, mesh.boundsRadius * maximumScale))
                    continue;
            }

            const Math::Mat4 lightMvp = Math::multiply(lightSpaceMatrix, model);
            glUniformMatrix4fvPtr(m_shadowMvpLocation, 1, GL_FALSE, lightMvp.m);
            glBindVertexArrayPtr(mesh.vertexArray);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
        }
        glBindFramebufferPtr(GL_FRAMEBUFFER, 0);
    }

    void Renderer::render(const Scene::Scene& scene, const Camera& camera, int width, int height)
    {
        if (!m_initialized || width <= 0 || height <= 0)
            return;

        glEnable(GL_DEPTH_TEST);

        const Scene::DirectionalLight& light = scene.directionalLight();
        const Math::Vec3 lightDirection = Math::normalize(light.direction);
        Math::Vec3 lightTarget{0.0f, 0.0f, 0.0f};
        float sceneBoundsRadius = 0.0f;
        bool hasSceneBounds = false;
        for (const Scene::MeshInstance& object : scene.objects())
        {
            if (object.meshIndex >= m_meshes.size())
                continue;
            const GpuMesh& mesh = m_meshes[object.meshIndex];
            if (!mesh.hasBounds)
                continue;

            const Math::Mat4 model = Math::composeTransform(
                object.transform.position,
                object.transform.rotationDegrees,
                object.transform.scale
            );
            const Math::Vec3 center = transformPoint(model, mesh.boundsCenter);
            const float radius = mesh.boundsRadius * std::max({
                std::abs(object.transform.scale.x),
                std::abs(object.transform.scale.y),
                std::abs(object.transform.scale.z)
            });
            if (!hasSceneBounds)
            {
                lightTarget = center;
                sceneBoundsRadius = radius;
                hasSceneBounds = true;
                continue;
            }

            const float offsetX = center.x - lightTarget.x;
            const float offsetY = center.y - lightTarget.y;
            const float offsetZ = center.z - lightTarget.z;
            const float distance = std::sqrt(offsetX * offsetX + offsetY * offsetY + offsetZ * offsetZ);
            if (distance + radius <= sceneBoundsRadius)
                continue;
            if (distance + sceneBoundsRadius <= radius)
            {
                lightTarget = center;
                sceneBoundsRadius = radius;
                continue;
            }

            const float newRadius = (sceneBoundsRadius + distance + radius) * 0.5f;
            if (distance > 1.0e-6f)
            {
                const float centerShift = (newRadius - sceneBoundsRadius) / distance;
                lightTarget.x += offsetX * centerShift;
                lightTarget.y += offsetY * centerShift;
                lightTarget.z += offsetZ * centerShift;
            }
            sceneBoundsRadius = newRadius;
        }
        const float lightRadius = hasSceneBounds ? sceneBoundsRadius + 1.0f : 5.0f;

        const Math::Vec3 lightPosition{
            lightTarget.x - lightDirection.x * (lightRadius + 1.0f),
            lightTarget.y - lightDirection.y * (lightRadius + 1.0f),
            lightTarget.z - lightDirection.z * (lightRadius + 1.0f)
        };
        const Math::Vec3 lightUp = std::abs(Math::dot(lightDirection, {0.0f, 1.0f, 0.0f})) > 0.98f
            ? Math::Vec3{0.0f, 0.0f, 1.0f}
            : Math::Vec3{0.0f, 1.0f, 0.0f};
        const Math::Mat4 lightView = Math::lookAt(lightPosition, lightTarget, lightUp);
        const Math::Mat4 lightProjection = Math::orthographic(
            -lightRadius, lightRadius, -lightRadius, lightRadius, 0.1f, 2.0f * lightRadius + 2.0f
        );
        const Math::Mat4 lightSpaceMatrix = Math::multiply(lightProjection, lightView);
        renderShadowMap(scene, lightSpaceMatrix);

        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.10f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const Math::Mat4 view = camera.viewMatrix();
        const Math::Mat4 projection = Math::perspective(
            70.0f,
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            100.0f
        );
        const Math::Mat4 viewProjection = Math::multiply(projection, view);
        const Frustum cameraFrustum = extractFrustum(viewProjection);

        glUseProgramPtr(m_program);
        glUniform1iPtr(m_textureLocation, 0);
        glUniform1iPtr(m_shadowTextureLocation, 1);
        glUniformMatrix4fvPtr(m_lightSpaceLocation, 1, GL_FALSE, lightSpaceMatrix.m);
        glActiveTexturePtr(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
        glActiveTexturePtr(GL_TEXTURE0);
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
            if (mesh.hasBounds)
            {
                const Math::Vec3 worldCenter = transformPoint(model, mesh.boundsCenter);
                const float maximumScale = std::max({
                    std::abs(object.transform.scale.x),
                    std::abs(object.transform.scale.y),
                    std::abs(object.transform.scale.z)
                });
                if (sphereOutsideFrustum(cameraFrustum, worldCenter, mesh.boundsRadius * maximumScale))
                    continue;
            }
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
        for (const unsigned int texture : m_textures)
            if (texture)
                glDeleteTextures(1, &texture);
        m_textures.clear();
        m_meshes.clear();
    }

    void Renderer::shutdown()
    {
        destroyMeshes();
        if (m_program && glDeleteProgramPtr)
            glDeleteProgramPtr(m_program);
        if (m_shadowProgram && glDeleteProgramPtr)
            glDeleteProgramPtr(m_shadowProgram);
        if (m_shadowFramebuffer && glDeleteFramebuffersPtr)
            glDeleteFramebuffersPtr(1, &m_shadowFramebuffer);
        if (m_shadowTexture)
            glDeleteTextures(1, &m_shadowTexture);
        m_program = 0;
        m_shadowProgram = 0;
        m_shadowFramebuffer = 0;
        m_shadowTexture = 0;
        m_mvpLocation = -1;
        m_modelLocation = -1;
        m_textureLocation = -1;
        m_albedoLocation = -1;
        m_lightDirectionLocation = -1;
        m_lightColorLocation = -1;
        m_ambientColorLocation = -1;
        m_cameraPositionLocation = -1;
        m_lightSpaceLocation = -1;
        m_shadowTextureLocation = -1;
        m_shadowMvpLocation = -1;
        m_pointLightCountLocation = -1;
        m_pointLightPositionLocations.fill(-1);
        m_pointLightColorLocations.fill(-1);
        m_pointLightIntensityLocations.fill(-1);
        m_pointLightAttenuationLocations.fill(-1);
        m_initialized = false;
        m_deviceContext = nullptr;
    }
}
