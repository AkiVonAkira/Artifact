#pragma once

#include <cstdint>
#include <string>

namespace Artifact::Assets {

    enum class TextureFormat {
        Unknown = 0,
        R8,
        RG8,
        RGB8,
        RGBA8,
        RGB16F,
        RGBA16F,
        RGB32F,
        RGBA32F
    };

    enum class TextureWrap {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class TextureFilter {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };

    struct TextureData {
        uint8_t* pixels{nullptr};
        int width{0};
        int height{0};
        int channels{0};
        TextureFormat format{TextureFormat::Unknown};

        ~TextureData();

        // Disable copy
        TextureData(const TextureData&) = delete;
        TextureData& operator=(const TextureData&) = delete;

        // Enable move
        TextureData(TextureData&& other) noexcept;
        TextureData& operator=(TextureData&& other) noexcept;

        TextureData() = default;

        [[nodiscard]] bool isValid() const { return pixels != nullptr && width > 0 && height > 0; }
        [[nodiscard]] size_t size() const { return width * height * channels; }
    };

    /// Texture loading and management using stb_image
    class TextureLoader {
    public:
        /// Load a texture from file
        static TextureData loadFromFile(const std::string& path, bool flipVertically = true, int desiredChannels = 0);

        /// Load a texture from memory
        static TextureData loadFromMemory(const uint8_t* data, size_t size, bool flipVertically = true, int desiredChannels = 0);

        /// Create an OpenGL texture from texture data
        static uint32_t createGLTexture(const TextureData& data,
                                       TextureFilter minFilter = TextureFilter::LinearMipmapLinear,
                                       TextureFilter magFilter = TextureFilter::Linear,
                                       TextureWrap wrap = TextureWrap::Repeat,
                                       bool generateMipmaps = true);

        /// Load and create OpenGL texture in one step
        static uint32_t loadTexture(const std::string& path,
                                   bool flipVertically = true,
                                   TextureFilter minFilter = TextureFilter::LinearMipmapLinear,
                                   TextureFilter magFilter = TextureFilter::Linear,
                                   TextureWrap wrap = TextureWrap::Repeat,
                                   bool generateMipmaps = true);

        /// Delete an OpenGL texture
        static void deleteTexture(uint32_t textureId);

        /// Get info about an image file without loading it
        static bool getImageInfo(const std::string& path, int& width, int& height, int& channels);

    private:
        static TextureFormat getFormat(int channels);
    };

} // namespace Artifact::Assets

