#pragma once

#include "Engine/Graphics/Scene/Scene.hpp"

#include <filesystem>
#include <string>

namespace Engine::Graphics::Scene
{
    struct ModelBounds
    {
        Math::Vec3 minimum{};
        Math::Vec3 maximum{};
        Math::Vec3 suggestedStart{};
    };

    class GltfLoader
    {
    public:
        // Loads a binary glTF 2.0 scene, including embedded base-color images.
        static bool loadGlb(const std::filesystem::path& path, Scene& scene,
            ModelBounds& bounds, std::string& error);
    };
}
