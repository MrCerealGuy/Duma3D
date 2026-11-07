#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Engine/Assets/AssetPath.hpp"

#include <system_error>
#include <vector>

namespace Engine::Assets
{
    std::filesystem::path resolveAssetPath(const std::filesystem::path& path)
    {
        if (path.is_absolute())
            return path.lexically_normal();

        std::vector<wchar_t> executablePath(32768);
        const DWORD length = GetModuleFileNameW(
            nullptr,
            executablePath.data(),
            static_cast<DWORD>(executablePath.size())
        );
        if (length > 0 && length < executablePath.size())
        {
            const std::filesystem::path executable(
                std::wstring(executablePath.data(), length)
            );
            const std::filesystem::path besideExecutable = (executable.parent_path() / path).lexically_normal();
            std::error_code existsError;
            if (std::filesystem::exists(besideExecutable, existsError))
                return besideExecutable;

            existsError.clear();
            const std::filesystem::path workingDirectory = std::filesystem::current_path(existsError);
            if (!existsError)
            {
                const std::filesystem::path besideWorkingDirectory = (workingDirectory / path).lexically_normal();
                existsError.clear();
                if (std::filesystem::exists(besideWorkingDirectory, existsError))
                    return besideWorkingDirectory;
            }
            return besideExecutable;
        }

        std::error_code error;
        const std::filesystem::path workingDirectory = std::filesystem::current_path(error);
        return error ? path.lexically_normal() : (workingDirectory / path).lexically_normal();
    }
}
