#include "Engine/Graphics/Scene/GltfLoader.hpp"

#include "Engine/Assets/AssetPath.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <variant>

namespace Engine::Graphics::Scene
{
    namespace
    {
        struct JsonValue
        {
            using Array = std::vector<JsonValue>;
            using Object = std::unordered_map<std::string, JsonValue>;
            std::variant<std::monostate, bool, double, std::string, Array, Object> value;

            const JsonValue* find(std::string_view key) const
            {
                const auto* object = std::get_if<Object>(&value);
                if (!object) return nullptr;
                const auto found = object->find(std::string(key));
                return found == object->end() ? nullptr : &found->second;
            }

            const JsonValue& at(std::size_t index) const
            {
                const auto* array = std::get_if<Array>(&value);
                if (!array || index >= array->size())
                    throw std::runtime_error("glTF JSON array index is out of range");
                return (*array)[index];
            }

            std::size_t size() const
            {
                const auto* array = std::get_if<Array>(&value);
                return array ? array->size() : 0;
            }

            double number(double fallback = 0.0) const
            {
                const auto* result = std::get_if<double>(&value);
                return result ? *result : fallback;
            }

            std::string string(std::string fallback = {}) const
            {
                const auto* result = std::get_if<std::string>(&value);
                return result ? *result : std::move(fallback);
            }
        };

