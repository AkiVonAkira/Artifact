#include "Artifact/Assets/TextureLoader.h"
#include "Artifact/Core/LogManager.h"
#include "Artifact/Core/Paths.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glad/glad.h>

namespace Artifact::Assets {

    TextureData::~TextureData() {
        if (pixels) {
            stbi_image_free(pixels);
            pixels = nullptr;
        }
    }

    TextureData::TextureData(TextureData&& other) noexcept
        : pixels(other.pixels)
        , width(other.width)
        , height(other.height)
        , channels(other.channels)
        , format(other.format) {
        other.pixels = nullptr;
        other.width = 0;
        other.height = 0;
        other.channels = 0;
        other.format = TextureFormat::Unknown;
    }

    TextureData& TextureData::operator=(TextureData&& other) noexcept {
        if (this != &other) {
            if (pixels) {
                stbi_image_free(pixels);
            }

            pixels = other.pixels;
            width = other.width;
            height = other.height;
            channels = other.channels;
            format = other.format;

            other.pixels = nullptr;
            other.width = 0;
            other.height = 0;
            other.channels = 0;
            other.format = TextureFormat::Unknown;
        }
        return *this;
    }

    TextureData TextureLoader::loadFromFile(const std::string& path, bool flipVertically, int desiredChannels) {
        TextureData data;

        auto resolvedPath = Core::Paths::ResolveAssetPath(path);
        if (!std::filesystem::exists(resolvedPath)) {
            LOG_ERROR("TextureLoader: File not found: {}", path);
            return data;
        }

        stbi_set_flip_vertically_on_load(flipVertically);

        data.pixels = stbi_load(resolvedPath.string().c_str(), &data.width, &data.height, &data.channels, desiredChannels);

        if (!data.pixels) {
            LOG_ERROR("TextureLoader: Failed to load texture: {}", path);
            LOG_ERROR("  STB Error: {}", stbi_failure_reason());
            return data;
        }

        if (desiredChannels > 0) {
            data.channels = desiredChannels;
        }

        data.format = getFormat(data.channels);

        LOG_INFO("TextureLoader: Loaded texture '{}' ({}x{}, {} channels)",
                 path, data.width, data.height, data.channels);

        return data;
    }

    TextureData TextureLoader::loadFromMemory(const uint8_t* buffer, size_t size, bool flipVertically, int desiredChannels) {
        TextureData data;

        if (!buffer || size == 0) {
            LOG_ERROR("TextureLoader: Invalid buffer");
            return data;
        }

        stbi_set_flip_vertically_on_load(flipVertically);

        data.pixels = stbi_load_from_memory(buffer, static_cast<int>(size), &data.width, &data.height, &data.channels, desiredChannels);

        if (!data.pixels) {
            LOG_ERROR("TextureLoader: Failed to load texture from memory");
            LOG_ERROR("  STB Error: {}", stbi_failure_reason());
            return data;
        }

        if (desiredChannels > 0) {
            data.channels = desiredChannels;
        }

        data.format = getFormat(data.channels);

        LOG_INFO("TextureLoader: Loaded texture from memory ({}x{}, {} channels)",
                 data.width, data.height, data.channels);

        return data;
    }

    uint32_t TextureLoader::createGLTexture(const TextureData& data,
                                           TextureFilter minFilter,
                                           TextureFilter magFilter,
                                           TextureWrap wrap,
                                           bool generateMipmaps) {
        if (!data.isValid()) {
            LOG_ERROR("TextureLoader: Cannot create GL texture from invalid data");
            return 0;
        }

        GLuint textureId;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // Set texture wrap mode
        GLenum wrapMode = GL_REPEAT;
        switch (wrap) {
            case TextureWrap::Repeat: wrapMode = GL_REPEAT; break;
            case TextureWrap::MirroredRepeat: wrapMode = GL_MIRRORED_REPEAT; break;
            case TextureWrap::ClampToEdge: wrapMode = GL_CLAMP_TO_EDGE; break;
            case TextureWrap::ClampToBorder: wrapMode = GL_CLAMP_TO_BORDER; break;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);

        // Set texture filtering
        auto getGLFilter = [](TextureFilter filter) -> GLenum {
            switch (filter) {
                case TextureFilter::Nearest: return GL_NEAREST;
                case TextureFilter::Linear: return GL_LINEAR;
                case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST;
                case TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST;
                case TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR;
                case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR;
                default: return GL_LINEAR;
            }
        };

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, getGLFilter(minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, getGLFilter(magFilter));

        // Upload texture data
        GLenum format = GL_RGBA;
        GLenum internalFormat = GL_RGBA8;

        switch (data.channels) {
            case 1:
                format = GL_RED;
                internalFormat = GL_R8;
                break;
            case 2:
                format = GL_RG;
                internalFormat = GL_RG8;
                break;
            case 3:
                format = GL_RGB;
                internalFormat = GL_RGB8;
                break;
            case 4:
                format = GL_RGBA;
                internalFormat = GL_RGBA8;
                break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, data.width, data.height, 0, format, GL_UNSIGNED_BYTE, data.pixels);

        if (generateMipmaps) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        glBindTexture(GL_TEXTURE_2D, 0);

        LOG_INFO("TextureLoader: Created GL texture ID {} ({}x{})", textureId, data.width, data.height);

        return textureId;
    }

    uint32_t TextureLoader::loadTexture(const std::string& path,
                                       bool flipVertically,
                                       TextureFilter minFilter,
                                       TextureFilter magFilter,
                                       TextureWrap wrap,
                                       bool generateMipmaps) {
        auto data = loadFromFile(path, flipVertically, 0);
        if (!data.isValid()) {
            return 0;
        }

        return createGLTexture(data, minFilter, magFilter, wrap, generateMipmaps);
    }

    void TextureLoader::deleteTexture(uint32_t textureId) {
        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
        }
    }

    bool TextureLoader::getImageInfo(const std::string& path, int& width, int& height, int& channels) {
        auto resolvedPath = Core::Paths::ResolveAssetPath(path);
        if (!std::filesystem::exists(resolvedPath)) {
            LOG_ERROR("TextureLoader: File not found: {}", path);
            return false;
        }

        int result = stbi_info(resolvedPath.string().c_str(), &width, &height, &channels);
        return result != 0;
    }

    TextureFormat TextureLoader::getFormat(int channels) {
        switch (channels) {
            case 1: return TextureFormat::R8;
            case 2: return TextureFormat::RG8;
            case 3: return TextureFormat::RGB8;
            case 4: return TextureFormat::RGBA8;
            default: return TextureFormat::Unknown;
        }
    }

} // namespace Artifact::Assets

