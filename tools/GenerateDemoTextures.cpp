#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr int textureSize = 256;
    constexpr float pi = 3.14159265358979323846f;

    struct Color
    {
        float r;
        float g;
        float b;
    };

    enum class MaterialType
    {
        Grass, Soil, Rock, Cobblestone, Plaster, Terracotta, Slate, Shingle,
        WoodFloor, DarkWood, Bark, Foliage, AutumnLeaves
    };

    std::uint32_t hash(std::uint32_t x, std::uint32_t y, std::uint32_t seed)
    {
        std::uint32_t value = x * 0x8da6b343u ^ y * 0xd8163841u ^ seed * 0xcb1ab31fu;
        value ^= value >> 16;
        value *= 0x7feb352du;
        value ^= value >> 15;
        value *= 0x846ca68bu;
        value ^= value >> 16;
        return value;
    }

    float random01(int x, int y, int cells, std::uint32_t seed)
    {
        const auto wrap = [cells](int value)
        {
            const int remainder = value % cells;
            return static_cast<std::uint32_t>(remainder < 0 ? remainder + cells : remainder);
        };
        return static_cast<float>(hash(wrap(x), wrap(y), seed) & 0x00ffffffu) / 16777215.0f;
    }

    float smooth(float value)
    {
        return value * value * (3.0f - 2.0f * value);
    }

    float valueNoise(float u, float v, int cells, std::uint32_t seed)
    {
        const float x = u * cells;
        const float y = v * cells;
        const int x0 = static_cast<int>(std::floor(x));
        const int y0 = static_cast<int>(std::floor(y));
        const float tx = smooth(x - x0);
        const float ty = smooth(y - y0);
        const float a = random01(x0, y0, cells, seed);
        const float b = random01(x0 + 1, y0, cells, seed);
        const float c = random01(x0, y0 + 1, cells, seed);
        const float d = random01(x0 + 1, y0 + 1, cells, seed);
        const float top = a + (b - a) * tx;
        const float bottom = c + (d - c) * tx;
        return top + (bottom - top) * ty;
    }

    float fractalNoise(float u, float v, std::uint32_t seed)
    {
        constexpr std::array<int, 5> frequencies{3, 7, 15, 31, 63};
        constexpr std::array<float, 5> amplitudes{0.42f, 0.26f, 0.16f, 0.10f, 0.06f};
        float value = 0.0f;
        float weight = 0.0f;
        for (std::size_t octave = 0; octave < frequencies.size(); ++octave)
        {
            value += valueNoise(u, v, frequencies[octave], seed + static_cast<std::uint32_t>(octave) * 97u) *
                amplitudes[octave];
            weight += amplitudes[octave];
        }
        return value / weight;
    }

    float wrappedDistance(float a, float b)
    {
        const float difference = std::abs(a - b);
        return std::min(difference, 1.0f - difference);
    }

    Color scale(Color color, float amount)
    {
        return {color.r * amount, color.g * amount, color.b * amount};
    }

    Color add(Color a, Color b)
    {
        return {a.r + b.r, a.g + b.g, a.b + b.b};
    }

    Color mix(Color a, Color b, float amount)
    {
        return add(scale(a, 1.0f - amount), scale(b, amount));
    }

    Color voronoiStone(float u, float v, int columns, std::uint32_t seed, Color base)
    {
        const float x = u * columns;
        const float y = v * columns;
        const int cellX = static_cast<int>(std::floor(x));
        const int cellY = static_cast<int>(std::floor(y));
        float nearest = 10.0f;
        float second = 10.0f;
        float colorVariation = 0.5f;
        for (int oy = -1; oy <= 1; ++oy)
        {
            for (int ox = -1; ox <= 1; ++ox)
            {
                const int cx = cellX + ox;
                const int cy = cellY + oy;
                const float centerX = cx + 0.22f + random01(cx, cy, columns, seed) * 0.56f;
                const float centerY = cy + 0.22f + random01(cx + 39, cy - 17, columns, seed) * 0.56f;
                const float dx = std::abs(x - centerX);
                const float dy = std::abs(y - centerY);
                const float wrappedX = std::min(dx, columns - dx);
                const float wrappedY = std::min(dy, columns - dy);
                const float distance = std::sqrt(wrappedX * wrappedX + wrappedY * wrappedY);
                if (distance < nearest)
                {
                    second = nearest;
                    nearest = distance;
                    colorVariation = random01(cx - 11, cy + 23, columns, seed + 71u);
                }
                else if (distance < second)
                {
                    second = distance;
                }
            }
        }

        const float mortar = std::clamp((second - nearest - 0.15f) / 0.20f, 0.0f, 1.0f);
        const float roughness = (fractalNoise(u, v, seed + 13u) - 0.5f) * 0.28f;
        const float stoneValue = 0.78f + colorVariation * 0.30f + roughness;
        return mix({64.0f, 59.0f, 52.0f}, scale(base, stoneValue), mortar);
    }

    Color sample(MaterialType type, float u, float v, std::uint32_t seed)
    {
        const float broad = fractalNoise(u, v, seed);
        const float fine = valueNoise(u, v, 109, seed + 41u);
        const float grain = valueNoise(u, v, 127, seed + 157u);
        switch (type)
        {
            case MaterialType::Grass:
            {
                Color color = {69.0f, 91.0f, 49.0f};
                color = add(color, scale({54.0f, 58.0f, 37.0f}, broad - 0.5f));
                color = add(color, scale({28.0f, 30.0f, 20.0f}, fine - 0.5f));
                const float blade = std::sin((u * 114.0f + valueNoise(u, v, 9, seed) * 0.11f) * pi);
                const float bladeMask = std::pow(std::max(0.0f, blade), 36.0f) *
                    (0.25f + valueNoise(u, v, 47, seed + 7u) * 0.75f);
                color = add(color, scale({-14.0f, -6.0f, -8.0f}, bladeMask));
                const float dryPatch = std::max(0.0f, valueNoise(u, v, 17, seed + 83u) - 0.77f) * 3.3f;
                color = add(color, scale({61.0f, 43.0f, 20.0f}, dryPatch));
                return color;
            }
            case MaterialType::Soil:
            {
                Color color = {92.0f, 65.0f, 43.0f};
                color = add(color, scale({54.0f, 39.0f, 28.0f}, broad - 0.5f));
                color = add(color, scale({24.0f, 20.0f, 15.0f}, fine - 0.5f));
                const float grit = std::pow(grain, 14.0f);
                return add(color, scale({37.0f, 30.0f, 20.0f}, grit));
            }
            case MaterialType::Rock:
            {
                Color color = {100.0f, 98.0f, 89.0f};
                color = add(color, scale({72.0f, 68.0f, 58.0f}, broad - 0.5f));
                color = add(color, scale({33.0f, 31.0f, 27.0f}, fine - 0.5f));
                const float vein = std::pow(std::max(0.0f, 0.5f - valueNoise(u, v, 23, seed + 17u)), 2.0f);
                return add(color, scale({38.0f, 34.0f, 27.0f}, vein));
            }
            case MaterialType::Cobblestone:
                return voronoiStone(u, v, 13, seed, {128.0f, 120.0f, 104.0f});
            case MaterialType::Plaster:
            {
                // Base hue is supplied per texture through the seed-specific palette below.
                const std::array<Color, 3> palettes{{
                    {181.0f, 145.0f, 101.0f}, {132.0f, 157.0f, 169.0f}, {143.0f, 158.0f, 128.0f}
                }};
                Color color = palettes[(seed / 100u) % palettes.size()];
                color = add(color, scale({42.0f, 40.0f, 35.0f}, broad - 0.5f));
                color = add(color, scale({24.0f, 22.0f, 18.0f}, fine - 0.5f));
                const float pits = std::pow(std::max(0.0f, 0.23f - valueNoise(u, v, 97, seed + 23u)), 7.0f);
                const float chalk = std::pow(std::max(0.0f, valueNoise(u, v, 73, seed + 37u) - 0.78f), 2.0f);
                return add(color, {-pits * 460.0f + chalk * 39.0f,
                    -pits * 420.0f + chalk * 37.0f, -pits * 360.0f + chalk * 32.0f});
            }
            case MaterialType::Terracotta:
            case MaterialType::Slate:
            case MaterialType::Shingle:
            {
                const bool terracotta = type == MaterialType::Terracotta;
                const bool slate = type == MaterialType::Slate;
                const int rows = 9;
                const float rowY = v * rows;
                const int row = static_cast<int>(std::floor(rowY));
                const float y = rowY - std::floor(rowY);
                const float stagger = (row & 1) ? 0.5f : 0.0f;
                const float x = u * 9.0f + stagger;
                const int column = static_cast<int>(std::floor(x));
                const float localX = x - std::floor(x) + (random01(column, row, 9, seed + 17u) - 0.5f) * 0.035f;
                const float verticalEdge = std::min(localX, 1.0f - localX);
                const float curvedBottom = 0.10f + std::pow(localX - 0.5f, 2.0f) * 0.22f;
                const float topEdge = std::min(y + 0.08f, 1.0f - y + curvedBottom);
                const float tile = std::clamp(std::min(verticalEdge - 0.025f, topEdge - 0.045f) * 20.0f, 0.0f, 1.0f);
                const float noise = (broad - 0.5f) * 0.30f + (fine - 0.5f) * 0.12f;
                if (terracotta)
                {
                    const float ridge = std::exp(-std::pow((y - 0.14f) * 16.0f, 2.0f)) * 12.0f;
                    return mix({54.0f, 42.0f, 35.0f}, {151.0f + ridge + noise * 255.0f,
                        74.0f + ridge * 0.45f + noise * 220.0f, 48.0f + ridge * 0.2f + noise * 170.0f}, tile);
                }
                if (slate)
                {
                    const float grainLine = std::sin((u * 150.0f + broad * 0.3f) * pi) * 2.0f;
                    return mix({31.0f, 38.0f, 43.0f}, {91.0f + noise * 180.0f + grainLine,
                        102.0f + noise * 180.0f + grainLine, 107.0f + noise * 180.0f + grainLine}, tile);
                }
                const float grainLine = std::sin((u * 55.0f + broad * 0.22f) * pi) * 6.0f;
                return mix({43.0f, 34.0f, 27.0f}, {109.0f + noise * 190.0f + grainLine,
                    72.0f + noise * 150.0f + grainLine, 43.0f + noise * 115.0f + grainLine}, tile);
            }
            case MaterialType::WoodFloor:
            case MaterialType::DarkWood:
            {
                const bool floor = type == MaterialType::WoodFloor;
                const int planks = floor ? 7 : 4;
                const float plankY = v * planks;
                const int plankIndex = static_cast<int>(std::floor(plankY));
                const float localY = plankY - std::floor(plankY);
                const float offset = random01(plankIndex, 0, planks, seed) * 0.45f;
                const float seamX = std::abs(std::sin((u * 4.0f + offset) * pi));
                const bool seam = localY < 0.035f || (seamX < 0.045f && random01(plankIndex, 1, planks, seed) > 0.4f);
                const float grain = std::sin((v * 178.0f + valueNoise(u, v, 5, seed + 53u) * 0.10f) * pi);
                const float knotX = wrappedDistance(u, random01(plankIndex, 3, planks, seed));
                const float knotY = wrappedDistance(v, random01(plankIndex, 7, planks, seed + 8u));
                const float knot = std::exp(-(knotX * knotX * 500.0f + knotY * knotY * 1300.0f));
                const float variation = (broad - 0.5f) * 0.32f + (fine - 0.5f) * 0.14f + grain * 0.035f - knot * 0.18f;
                const Color wood = floor ? Color{132.0f, 91.0f, 57.0f} : Color{83.0f, 57.0f, 38.0f};
                const Color textured{wood.r + variation * 240.0f, wood.g + variation * 190.0f,
                    wood.b + variation * 130.0f};
                return seam ? Color{43.0f, 34.0f, 27.0f} : textured;
            }
            case MaterialType::Bark:
            {
                const float warp = valueNoise(u, v, 6, seed + 37u) * 1.8f +
                    valueNoise(u, v, 13, seed + 91u) * 0.7f;
                const float groove = std::sin((u * 23.0f + warp) * 2.0f * pi);
                const float deepGroove = std::pow(std::max(0.0f, -groove), 5.0f);
                Color color{102.0f, 72.0f, 48.0f};
                color = add(color, scale({42.0f, 30.0f, 20.0f}, broad - 0.5f));
                color = add(color, scale({24.0f, 18.0f, 12.0f}, fine - 0.5f));
                const float lichen = std::pow(std::max(0.0f, valueNoise(u, v, 11, seed + 151u) - 0.72f), 2.0f);
                color = add(color, scale({-35.0f, -27.0f, -17.0f}, deepGroove));
                return add(color, scale({17.0f, 20.0f, 9.0f}, lichen));
            }
            case MaterialType::Foliage:
            case MaterialType::AutumnLeaves:
            {
                const bool autumn = type == MaterialType::AutumnLeaves;
                const Color base = autumn ? Color{132.0f, 73.0f, 30.0f} : Color{48.0f, 78.0f, 38.0f};
                const float fleck = valueNoise(u, v, 53, seed + 81u) - 0.5f;
                Color leaves = add(base, scale({64.0f, 54.0f, 31.0f}, broad - 0.5f));
                leaves = add(leaves, scale({34.0f, 31.0f, 19.0f}, fleck));
                const float vein = std::pow(std::max(0.0f, 0.20f -
                    std::abs(valueNoise(u, v, 17, seed + 29u) - 0.5f)), 2.0f);
                leaves = add(leaves, scale({19.0f, 13.0f, 5.0f}, vein));
                if (autumn)
                {
                    const float colorShift = valueNoise(u, v, 7, seed + 21u);
                    leaves = mix(leaves, {180.0f, 111.0f, 38.0f}, colorShift * 0.52f);
                }
                else
                {
                    leaves = add(leaves, scale({25.0f, 31.0f, 13.0f}, valueNoise(u, v, 9, seed + 201u) - 0.5f));
                }
                return leaves;
            }
        }
        return {128.0f, 128.0f, 128.0f};
    }

    bool writeTexture(const std::filesystem::path& directory, const char* name,
        MaterialType type, std::uint32_t seed)
    {
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(textureSize) * textureSize * 3);
        for (int y = 0; y < textureSize; ++y)
        {
            for (int x = 0; x < textureSize; ++x)
            {
                const float u = static_cast<float>(x) / textureSize;
                const float v = static_cast<float>(y) / textureSize;
                const Color color = sample(type, u, v, seed);
                const std::size_t offset = (static_cast<std::size_t>(y) * textureSize + x) * 3;
                pixels[offset] = static_cast<std::uint8_t>(std::clamp(color.r, 0.0f, 255.0f));
                pixels[offset + 1] = static_cast<std::uint8_t>(std::clamp(color.g, 0.0f, 255.0f));
                pixels[offset + 2] = static_cast<std::uint8_t>(std::clamp(color.b, 0.0f, 255.0f));
            }
        }

        std::ofstream file(directory / name, std::ios::binary);
        if (!file)
            return false;
        file << "P6\n" << textureSize << " " << textureSize << "\n255\n";
        file.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
        return static_cast<bool>(file);
    }
}