        void appendUtf8(std::string& output, std::uint32_t codepoint)
        {
            if (codepoint <= 0x7f)
                output.push_back(static_cast<char>(codepoint));
            else if (codepoint <= 0x7ff)
            {
                output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
                output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
            else
            {
                output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
                output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
                output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
            }
        }

        class JsonParser
        {
        public:
            explicit JsonParser(std::string_view source) : source_(source) {}

            JsonValue parse()
            {
                JsonValue result = parseValue();
                skipWhitespace();
                if (position_ != source_.size())
                    throw std::runtime_error("unexpected characters after glTF JSON document");
                return result;
            }

        private:
            void skipWhitespace()
            {
                while (position_ < source_.size() &&
                    (source_[position_] == ' ' || source_[position_] == '\t' ||
                     source_[position_] == '\r' || source_[position_] == '\n'))
                    ++position_;
            }

            bool consume(char value)
            {
                skipWhitespace();
                if (position_ >= source_.size() || source_[position_] != value)
                    return false;
                ++position_;
                return true;
            }

            std::string parseString()
            {
                if (!consume('"')) throw std::runtime_error("expected a glTF JSON string");
                std::string result;
                while (position_ < source_.size())
                {
                    const unsigned char character = static_cast<unsigned char>(source_[position_++]);
                    if (character == '"') return result;
                    if (character < 0x20) throw std::runtime_error("invalid control character in glTF JSON string");
                    if (character != '\\')
                    {
                        result.push_back(static_cast<char>(character));
                        continue;
                    }
                    if (position_ >= source_.size()) break;
                    const char escaped = source_[position_++];
                    switch (escaped)
                    {
                        case '"': result.push_back('"'); break;
                        case '\\': result.push_back('\\'); break;
                        case '/': result.push_back('/'); break;
                        case 'b': result.push_back('\b'); break;
                        case 'f': result.push_back('\f'); break;
                        case 'n': result.push_back('\n'); break;
                        case 'r': result.push_back('\r'); break;
                        case 't': result.push_back('\t'); break;
                        case 'u':
                        {
                            if (position_ + 4 > source_.size())
                                throw std::runtime_error("truncated Unicode escape in glTF JSON");
                            std::uint32_t codepoint = 0;
                            for (int digit = 0; digit < 4; ++digit)
                            {
                                const char hex = source_[position_++];
                                codepoint <<= 4;
                                if (hex >= '0' && hex <= '9') codepoint |= static_cast<std::uint32_t>(hex - '0');
                                else if (hex >= 'a' && hex <= 'f') codepoint |= static_cast<std::uint32_t>(hex - 'a' + 10);
                                else if (hex >= 'A' && hex <= 'F') codepoint |= static_cast<std::uint32_t>(hex - 'A' + 10);
                                else throw std::runtime_error("invalid Unicode escape in glTF JSON");
                            }
                            appendUtf8(result, codepoint);
                            break;
                        }
                        default: throw std::runtime_error("invalid escape in glTF JSON string");
                    }
                }
                throw std::runtime_error("unterminated glTF JSON string");
            }

            JsonValue parseValue()
            {
                skipWhitespace();
                if (position_ >= source_.size()) throw std::runtime_error("unexpected end of glTF JSON");
                if (source_[position_] == '"')
                    return JsonValue{parseString()};
                if (source_[position_] == '{')
                {
                    ++position_;
                    JsonValue::Object object;
                    if (consume('}')) return JsonValue{std::move(object)};
                    do
                    {
                        const std::string key = parseString();
                        if (!consume(':')) throw std::runtime_error("expected ':' in glTF JSON object");
                        object.insert_or_assign(key, parseValue());
                    } while (consume(','));
                    if (!consume('}')) throw std::runtime_error("expected '}' in glTF JSON object");
                    return JsonValue{std::move(object)};
                }
                if (source_[position_] == '[')
                {
                    ++position_;
                    JsonValue::Array array;
                    if (consume(']')) return JsonValue{std::move(array)};
                    do { array.push_back(parseValue()); } while (consume(','));
                    if (!consume(']')) throw std::runtime_error("expected ']' in glTF JSON array");
                    return JsonValue{std::move(array)};
                }
                if (source_.substr(position_, 4) == "true")
                {
                    position_ += 4;
                    return JsonValue{true};
                }
                if (source_.substr(position_, 5) == "false")
                {
                    position_ += 5;
                    return JsonValue{false};
                }
                if (source_.substr(position_, 4) == "null")
                {
                    position_ += 4;
                    return JsonValue{};
                }
                const char* begin = source_.data() + position_;
                char* end = nullptr;
                const double parsed = std::strtod(begin, &end);
                if (end == begin || !std::isfinite(parsed))
                    throw std::runtime_error("invalid number in glTF JSON");
                position_ = static_cast<std::size_t>(end - source_.data());
                return JsonValue{parsed};
            }

            std::string_view source_;
            std::size_t position_ = 0;
        };

        const JsonValue& required(const JsonValue& object, std::string_view key)
        {
            const JsonValue* result = object.find(key);
            if (!result) throw std::runtime_error("glTF is missing required field '" + std::string(key) + "'");
            return *result;
        }

        std::size_t indexValue(const JsonValue& value)
        {
            const double number = value.number(-1.0);
            if (number < 0.0 || number > static_cast<double>(std::numeric_limits<std::size_t>::max()) ||
                std::floor(number) != number)
                throw std::runtime_error("invalid index in glTF JSON");
            return static_cast<std::size_t>(number);
        }

        std::uint32_t readU32(const std::vector<unsigned char>& bytes, std::size_t offset)
        {
            if (offset > bytes.size() || bytes.size() - offset < 4)
                throw std::runtime_error("truncated GLB chunk header");
            return static_cast<std::uint32_t>(bytes[offset]) |
                (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
                (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
                (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
        }

        std::size_t componentBytes(int componentType)
        {
            switch (componentType)
            {
                case 5120: case 5121: return 1;
                case 5122: case 5123: return 2;
                case 5125: case 5126: return 4;
                default: throw std::runtime_error("unsupported glTF accessor component type");
            }
        }

        int componentCount(const std::string& type)
        {
            if (type == "SCALAR") return 1;
            if (type == "VEC2") return 2;
            if (type == "VEC3") return 3;
            if (type == "VEC4") return 4;
            throw std::runtime_error("unsupported glTF accessor shape '" + type + "'");
        }

        struct AccessorData
        {
            const JsonValue* accessor = nullptr;
            std::size_t offset = 0;
            std::size_t stride = 0;
            std::size_t count = 0;
            std::size_t elementBytes = 0;
            int componentType = 0;
            int components = 0;
            bool normalized = false;
        };

        AccessorData accessorData(const JsonValue& root, const std::vector<unsigned char>& binary,
            std::size_t accessorIndex, const char* expectedType = nullptr)
        {
            const JsonValue& accessors = required(root, "accessors");
            const JsonValue& accessor = accessors.at(accessorIndex);
            if (accessor.find("sparse"))
                throw std::runtime_error("sparse glTF accessors are not supported");
            const std::size_t viewIndex = indexValue(required(accessor, "bufferView"));
            const JsonValue& views = required(root, "bufferViews");
            const JsonValue& view = views.at(viewIndex);
            if (view.find("buffer") && indexValue(*view.find("buffer")) != 0)
                throw std::runtime_error("GLB references a non-binary buffer");
            AccessorData data;
            data.accessor = &accessor;
            data.componentType = static_cast<int>(required(accessor, "componentType").number());
            data.components = componentCount(required(accessor, "type").string());
            if (expectedType && required(accessor, "type").string() != expectedType)
                throw std::runtime_error("glTF attribute has an unexpected accessor type");
            data.count = indexValue(required(accessor, "count"));
            data.normalized = accessor.find("normalized") &&
                std::get_if<bool>(&accessor.find("normalized")->value) &&
                std::get<bool>(accessor.find("normalized")->value);
            data.elementBytes = componentBytes(data.componentType) * static_cast<std::size_t>(data.components);
            const std::size_t viewOffset = view.find("byteOffset") ? indexValue(*view.find("byteOffset")) : 0;
            const std::size_t accessorOffset = accessor.find("byteOffset") ? indexValue(*accessor.find("byteOffset")) : 0;
            if (viewOffset > std::numeric_limits<std::size_t>::max() - accessorOffset)
                throw std::runtime_error("glTF accessor offset overflow");
            data.offset = viewOffset + accessorOffset;
            data.stride = view.find("byteStride") ? indexValue(*view.find("byteStride")) : data.elementBytes;
            if (data.stride < data.elementBytes || data.count == 0 ||
                data.count - 1 > (std::numeric_limits<std::size_t>::max() - data.elementBytes) / data.stride)
                throw std::runtime_error("invalid glTF accessor layout");
            const std::size_t finalEnd = data.offset + (data.count - 1) * data.stride + data.elementBytes;
            if (finalEnd > binary.size())
                throw std::runtime_error("glTF accessor exceeds the binary buffer");
            if (view.find("byteLength"))
            {
                const std::size_t viewEnd = viewOffset + indexValue(*view.find("byteLength"));
                if (finalEnd > viewEnd)
                    throw std::runtime_error("glTF accessor exceeds its buffer view");
            }
            return data;
        }

        double readComponent(const std::vector<unsigned char>& bytes, const AccessorData& accessor,
            std::size_t element, std::size_t component)
        {
            const std::size_t offset = accessor.offset + element * accessor.stride +
                component * componentBytes(accessor.componentType);
            switch (accessor.componentType)
            {
                case 5126:
                {
                    float result = 0.0f;
                    std::memcpy(&result, bytes.data() + offset, sizeof(result));
                    return result;
                }
                case 5125:
                {
                    std::uint32_t result = 0;
                    std::memcpy(&result, bytes.data() + offset, sizeof(result));
                    return result;
                }
                case 5123:
                {
                    std::uint16_t result = 0;
                    std::memcpy(&result, bytes.data() + offset, sizeof(result));
                    if (accessor.normalized) return static_cast<double>(result) / 65535.0;
                    return result;
                }
                case 5121:
                    if (accessor.normalized) return static_cast<double>(bytes[offset]) / 255.0;
                    return bytes[offset];
                default:
                    throw std::runtime_error("unsupported glTF attribute data type");
            }
        }

        std::uint32_t readIndex(const std::vector<unsigned char>& bytes, const AccessorData& accessor,
            std::size_t element)
        {
            const std::size_t offset = accessor.offset + element * accessor.stride;
            if (accessor.componentType == 5121) return bytes[offset];
            if (accessor.componentType == 5123)
            {
                std::uint16_t result = 0;
                std::memcpy(&result, bytes.data() + offset, sizeof(result));
                return result;
            }
            if (accessor.componentType == 5125)
            {
                std::uint32_t result = 0;
                std::memcpy(&result, bytes.data() + offset, sizeof(result));
                return result;
            }
            throw std::runtime_error("glTF indices must use an unsigned integer component type");
        }

        Math::Mat4 nodeTransform(const JsonValue& node)
        {
            Math::Mat4 transform = Math::identity();
            if (const JsonValue* matrix = node.find("matrix"))
            {
                if (matrix->size() != 16) throw std::runtime_error("glTF node matrix must contain 16 values");
                for (std::size_t index = 0; index < 16; ++index)
                    transform.m[index] = static_cast<float>(matrix->at(index).number());
                return transform;
            }

            const JsonValue* translation = node.find("translation");
            const JsonValue* rotation = node.find("rotation");
            const JsonValue* scale = node.find("scale");
            const float tx = translation && translation->size() > 0 ? static_cast<float>(translation->at(0).number()) : 0.0f;
            const float ty = translation && translation->size() > 1 ? static_cast<float>(translation->at(1).number()) : 0.0f;
            const float tz = translation && translation->size() > 2 ? static_cast<float>(translation->at(2).number()) : 0.0f;
            const float qx = rotation && rotation->size() > 0 ? static_cast<float>(rotation->at(0).number()) : 0.0f;
            const float qy = rotation && rotation->size() > 1 ? static_cast<float>(rotation->at(1).number()) : 0.0f;
            const float qz = rotation && rotation->size() > 2 ? static_cast<float>(rotation->at(2).number()) : 0.0f;
            const float qw = rotation && rotation->size() > 3 ? static_cast<float>(rotation->at(3).number(1.0)) : 1.0f;
            const float sx = scale && scale->size() > 0 ? static_cast<float>(scale->at(0).number()) : 1.0f;
            const float sy = scale && scale->size() > 1 ? static_cast<float>(scale->at(1).number()) : 1.0f;
            const float sz = scale && scale->size() > 2 ? static_cast<float>(scale->at(2).number()) : 1.0f;
            transform.m[0] = (1.0f - 2.0f * (qy * qy + qz * qz)) * sx;
            transform.m[1] = (2.0f * (qx * qy + qz * qw)) * sx;
            transform.m[2] = (2.0f * (qx * qz - qy * qw)) * sx;
            transform.m[4] = (2.0f * (qx * qy - qz * qw)) * sy;
            transform.m[5] = (1.0f - 2.0f * (qx * qx + qz * qz)) * sy;
            transform.m[6] = (2.0f * (qy * qz + qx * qw)) * sy;
            transform.m[8] = (2.0f * (qx * qz + qy * qw)) * sz;
            transform.m[9] = (2.0f * (qy * qz - qx * qw)) * sz;
            transform.m[10] = (1.0f - 2.0f * (qx * qx + qy * qy)) * sz;
            transform.m[12] = tx;
            transform.m[13] = ty;
            transform.m[14] = tz;
            return transform;
        }

        Math::Vec3 transformPosition(const Math::Mat4& transform, Math::Vec3 position)
        {
            return {
                transform.m[0] * position.x + transform.m[4] * position.y + transform.m[8] * position.z + transform.m[12],
                transform.m[1] * position.x + transform.m[5] * position.y + transform.m[9] * position.z + transform.m[13],
                transform.m[2] * position.x + transform.m[6] * position.y + transform.m[10] * position.z + transform.m[14]
            };
        }

        Math::Vec3 transformNormal(const Math::Mat4& transform, Math::Vec3 normal)
        {
            const float a = transform.m[0], b = transform.m[4], c = transform.m[8];
            const float d = transform.m[1], e = transform.m[5], f = transform.m[9];
            const float g = transform.m[2], h = transform.m[6], i = transform.m[10];
            const float determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
            if (std::abs(determinant) < 1.0e-8f)
                return Math::normalize({a * normal.x + b * normal.y + c * normal.z,
                    d * normal.x + e * normal.y + f * normal.z,
                    g * normal.x + h * normal.y + i * normal.z});
            const float inverse = 1.0f / determinant;
            return Math::normalize({
                ((e * i - f * h) * normal.x + (f * g - d * i) * normal.y + (d * h - e * g) * normal.z) * inverse,
                ((c * h - b * i) * normal.x + (a * i - c * g) * normal.y + (b * g - a * h) * normal.z) * inverse,
                ((b * f - c * e) * normal.x + (c * d - a * f) * normal.y + (a * e - b * d) * normal.z) * inverse
            });
        }

        std::string imageExtension(const std::string& mimeType)
        {
            if (mimeType == "image/png") return ".png";
            if (mimeType == "image/jpeg") return ".jpg";
            throw std::runtime_error("GLB contains an unsupported embedded image format '" + mimeType + "'");
        }

        std::string extractImage(const JsonValue& root, const std::vector<unsigned char>& binary,
            const JsonValue& image, std::size_t imageIndex, const std::filesystem::path& modelPath,
            const std::filesystem::path& cacheDirectory)
        {
            if (const JsonValue* uri = image.find("uri"))
            {
                const std::string value = uri->string();
                if (value.starts_with("data:"))
                    throw std::runtime_error("data URI images are not supported in GLB files");
                return (modelPath.parent_path() / std::filesystem::path(value)).lexically_normal().string();
            }
            if (!image.find("bufferView"))
                throw std::runtime_error("glTF image must specify a URI or bufferView");
            const std::size_t viewIndex = indexValue(required(image, "bufferView"));
            const JsonValue& view = required(root, "bufferViews").at(viewIndex);
            if (view.find("buffer") && indexValue(*view.find("buffer")) != 0)
                throw std::runtime_error("embedded image references a non-binary GLB buffer");
            const std::size_t offset = view.find("byteOffset") ? indexValue(*view.find("byteOffset")) : 0;
            const std::size_t length = indexValue(required(view, "byteLength"));
            if (offset > binary.size() || length > binary.size() - offset)
                throw std::runtime_error("embedded GLB image exceeds the binary chunk");
            const std::string extension = imageExtension(required(image, "mimeType").string());
            const std::filesystem::path destination = cacheDirectory /
                ("image_" + std::to_string(imageIndex) + extension);
            std::error_code error;
            if (std::filesystem::exists(destination, error) && !error &&
                std::filesystem::file_size(destination, error) == length && !error)
                return destination.string();
            std::filesystem::create_directories(cacheDirectory);
            std::ofstream file(destination, std::ios::binary | std::ios::trunc);
            if (!file.write(reinterpret_cast<const char*>(binary.data() + offset), static_cast<std::streamsize>(length)))
                throw std::runtime_error("could not extract embedded GLB texture to temporary storage");
            return destination.string();
        }

        Math::Vec3 readVec3(const std::vector<unsigned char>& binary, const AccessorData& accessor,
            std::size_t index)
        {
            return {static_cast<float>(readComponent(binary, accessor, index, 0)),
                static_cast<float>(readComponent(binary, accessor, index, 1)),
                static_cast<float>(readComponent(binary, accessor, index, 2))};
        }

        bool loadGlbImpl(const std::filesystem::path& requestedPath, Scene& outputScene,
            ModelBounds& outputBounds, std::string& error)
        {
            try
            {
                const std::filesystem::path modelPath = Assets::resolveAssetPath(requestedPath);
                std::ifstream file(modelPath, std::ios::binary);
                if (!file) throw std::runtime_error("Could not open GLB file: " + modelPath.string());
                std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                if (bytes.size() < 20 || readU32(bytes, 0) != 0x46546c67u || readU32(bytes, 4) != 2 ||
                    readU32(bytes, 8) != bytes.size())
                    throw std::runtime_error("file is not a valid glTF binary 2.0 document");

                std::size_t offset = 12;
                std::string jsonText;
                std::vector<unsigned char> binary;
                while (offset + 8 <= bytes.size())
                {
                    const std::uint32_t chunkLength = readU32(bytes, offset);
                    const std::uint32_t chunkType = readU32(bytes, offset + 4);
                    offset += 8;
                    if (chunkLength > bytes.size() - offset)
                        throw std::runtime_error("GLB chunk exceeds the file length");
                    if (chunkType == 0x4e4f534au && jsonText.empty())
                        jsonText.assign(reinterpret_cast<const char*>(bytes.data() + offset), chunkLength);
                    else if (chunkType == 0x004e4942u && binary.empty())
                        binary.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                            bytes.begin() + static_cast<std::ptrdiff_t>(offset + chunkLength));
                    offset += chunkLength;
                }
                while (!jsonText.empty() && (jsonText.back() == '\0' || jsonText.back() == ' ' ||
                    jsonText.back() == '\r' || jsonText.back() == '\n' || jsonText.back() == '\t'))
                    jsonText.pop_back();
                if (jsonText.empty() || binary.empty())
                    throw std::runtime_error("GLB is missing its JSON or binary chunk");
                const JsonValue root = JsonParser(jsonText).parse();
                const JsonValue& asset = required(root, "asset");
                if (asset.find("version") && asset.find("version")->string() != "2.0")
                    throw std::runtime_error("only glTF 2.0 files are supported");
                if (const JsonValue* extensions = root.find("extensionsRequired"); extensions && extensions->size() > 0)
                    throw std::runtime_error("GLB uses required extensions that Duma3D does not support");

                std::ostringstream hashText;
                hashText << std::hex << std::hash<std::string>{}(modelPath.lexically_normal().string());
                const std::filesystem::path cacheDirectory = std::filesystem::temp_directory_path() /
                    "Duma3D" / "glb_textures" / hashText.str();
                std::vector<std::string> imagePaths;
                if (const JsonValue* images = root.find("images"))
                {
                    imagePaths.reserve(images->size());
                    for (std::size_t index = 0; index < images->size(); ++index)
                        imagePaths.push_back(extractImage(root, binary, images->at(index), index, modelPath, cacheDirectory));
                }
                std::vector<std::string> texturePaths;
                if (const JsonValue* textures = root.find("textures"))
                {
                    texturePaths.reserve(textures->size());
                    for (std::size_t index = 0; index < textures->size(); ++index)
                    {
                        const JsonValue& texture = textures->at(index);
                        if (!texture.find("source")) throw std::runtime_error("glTF texture has no image source");
                        const std::size_t source = indexValue(*texture.find("source"));
                        if (source >= imagePaths.size()) throw std::runtime_error("glTF texture image index is out of range");
                        texturePaths.push_back(imagePaths[source]);
                    }
                }

                Scene scene;
                scene.addMaterial({{1.0f, 1.0f, 1.0f}, {}});
                std::vector<std::size_t> materialIndices;
                if (const JsonValue* materials = root.find("materials"))
                {
                    materialIndices.reserve(materials->size());
                    for (std::size_t index = 0; index < materials->size(); ++index)
                    {
                        const JsonValue& material = materials->at(index);
                        Math::Vec3 baseColor{1.0f, 1.0f, 1.0f};
                        std::string texturePath;
                        if (const JsonValue* pbr = material.find("pbrMetallicRoughness"))
                        {
                            if (const JsonValue* factor = pbr->find("baseColorFactor"))
                            {
                                if (factor->size() != 4) throw std::runtime_error("glTF baseColorFactor must have four values");
                                baseColor = {static_cast<float>(factor->at(0).number()),
                                    static_cast<float>(factor->at(1).number()),
                                    static_cast<float>(factor->at(2).number())};
                            }
                            if (const JsonValue* textureInfo = pbr->find("baseColorTexture"))
                            {
                                const std::size_t textureIndex = indexValue(required(*textureInfo, "index"));
                                if (textureIndex >= texturePaths.size())
                                    throw std::runtime_error("glTF material texture index is out of range");
                                texturePath = texturePaths[textureIndex];
                            }
                        }
                        materialIndices.push_back(scene.addMaterial({baseColor, std::move(texturePath)}));
                    }
                }

                const JsonValue& nodes = required(root, "nodes");
                const JsonValue& meshes = required(root, "meshes");
                ModelBounds bounds{{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
                    {-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()}};
                std::array<std::vector<float>, 3> objectCenters;
                bool hasGeometry = false;
                std::function<void(std::size_t, const Math::Mat4&)> visitNode;
                visitNode = [&](std::size_t nodeIndex, const Math::Mat4& parentTransform)
                {
                    const JsonValue& node = nodes.at(nodeIndex);
                    const Math::Mat4 worldTransform = Math::multiply(parentTransform, nodeTransform(node));
                    if (const JsonValue* nodeMesh = node.find("mesh"))
                    {
                        const JsonValue& mesh = meshes.at(indexValue(*nodeMesh));
                        const JsonValue& primitives = required(mesh, "primitives");
                        for (std::size_t primitiveIndex = 0; primitiveIndex < primitives.size(); ++primitiveIndex)
                        {
                            const JsonValue& primitive = primitives.at(primitiveIndex);
                            const std::size_t mode = primitive.find("mode") ? indexValue(*primitive.find("mode")) : 4;
                            if (mode != 4) throw std::runtime_error("only triangle-list glTF primitives are supported");
                            const JsonValue& attributes = required(primitive, "attributes");
                            const AccessorData position = accessorData(root, binary,
                                indexValue(required(attributes, "POSITION")), "VEC3");
                            const AccessorData* normal = nullptr;
                            AccessorData normalData;
                            if (const JsonValue* normalIndex = attributes.find("NORMAL"))
                            {
                                normalData = accessorData(root, binary, indexValue(*normalIndex), "VEC3");
                                if (normalData.count != position.count)
                                    throw std::runtime_error("glTF normal and position counts differ");
                                normal = &normalData;
                            }
                            const AccessorData* texcoord = nullptr;
                            AccessorData texcoordData;
                            if (const JsonValue* uvIndex = attributes.find("TEXCOORD_0"))
                            {
                                texcoordData = accessorData(root, binary, indexValue(*uvIndex), "VEC2");
                                if (texcoordData.count != position.count)
                                    throw std::runtime_error("glTF UV and position counts differ");
                                texcoord = &texcoordData;
                            }

                            Mesh outputMesh;
                            outputMesh.vertices.resize(position.count);
                            Math::Vec3 primitiveMinimum{std::numeric_limits<float>::max(),
                                std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
                            Math::Vec3 primitiveMaximum{-std::numeric_limits<float>::max(),
                                -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()};
                            for (std::size_t vertexIndex = 0; vertexIndex < position.count; ++vertexIndex)
                            {
                                Vertex& vertex = outputMesh.vertices[vertexIndex];
                                vertex.position = transformPosition(worldTransform, readVec3(binary, position, vertexIndex));
                                if (normal)
                                    vertex.normal = transformNormal(worldTransform, readVec3(binary, *normal, vertexIndex));
                                if (texcoord)
                                {
                                    vertex.textureCoordinate = {
                                        static_cast<float>(readComponent(binary, *texcoord, vertexIndex, 0)),
                                        static_cast<float>(readComponent(binary, *texcoord, vertexIndex, 1))};
                                }
                                bounds.minimum.x = std::min(bounds.minimum.x, vertex.position.x);
                                bounds.minimum.y = std::min(bounds.minimum.y, vertex.position.y);
                                bounds.minimum.z = std::min(bounds.minimum.z, vertex.position.z);
                                bounds.maximum.x = std::max(bounds.maximum.x, vertex.position.x);
                                bounds.maximum.y = std::max(bounds.maximum.y, vertex.position.y);
                                bounds.maximum.z = std::max(bounds.maximum.z, vertex.position.z);
                                primitiveMinimum.x = std::min(primitiveMinimum.x, vertex.position.x);
                                primitiveMinimum.y = std::min(primitiveMinimum.y, vertex.position.y);
                                primitiveMinimum.z = std::min(primitiveMinimum.z, vertex.position.z);
                                primitiveMaximum.x = std::max(primitiveMaximum.x, vertex.position.x);
                                primitiveMaximum.y = std::max(primitiveMaximum.y, vertex.position.y);
                                primitiveMaximum.z = std::max(primitiveMaximum.z, vertex.position.z);
                            }
                            objectCenters[0].push_back((primitiveMinimum.x + primitiveMaximum.x) * 0.5f);
                            objectCenters[1].push_back((primitiveMinimum.y + primitiveMaximum.y) * 0.5f);
                            objectCenters[2].push_back((primitiveMinimum.z + primitiveMaximum.z) * 0.5f);
                            if (const JsonValue* indexAccessor = primitive.find("indices"))
                            {
                                const AccessorData indices = accessorData(root, binary, indexValue(*indexAccessor), "SCALAR");
                                outputMesh.indices.reserve(indices.count);
                                for (std::size_t index = 0; index < indices.count; ++index)
                                {
                                    const std::uint32_t vertexIndex = readIndex(binary, indices, index);
                                    if (vertexIndex >= outputMesh.vertices.size())
                                        throw std::runtime_error("glTF primitive index is outside its vertex array");
                                    outputMesh.indices.push_back(vertexIndex);
                                }
                            }
                            else
                            {
                                outputMesh.indices.reserve(outputMesh.vertices.size());
                                for (std::size_t index = 0; index < outputMesh.vertices.size(); ++index)
                                    outputMesh.indices.push_back(static_cast<std::uint32_t>(index));
                            }
                            if (outputMesh.indices.empty() || outputMesh.indices.size() % 3 != 0)
                                throw std::runtime_error("glTF triangle primitive has an invalid index count");
                            if (!normal)
                            {
                                for (std::size_t index = 0; index < outputMesh.indices.size(); index += 3)
                                {
                                    Vertex& a = outputMesh.vertices[outputMesh.indices[index]];
                                    Vertex& b = outputMesh.vertices[outputMesh.indices[index + 1]];
                                    Vertex& c = outputMesh.vertices[outputMesh.indices[index + 2]];
                                    const Math::Vec3 first{b.position.x - a.position.x, b.position.y - a.position.y, b.position.z - a.position.z};
                                    const Math::Vec3 second{c.position.x - a.position.x, c.position.y - a.position.y, c.position.z - a.position.z};
                                    const Math::Vec3 face = Math::cross(first, second);
                                    a.normal.x += face.x; a.normal.y += face.y; a.normal.z += face.z;
                                    b.normal.x += face.x; b.normal.y += face.y; b.normal.z += face.z;
                                    c.normal.x += face.x; c.normal.y += face.y; c.normal.z += face.z;
                                }
                                for (Vertex& vertex : outputMesh.vertices)
                                    vertex.normal = Math::normalize(vertex.normal);
                            }
                            std::size_t materialIndex = 0;
                            if (const JsonValue* material = primitive.find("material"))
                            {
                                const std::size_t index = indexValue(*material);
                                if (index >= materialIndices.size()) throw std::runtime_error("glTF primitive material index is out of range");
                                materialIndex = materialIndices[index];
                            }
                            const std::size_t meshIndex = scene.addMesh(std::move(outputMesh));
                            if (!scene.addObject(meshIndex, materialIndex))
                                throw std::runtime_error("could not add a glTF mesh to the scene");
                            hasGeometry = true;
                        }
                    }
                    if (const JsonValue* children = node.find("children"))
                        for (std::size_t child = 0; child < children->size(); ++child)
                            visitNode(indexValue(children->at(child)), worldTransform);
                };

                const JsonValue& scenes = required(root, "scenes");
                const std::size_t sceneIndex = root.find("scene") ? indexValue(*root.find("scene")) : 0;
                const JsonValue& activeScene = scenes.at(sceneIndex);
                const JsonValue& roots = required(activeScene, "nodes");
                for (std::size_t index = 0; index < roots.size(); ++index)
                    visitNode(indexValue(roots.at(index)), Math::identity());
                if (!hasGeometry) throw std::runtime_error("GLB scene contains no renderable triangle meshes");
                for (std::size_t axis = 0; axis < objectCenters.size(); ++axis)
                {
                    auto& centers = objectCenters[axis];
                    const std::size_t middle = centers.size() / 2;
                    std::nth_element(centers.begin(), centers.begin() + static_cast<std::ptrdiff_t>(middle), centers.end());
                    float median = centers[middle];
                    if (centers.size() % 2 == 0)
                    {
                        const float lower = *std::max_element(centers.begin(), centers.begin() + static_cast<std::ptrdiff_t>(middle));
                        median = (median + lower) * 0.5f;
                    }
                    if (axis == 0) bounds.suggestedStart.x = median;
                    else if (axis == 1) bounds.suggestedStart.y = median;
                    else bounds.suggestedStart.z = median;
                }
                outputScene = std::move(scene);
                outputBounds = bounds;
                error.clear();
                return true;
            }
            catch (const std::exception& exception)
            {
                error = exception.what();
                return false;
            }
        }
    }

    bool GltfLoader::loadGlb(const std::filesystem::path& path, Scene& scene,
        ModelBounds& bounds, std::string& error)
    {
        return loadGlbImpl(path, scene, bounds, error);
    }
}
