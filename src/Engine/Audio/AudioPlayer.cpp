#include "Engine/Audio/AudioPlayer.hpp"

#include "Engine/Assets/AssetPath.hpp"

#include <windows.h>
#include <mmsystem.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>

namespace Engine::Audio
{
    namespace
    {
        float waveDuration(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            std::array<unsigned char, 44> header{};
            if (!file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size())) ||
                std::string(reinterpret_cast<const char*>(header.data()), 4) != "RIFF" ||
                std::string(reinterpret_cast<const char*>(header.data() + 8), 4) != "WAVE" ||
                std::string(reinterpret_cast<const char*>(header.data() + 36), 4) != "data")
                return 0.0f;
            const std::uint32_t byteRate = static_cast<std::uint32_t>(header[28]) |
                (static_cast<std::uint32_t>(header[29]) << 8) |
                (static_cast<std::uint32_t>(header[30]) << 16) |
                (static_cast<std::uint32_t>(header[31]) << 24);
            const std::uint32_t dataSize = static_cast<std::uint32_t>(header[40]) |
                (static_cast<std::uint32_t>(header[41]) << 8) |
                (static_cast<std::uint32_t>(header[42]) << 16) |
                (static_cast<std::uint32_t>(header[43]) << 24);
            return byteRate == 0 ? 0.0f : static_cast<float>(dataSize) / static_cast<float>(byteRate);
        }
    }

    AudioPlayer::~AudioPlayer()
    {
        stop();
    }

    bool AudioPlayer::loop(const std::filesystem::path& relativeAssetPath)
    {
        m_loopPath = Assets::resolveAssetPath(relativeAssetPath);
        m_remainingSeconds = 0.0f;
        m_priority = Priority::Ambient;
        return startLoop();
    }

    bool AudioPlayer::startLoop()
    {
        if (m_loopPath.empty())
            return false;
        return PlaySoundW(m_loopPath.c_str(), nullptr,
            SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT) != FALSE;
    }

    bool AudioPlayer::play(const std::filesystem::path& relativeAssetPath, Priority priority)
    {
        const std::filesystem::path resolvedPath = Assets::resolveAssetPath(relativeAssetPath);
        if (m_remainingSeconds > 0.0f && static_cast<int>(priority) < static_cast<int>(m_priority))
            return false;
        const float duration = waveDuration(resolvedPath);
        if (duration <= 0.0f)
            return false;
        if (!PlaySoundW(resolvedPath.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT))
            return false;
        m_priority = priority;
        m_remainingSeconds = duration;
        return true;
    }

    void AudioPlayer::update(float deltaTime)
    {
        if (m_remainingSeconds <= 0.0f || !std::isfinite(deltaTime) || deltaTime <= 0.0f)
            return;
        m_remainingSeconds -= deltaTime;
        if (m_remainingSeconds <= 0.0f)
        {
            m_remainingSeconds = 0.0f;
            m_priority = Priority::Ambient;
            startLoop();
        }
    }

    void AudioPlayer::stop()
    {
        PlaySoundW(nullptr, nullptr, 0);
        m_loopPath.clear();
        m_remainingSeconds = 0.0f;
        m_priority = Priority::Ambient;
    }
}
