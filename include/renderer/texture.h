#ifndef TEXTURE_H
#define TEXTURE_H
#include <stb/stb_image.h>
#include "core.h"

using layer_t = int;

class Texture
{
public:
    Texture CreateRegularTexture(std::string_view filepath, bool pixelated);

    std::string_view m_filepath;
    int m_width{};
    int m_height{};
    int m_channel_amount{};
    GLint m_texture_format{};
    GLint m_internal_format{};
    uint32 m_texture_Id{};
};

class TextureArray : public Texture
{
public:
    TextureArray CreateAtlasSlice(std::string_view filepath, bool pixelated);

    uint16 layer_amount{};
};

class CubeMap : public Texture
{
public:
    CubeMap CreateCubeMap(std::string_view filepath);
};

#endif