int main(int argc, char** argv)
{
    const std::filesystem::path outputDirectory = argc > 1 ? argv[1] : "assets/textures";
    std::error_code error;
    std::filesystem::create_directories(outputDirectory, error);
    if (error)
    {
        std::cerr << "Could not create output directory: " << error.message() << '\n';
        return 1;
    }

    struct Texture
    {
        const char* name;
        MaterialType type;
        std::uint32_t seed;
    };
    constexpr std::array<Texture, 15> textures{{
        {"grass.ppm", MaterialType::Grass, 101u},
        {"soil.ppm", MaterialType::Soil, 207u},
        {"rock.ppm", MaterialType::Rock, 313u},
        {"cobblestone.ppm", MaterialType::Cobblestone, 419u},
        {"plaster_ochre.ppm", MaterialType::Plaster, 0u},
        {"plaster_blue.ppm", MaterialType::Plaster, 100u},
        {"plaster_olive.ppm", MaterialType::Plaster, 200u},
        {"roof_terracotta.ppm", MaterialType::Terracotta, 523u},
        {"roof_slate.ppm", MaterialType::Slate, 631u},
        {"roof_wood.ppm", MaterialType::Shingle, 739u},
        {"wood_floor.ppm", MaterialType::WoodFloor, 841u},
        {"dark_wood.ppm", MaterialType::DarkWood, 947u},
        {"bark.ppm", MaterialType::Bark, 1051u},
        {"foliage.ppm", MaterialType::Foliage, 1153u},
        {"autumn_leaves.ppm", MaterialType::AutumnLeaves, 1259u}
    }};

    for (const Texture& texture : textures)
    {
        if (!writeTexture(outputDirectory, texture.name, texture.type, texture.seed))
        {
            std::cerr << "Could not write " << (outputDirectory / texture.name).string() << '\n';
            return 1;
        }
    }
    std::cout << "Generated " << textures.size() << " tileable 256x256 textures in "
        << outputDirectory.string() << '\n';
    return 0;
}
