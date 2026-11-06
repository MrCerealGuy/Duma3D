#pragma once

#include <filesystem>

namespace Engine::Audio
{
    enum class Priority : int
    {
        Ambient = 0,
        Flicker = 1,
        Door = 2,
        Switch = 3,
        Teleporter = 4,
        Alert = 5
    };

    // Small Windows WAV player for ambient loops and prioritized one-shot cues.
    class AudioPlayer
    {
    public:
        AudioPlayer() = default;
        ~AudioPlayer();
        AudioPlayer(const AudioPlayer&) = delete;
        AudioPlayer& operator=(const AudioPlayer&) = delete;

        bool loop(const std::filesystem::path& relativeAssetPath);
        bool play(const std::filesystem::path& relativeAssetPath, Priority priority);
        void update(float deltaTime);
        void stop();

    private:
        bool startLoop();

        std::filesystem::path m_loopPath;
        float m_remainingSeconds = 0.0f;
        Priority m_priority = Priority::Ambient;
    };
}
