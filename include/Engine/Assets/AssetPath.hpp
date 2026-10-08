#pragma once

#include <filesystem>

namespace Engine::Assets
{
    std::filesystem::path resolveAssetPath(const std::filesystem::path& path);
}
