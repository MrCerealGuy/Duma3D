#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr std::uint32_t sampleRate = 44100;
    constexpr double pi = 3.14159265358979323846;

    void writeU16(std::ofstream& file, std::uint16_t value)
    {
        file.put(static_cast<char>(value & 0xff));
        file.put(static_cast<char>((value >> 8) & 0xff));
    }

    void writeU32(std::ofstream& file, std::uint32_t value)
    {
        writeU16(file, static_cast<std::uint16_t>(value & 0xffff));
        writeU16(file, static_cast<std::uint16_t>(value >> 16));
    }

    std::uint32_t hash(std::uint32_t value)
    {
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        return value ^ (value >> 16);
    }

    double noise(std::uint32_t index)
    {
        return static_cast<double>(hash(index) & 0xffffu) / 32767.5 - 1.0;
    }

    void writeWave(const std::filesystem::path& path, float duration,
        const std::function<double(double, std::uint32_t)>& synth)
    {
        const std::uint32_t frameCount = static_cast<std::uint32_t>(duration * sampleRate);
        const std::uint32_t dataSize = frameCount * 2;
        std::ofstream file(path, std::ios::binary);
        file.write("RIFF", 4);
        writeU32(file, 36 + dataSize);
        file.write("WAVEfmt ", 8);
        writeU32(file, 16);
        writeU16(file, 1);
        writeU16(file, 1);
        writeU32(file, sampleRate);
        writeU32(file, sampleRate * 2);
        writeU16(file, 2);
        writeU16(file, 16);
        file.write("data", 4);
        writeU32(file, dataSize);
        for (std::uint32_t frame = 0; frame < frameCount; ++frame)
        {
            const double time = static_cast<double>(frame) / sampleRate;
            const double amplitude = std::clamp(synth(time, frame), -0.96, 0.96);
            const auto sample = static_cast<std::int16_t>(amplitude * 32767.0);
            writeU16(file, static_cast<std::uint16_t>(sample));
        }
    }

    double envelope(double time, double attack, double release)
    {
        return std::min(1.0, time / attack) * std::exp(-time / release);
    }

    double door(double time, std::uint32_t frame, bool closing)
    {
        constexpr double duration = 2.8;
        const double progress = time / duration;
        const double motor = std::sin(2.0 * pi * ((closing ? 48.0 : 39.0) * time + 18.0 * time * time));
        const double gear = std::sin(2.0 * pi * (93.0 * time + 2.0 * std::sin(time * 4.0)));
        const double rattle = noise(frame * 17u + (closing ? 91u : 37u)) *
            std::abs(std::sin(time * (closing ? 35.0 : 29.0))) * 0.12;
        const double startClunk = time < 0.22 ? std::sin(2.0 * pi * 130.0 * time) * envelope(time, 0.001, 0.035) : 0.0;
        const double endTime = time - (closing ? 2.5 : 2.63);
        const double endClank = endTime > 0.0
            ? (std::sin(2.0 * pi * 440.0 * endTime) + 0.45 * std::sin(2.0 * pi * 783.0 * endTime)) *
                std::exp(-endTime * 18.0)
            : 0.0;
        const double fade = std::sin(pi * std::clamp(progress, 0.0, 1.0));
        return 0.22 * motor * (0.72 + 0.28 * fade) + 0.12 * gear + rattle + 0.45 * startClunk + 0.38 * endClank;
    }

    double switchClick(double time, std::uint32_t frame)
    {
        const double clickTime = time - 0.018;
        const double click = clickTime > 0.0
            ? noise(frame * 31u + 13u) * std::exp(-clickTime * 72.0) * 0.46
            : 0.0;
        const double springTime = time - 0.12;
        const double spring = springTime > 0.0
            ? std::sin(2.0 * pi * (265.0 - 170.0 * springTime) * springTime) * std::exp(-springTime * 24.0) * 0.22
            : 0.0;
        return click + spring + 0.1 * std::sin(2.0 * pi * 58.0 * time) * std::exp(-time * 9.0);
    }

    double lampFlicker(double time, std::uint32_t frame)
    {
        const double crackOne = time > 0.012 && time < 0.055
            ? noise(frame * 43u + 201u) * std::exp(-(time - 0.012) * 58.0) * 0.5 : 0.0;
        const double crackTwo = time > 0.12 && time < 0.16
            ? noise(frame * 23u + 811u) * std::exp(-(time - 0.12) * 70.0) * 0.35 : 0.0;
        const double buzz = std::sin(2.0 * pi * 120.0 * time) * std::sin(2.0 * pi * 7.0 * time);
        return crackOne + crackTwo + 0.12 * buzz * std::exp(-time * 3.0);
    }

    double teleporter(double time, std::uint32_t frame)
    {
        const double rise = std::clamp(time / 0.72, 0.0, 1.0);
        const double sweep = std::sin(2.0 * pi * (95.0 * time + 420.0 * time * time));
        const double harmonic = std::sin(2.0 * pi * (190.0 * time + 710.0 * time * time));
        const double shimmer = noise(frame * 19u + 53u) * 0.06 * rise;
        const double burstTime = time - 0.72;
        const double burst = burstTime > 0.0
            ? (std::sin(2.0 * pi * 58.0 * burstTime) + 0.5 * std::sin(2.0 * pi * 116.0 * burstTime)) *
                std::exp(-burstTime * 3.4)
            : 0.0;
        return 0.18 * rise * sweep + 0.06 * harmonic + shimmer + 0.36 * burst;
    }

    double ambience(double time, std::uint32_t)
    {
        const double modulation = 0.76 + 0.12 * std::sin(2.0 * pi * 3.0 * time) +
            0.08 * std::sin(2.0 * pi * 5.0 * time);
        return modulation * (0.08 * std::sin(2.0 * pi * 43.0 * time) +
            0.055 * std::sin(2.0 * pi * 57.0 * time) + 0.028 * std::sin(2.0 * pi * 86.0 * time));
    }
}

int main(int argc, char** argv)
{
    const std::filesystem::path outputDirectory = argc > 1 ? argv[1] : "assets/audio";
    std::filesystem::create_directories(outputDirectory);
    writeWave(outputDirectory / "sector_ambience.wav", 8.0f, ambience);
    writeWave(outputDirectory / "door_open.wav", 2.8f,
        [](double time, std::uint32_t frame) { return door(time, frame, false); });
    writeWave(outputDirectory / "door_close.wav", 2.8f,
        [](double time, std::uint32_t frame) { return door(time, frame, true); });
    writeWave(outputDirectory / "relay_switch.wav", 0.55f, switchClick);
    writeWave(outputDirectory / "lamp_flicker.wav", 0.28f, lampFlicker);
    writeWave(outputDirectory / "teleporter.wav", 1.7f, teleporter);
    writeWave(outputDirectory / "access_denied.wav", 0.42f,
        [](double time, std::uint32_t frame)
        {
            return time < 0.18 ? 0.18 * std::sin(2.0 * pi * 154.0 * time) +
                0.08 * noise(frame * 11u + 7u) * std::exp(-time * 14.0) : 0.0;
        });
    std::cout << "Generated Demo 2 audio cues in " << outputDirectory.string() << '\n';
    return 0;
}
