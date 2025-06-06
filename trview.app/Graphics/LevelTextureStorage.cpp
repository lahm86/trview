#include "LevelTextureStorage.h"
#include "TextureStorage.h"

namespace trview
{
    ILevelTextureStorage::~ILevelTextureStorage()
    {
    }

    LevelTextureStorage::LevelTextureStorage(const std::shared_ptr<graphics::IDevice>& device, std::unique_ptr<ITextureStorage> texture_storage)
        : _device(device), _texture_storage(std::move(texture_storage))
    {
    }

    void LevelTextureStorage::determine_texture_mode()
    {
        for (const auto& object_texture : _object_textures)
        {
            for (const auto& vert : object_texture.Vertices)
            {
                if ((vert.x_frac > 1 && vert.x_frac < 255) ||
                    (vert.y_frac > 1 && vert.y_frac < 255))
                {
                    _texture_mode = TextureMode::Custom;
                    return;
                }
            }
        }
    }

    void LevelTextureStorage::add_texture(const std::vector<uint32_t>& pixels, uint32_t width, uint32_t height)
    {
        _texture_storage->add_texture(pixels, width, height);
    }

    graphics::Texture LevelTextureStorage::texture(uint32_t tile_index) const
    {
        return _texture_storage->texture(tile_index);
    }

    graphics::Texture LevelTextureStorage::opaque_texture(uint32_t tile_index) const
    {
        return _opaque_tiles[tile_index];
    }

    graphics::Texture LevelTextureStorage::coloured(uint32_t colour) const
    {
        return _texture_storage->coloured(colour);
    }

    graphics::Texture LevelTextureStorage::untextured() const
    {
        return _texture_storage->untextured();
    }

    DirectX::SimpleMath::Vector2 LevelTextureStorage::uv(uint32_t texture_index, uint32_t uv_index) const
    {
        using namespace DirectX::SimpleMath;
        if (texture_index >= _object_textures.size())
        {
            return Vector2::Zero;
        }

        const auto& vert = _object_textures[texture_index].Vertices[uv_index];

        if (_texture_mode == TextureMode::Official)
        {
            return Vector2(static_cast<float>(vert.x_whole + static_cast<int8_t>(vert.x_frac)), static_cast<float>(vert.y_whole + static_cast<int8_t>(vert.y_frac))) / 255.0f;
        }
        
        float x = static_cast<float>(vert.x_whole) + (vert.x_frac / 256.0f);
        float y = static_cast<float>(vert.y_whole) + (vert.y_frac / 256.0f);
        return Vector2(x, y) / 256.0f;
    }

    uint32_t LevelTextureStorage::tile(uint32_t texture_index) const
    {
        if (texture_index < _object_textures.size())
        {
            return _object_textures[texture_index].TileAndFlag & 0x7FFF;
        }
        return 0;
    }

    uint32_t LevelTextureStorage::num_textures() const
    {
        return _texture_storage->num_textures();
    }

    uint32_t LevelTextureStorage::num_tiles() const
    {
        return _texture_storage->num_textures();
    }

    uint16_t LevelTextureStorage::attribute(uint32_t texture_index) const
    {
        if (texture_index < _object_textures.size())
        {
            return _object_textures[texture_index].Attribute;
        }
        return 0;
    }

    DirectX::SimpleMath::Color LevelTextureStorage::palette_from_texture(uint32_t texture) const
    {
        if (_platform_and_version.version > trlevel::LevelVersion::Tomb1)
        {
            return _palette[texture >> 8];
        }

        if (_platform_and_version.platform == trlevel::Platform::PSX)
        {
            if (auto level = _level.lock())
            {
                const auto colour = level->get_palette_entry(texture);
                return DirectX::SimpleMath::Color(colour.Red / 255.f, colour.Green / 255.f, colour.Blue / 255.f, 1.0f);
            }
            return Colour::Black;
        }

        return _palette[texture & 0xff];
    }

    graphics::Texture LevelTextureStorage::lookup(const std::string&) const
    {
        return graphics::Texture();
    }

    void LevelTextureStorage::store(const std::string&, const graphics::Texture&)
    {
    }

    graphics::Texture LevelTextureStorage::geometry_texture() const
    {
        return _texture_storage->geometry_texture();
    }

    uint32_t LevelTextureStorage::num_object_textures() const
    {
        return static_cast<uint32_t>(_object_textures.size());
    }

    trlevel::PlatformAndVersion LevelTextureStorage::platform_and_version() const
    {
        return _platform_and_version;
    }

    void LevelTextureStorage::load(const std::shared_ptr<trlevel::ILevel>& level)
    {
        _platform_and_version = level->platform_and_version();
        _level = level;

        // Copy object textures locally from the level.
        for (uint32_t i = 0; i < level->num_object_textures(); ++i)
        {
            _object_textures.push_back(level->get_object_texture(i));
        }

        if (_platform_and_version.version < trlevel::LevelVersion::Tomb4)
        {
            using namespace DirectX::SimpleMath;
            for (uint32_t i = 0; i < 256; ++i)
            {
                auto entry = level->get_palette_entry(i);
                _palette[i] = Color(entry.Red / 255.f, entry.Green / 255.f, entry.Blue / 255.f, 1.0f);
            }
        }

        determine_texture_mode();
    }

    void LevelTextureStorage::add_textile(const std::vector<uint32_t>& textile)
    {
        _texture_storage->add_texture(textile, 256, 256);
        auto opaque = textile;
        for (auto& d : opaque)
        {
            d |= 0xff000000;
        }
        _opaque_tiles.emplace_back(*_device, 256, 256, opaque);
    }
}