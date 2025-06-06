#include "Level.h"
#include "LevelLoadException.h"
#include "LevelEncryptedException.h"
#include <format>
#include <ranges>
#include <spanstream>
#include <numeric>

#include "Level_common.h"
#include "Level_psx.h"
#include "Level_tr3.h"
#include <trview.common/Version.h>

namespace trlevel
{
    ILevel::~ILevel()
    {
    }

    void ILevel::LoadCallbacks::on_progress(const std::string& message) const
    {
        if (on_progress_callback)
        {
            on_progress_callback(message);
        }
    }

    void ILevel::LoadCallbacks::on_textile(const std::vector<uint32_t>& data) const
    {
        if (on_textile_callback)
        {
            on_textile_callback(data);
        }
    }

    void ILevel::LoadCallbacks::on_sound(uint16_t sound_map, uint16_t sound_details, uint16_t sample_index, const std::vector<uint8_t>& data) const
    {
        if (on_sound_callback)
        {
            on_sound_callback(sound_map, sound_details, sample_index, data);
        }
    }

    namespace
    {
        const float PiMul2 = 6.283185307179586476925286766559f;
        const int16_t Lara = 0;
        const int16_t LaraSkinTR3 = 315;
        const int16_t LaraSkinTR3Demo55 = 275;
        const int16_t LaraSkinPostTR3 = 8;
        const int16_t LaraSkinTR4OPSM90 = 10;
    
        bool is_tr5(trview::Activity& activity, LevelVersion version, const std::wstring& filename)
        {
            if (version != LevelVersion::Tomb4)
            {
                return false;
            }

            activity.log("Checking file extension to determine whether this is a Tomb5 level");
            std::wstring transformed;
            std::transform(filename.begin(), filename.end(), std::back_inserter(transformed), towupper);
            return transformed.find(L".TRC") != filename.npos;
        }

        std::optional<PlatformAndVersion>  check_for_tr1_psx_without_sound(std::basic_ispanstream<uint8_t>& file)
        {
            // TR1 PSX sometimes has sound separated
            file.seekg(sizeof(tr_textile4) * 13 + sizeof(tr_clut) * 1024);
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 32)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb1, .raw_version = 32 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr1_psx(std::basic_ispanstream<uint8_t>& file)
        {
            skip(file, read<uint32_t>(file));
            skip(file, read<uint32_t>(file));
            skip(file, sizeof(tr_textile4) * 13 + sizeof(tr_clut) * 1024);
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 32)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb1, .raw_version = 32 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr2_psx(std::basic_ispanstream<uint8_t>& file)
        {
            // TR2 PSX has sound data before the version number - attempt to read
            // sound data and see if we end up at a TR2 version number.
            skip(file, read<uint32_t>(file) * sizeof(uint32_t));
            skip(file, read<uint32_t>(file));
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 45)
            {
                return PlatformAndVersion { .platform = Platform::PSX, .version = LevelVersion::Tomb2, .raw_version = 45 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr2_psx_version_44(std::basic_ispanstream<uint8_t>& file)
        {
            // TR2 beta has TR1 style sounds first.
            skip(file, read<uint32_t>(file));
            skip(file, read<uint32_t>(file));
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 44)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb2, .raw_version = 44 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr2_version_42(std::basic_ispanstream<uint8_t>& file)
        {
            // TR2 42 has sounds, style sounds then 18 textiles + cluts;
            skip(file, read<uint32_t>(file));
            skip(file, read<uint32_t>(file));
            skip(file, sizeof(tr_textile4) * 18);
            skip(file, sizeof(tr_clut) * 2048);
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 42)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb2, .raw_version = 42 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr2_psx_version_38(std::basic_ispanstream<uint8_t>& file)
        {
            // TR2 38 has TR1 style sounds then 14 textiles + cluts.
            skip(file, read<uint32_t>(file));
            skip(file, read<uint32_t>(file));
            skip(file, sizeof(tr_textile4) * 14);
            skip(file, sizeof(tr_clut) * 1024);
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 38)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb2, .raw_version = 38 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr1_psx_version_27(std::basic_ispanstream<uint8_t>& file)
        {
            // TR1 PSX August 1996 has textiles first.
            file.seekg(sizeof(tr_textile4) * 15 + sizeof(tr_clut) * 1024);
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 27)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb1, .raw_version = 27 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr1_may_1996(std::basic_ispanstream<uint8_t>& file)
        {
            // TR1 PSX May 1996 has textiles first.
            file.seekg(sizeof(tr_textile4) * 21 + sizeof(tr_clut) * 1024);
            const uint32_t potential_version = read<uint32_t>(file);
            if (potential_version == 11)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb1, .raw_version = 11 };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr1_saturn(std::basic_ispanstream<uint8_t>& file)
        {
            std::array<uint8_t, 8> data;
            file.read(&data[0], 8);
            if ((data | std::ranges::to<std::string>()) == "ROOMFILE")
            {
                skip(file, 4);
                const int32_t version = std::byteswap(read<int32_t>(file));
                return PlatformAndVersion{ .platform = Platform::Saturn, .version = LevelVersion::Tomb1, .raw_version = version };
            }
            return std::nullopt;
        }

        std::optional<PlatformAndVersion> check_for_tr5_psx(std::basic_ispanstream<uint8_t>& file)
        {
            file.seekg(313344);
            const int32_t potential_version = read<int32_t>(file);
            if (potential_version == -225 || potential_version == -224)
            {
                return PlatformAndVersion{ .platform = Platform::PSX, .version = LevelVersion::Tomb5, .raw_version = potential_version };
            }
            return std::nullopt;
        }

        class FileResetter final
        {
        public:
            FileResetter(std::basic_ispanstream<uint8_t>& file)
                : _file(file)
            {
                _file.seekg(0, std::ios::beg);
            }

            ~FileResetter()
            {
                _file.clear();
                _file.seekg(0, std::ios::beg);
            }
        private:
            std::basic_ispanstream<uint8_t>& _file;
        };

        PlatformAndVersion detect_level_version(std::basic_ispanstream<uint8_t>& file)
        {
            const std::vector<std::function<std::optional<PlatformAndVersion>(std::basic_ispanstream<uint8_t>&)>> checks
            {
                check_for_tr2_psx,
                check_for_tr2_psx_version_44,
                check_for_tr2_version_42,
                check_for_tr2_psx_version_38,
                check_for_tr1_psx_version_27,
                check_for_tr1_may_1996,
                check_for_tr1_psx_without_sound,
                check_for_tr1_psx,
                check_for_tr5_psx,
                check_for_tr1_saturn
            };

            for (const auto& check : checks)
            {
                const FileResetter resetter(file);
                try
                {
                    if (const auto version = check(file))
                    {
                        return version.value();
                    }
                }
                catch(...)
                {
                }
            }
            return {};
        }

        bool has_frame_count(PlatformAndVersion version)
        {
            return (version.version == LevelVersion::Tomb1 && !version.is_tr2_saturn) || is_tr2_version_42(version) || is_tr2_e3(version) || is_tr2_version_38(version);
        }

        bool is_tr1_frame_format(PlatformAndVersion version)
        {
            return
                (version.version == LevelVersion::Tomb1 && version.platform != Platform::Saturn)
                || is_tr2_version_42(version) || is_tr2_e3(version) || is_tr2_version_38(version);
        }
    }

    Level::Level(const std::string& filename, const std::shared_ptr<IPack>& pack, const std::shared_ptr<trview::IFiles>& files, const std::shared_ptr<IDecrypter>& decrypter, const std::shared_ptr<trview::ILog>& log, const IPack::Source& pack_source)
        : _log(log), _decrypter(decrypter), _filename(filename), _files(files), _pack_source(pack_source), _pack(pack)
    {
    }

    Level::Level(const std::string& filename, const std::shared_ptr<IPack>& pack, const std::shared_ptr<trview::IFiles>& files, const std::shared_ptr<IDecrypter>& decrypter, const std::shared_ptr<trview::ILog>& log)
        : _log(log), _decrypter(decrypter), _filename(filename), _files(files), _pack(pack)
    {
    }

    Level::~Level()
    {
    }

    void Level::generate_meshes(const std::vector<uint16_t>& mesh_data)
    {
        if (_mesh_data.empty())
        {
            return;
        }

        // As well as reading the actual mesh data, generate a map of mesh_pointer to 
        // mesh. It seems that a lot of the pointers point to the same mesh.
        std::span span{ reinterpret_cast<const uint8_t*>(&mesh_data[0]), mesh_data.size() * sizeof(uint16_t) };
        std::basic_ispanstream<uint8_t> stream{ span };
        stream.exceptions(std::istream::failbit | std::istream::badbit | std::istream::eofbit);
        for (auto pointer : _mesh_pointers)
        {
            // Does the map already contain this mesh? If so, don't bother reading it again.
            auto found = _meshes.find(pointer);
            if (found != _meshes.end())
            {
                continue;
            }

            stream.seekg(pointer, std::ios::beg);

            tr_mesh mesh;

            if (_platform_and_version.platform == Platform::PSX)
            {
                if (_platform_and_version.version == LevelVersion::Tomb1)
                {
                    if (is_tr1_may_1996(_platform_and_version))
                    {
                        generate_mesh_tr1_psx_may_1996(mesh, stream);
                    }
                    else
                    {
                        generate_mesh_tr1_psx(mesh, stream);
                    }
                }
                else if (_platform_and_version.version == LevelVersion::Tomb2)
                {
                    if (is_tr2_version_44(_platform_and_version) || 
                        is_tr2_version_42(_platform_and_version))
                    {
                        generate_mesh_tr2_psx_version_44(mesh, stream);
                    }
                    else if (is_tr2_version_38(_platform_and_version))
                    {
                        generate_mesh_tr2_psx_version_38(mesh, stream);
                    }
                    else
                    {
                        generate_mesh_tr2_psx(mesh, stream);
                    }
                }
                else if (_platform_and_version.version == LevelVersion::Tomb3)
                {
                    generate_mesh_tr3_psx(mesh, stream);
                }
                else if (_platform_and_version.version == LevelVersion::Tomb4 ||
                         _platform_and_version.version == LevelVersion::Tomb5)
                {
                    generate_mesh_tr4_psx(mesh, stream);
                }
            }
            else if (_platform_and_version.platform == Platform::Saturn)
            {
                generate_mesh_tr1_saturn(mesh, stream);
            }
            else if (is_tr1_may_1996(_platform_and_version))
            {
                generate_mesh_tr1_pc_may_1996(mesh, stream);
            }
            else if (is_tr1_version_21(_platform_and_version))
            {
                generate_mesh_tr1_pc_version_21(mesh, stream);
            }
            else
            {
                generate_mesh(mesh, stream);
            }

            _meshes.insert({ pointer, mesh });
        }
    }

    tr_colour Level::get_palette_entry8(uint32_t index) const
    {
        return index < _palette.size() ? _palette[index] : tr_colour { .Red = 0, .Green = 0, .Blue = 0 };
    }

    tr_colour4 Level::get_palette_entry_16(uint32_t index) const
    {
        return _palette16[index];
    }

    tr_colour4 Level::get_palette_entry(uint32_t index) const
    {
        if (_platform_and_version.platform == Platform::PSX &&
            _platform_and_version.version == LevelVersion::Tomb1)
        {
            return colour_from_object_texture(index);
        }

        if (index < _palette16.size())
        {
            return get_palette_entry_16(index);
        }
        return convert_to_colour4(get_palette_entry8(index));
    }

    tr_colour4 Level::get_palette_entry(uint32_t index8, uint32_t index16) const
    {
        if (index16 < _palette16.size())
        {
            return get_palette_entry_16(index16);
        }
        return convert_to_colour4(get_palette_entry8(index8));
    }

    uint32_t Level::num_rooms() const
    {
        return static_cast<uint32_t>(_rooms.size());
    }

    tr3_room Level::get_room(uint32_t index) const
    {
        return _rooms[index];
    }

    uint32_t Level::num_object_textures() const
    {
        return static_cast<uint32_t>(_object_textures.size());
    }

    tr_object_texture Level::get_object_texture(uint32_t index) const
    {
        return _object_textures[index];
    }

    uint32_t Level::num_floor_data() const
    {
        return static_cast<uint32_t>(_floor_data.size());
    }

    uint16_t Level::get_floor_data(uint32_t index) const
    {
        return _floor_data[index];
    }

    std::vector<std::uint16_t> 
    Level::get_floor_data_all() const
    {
        return _floor_data;
    }

    uint32_t Level::num_ai_objects() const
    {
        return static_cast<uint32_t>(_ai_objects.size());
    }

    tr4_ai_object Level::get_ai_object(uint32_t index) const
    {
        return _ai_objects[index];
    }

    uint32_t Level::num_entities() const
    {
        return static_cast<uint32_t>(_entities.size());
    }

    tr2_entity Level::get_entity(uint32_t index) const 
    {
        return _entities[index];
    }

    uint32_t Level::num_models() const
    {
        return static_cast<uint32_t>(_models.size());
    }

    tr_model Level::get_model(uint32_t index) const
    {
        return _models[index];
    }

    bool Level::get_model_by_id(uint32_t id, tr_model& output) const 
    {
        for (const auto& model : _models)
        {
            if (model.ID == id)
            {
                output = model;
                return true;
            }
        }
        return false;
    }

    uint32_t Level::num_static_meshes() const
    {
        return static_cast<uint32_t>(_static_meshes.size());
    }

    std::optional<tr_staticmesh> Level::get_static_mesh(uint32_t mesh_id) const
    {
        auto mesh = _static_meshes.find(mesh_id);
        if (mesh == _static_meshes.end())
        {
            return std::nullopt;
        }
        return mesh->second;
    }

    uint32_t Level::num_mesh_pointers() const
    {
        return static_cast<uint32_t>(_mesh_pointers.size());
    }

    tr_mesh Level::get_mesh_by_pointer(uint32_t mesh_pointer) const
    {
        auto index = _mesh_pointers[mesh_pointer];
        return _meshes.find(index)->second;
    }

    std::vector<tr_meshtree_node> Level::get_meshtree(uint32_t starting_index, uint32_t node_count) const
    {
        uint32_t index = starting_index;
        std::vector<tr_meshtree_node> nodes;
        for (uint32_t i = 0; i < node_count; ++i)
        {
            tr_meshtree_node node;
            node.Flags = _meshtree[index++];
            node.Offset_X = static_cast<int32_t>(_meshtree[index++]);
            node.Offset_Y = static_cast<int32_t>(_meshtree[index++]);
            node.Offset_Z = static_cast<int32_t>(_meshtree[index++]);
            nodes.push_back(node);
        }
        return nodes;
    }

    tr2_frame Level::get_frame(uint32_t frame_offset, uint32_t mesh_count) const
    {
        uint32_t offset = frame_offset;
        tr2_frame frame;

        // Some models have a frames reference equal to the frames length. Presumably this
        // means they don't have any frames, so just return default.
        if (offset >= _frames.size())
        {
            return frame;
        }

        frame.bb1x = _frames[offset++];
        frame.bb1y = _frames[offset++];
        frame.bb1z = _frames[offset++];
        frame.bb2x = _frames[offset++];
        frame.bb2y = _frames[offset++];
        frame.bb2z = _frames[offset++];
        frame.offsetx = _frames[offset++];
        frame.offsety = _frames[offset++];
        frame.offsetz = _frames[offset++];

        // Tomb Raider I has the mesh count in the frame structure - all other tombs
        // already know based on the number of meshes.
        if (has_frame_count(_platform_and_version))
        {
            mesh_count = _frames[offset++];
        }

        for (uint32_t i = 0; i < mesh_count; ++i)
        {
            tr2_frame_rotation rotation;

            uint16_t next = 0;
            uint16_t data = 0;
            uint16_t mode = 0;

            // Tomb Raider I has reversed words and always uses the two word format.
            if (is_tr1_frame_format(_platform_and_version))
            {
                next = _frames[offset++];
                data = _frames[offset++];
            }
            else
            {
                data = _frames[offset++];
                mode = data & 0xC000;
                if (!mode)
                {
                    next = _frames[offset++];
                }
            }

            if (mode)
            {
                float angle = 0;
                if (get_version() >= LevelVersion::Tomb4)
                {
                    angle = (data & 0x0fff) * PiMul2 / 4096.0f;
                }
                else
                {
                    angle = (data & 0x03ff) * PiMul2 / 1024.0f;
                }

                if (mode == 0x4000)
                {
                    rotation.x = angle;
                }
                else if (mode == 0x8000)
                {
                    rotation.y = angle;
                }
                else if (mode == 0xC000)
                {
                    rotation.z = angle;
                }
            }
            else
            {
                rotation.x = ((data & 0x3ff0) >> 4) * PiMul2 / 1024.0f;
                rotation.y = ((((data & 0x000f) << 6)) | ((next & 0xfc00) >> 10)) * PiMul2 / 1024.0f;
                rotation.z = (next & 0x03ff) * PiMul2 / 1024.0f;
            }
            frame.values.push_back(rotation);
        }
        return frame;
    }

    LevelVersion Level::get_version() const 
    {
        return _platform_and_version.version;
    }

    bool Level::get_sprite_sequence_by_id(int32_t sprite_sequence_id, tr_sprite_sequence& output) const
    {
        auto found_sequence = std::find_if(_sprite_sequences.begin(), _sprite_sequences.end(), [=](const auto& sequence)
        {
            return sequence.SpriteID == sprite_sequence_id; 
        });

        if (found_sequence == _sprite_sequences.end())
        {
            return false;
        }

        output = *found_sequence;
        return true;
    }

    std::optional<tr_sprite_texture> Level::get_sprite_texture(uint32_t index) const
    {
        if (index < _sprite_textures.size())
        {
            return _sprite_textures[index];
        }
        return std::nullopt;
    }

    bool Level::find_first_entity_by_type(int16_t type, tr2_entity& entity) const
    {
        auto found = std::find_if(_entities.begin(), _entities.end(), [type](const auto& e) { return e.TypeID == type; });
        if (found == _entities.end())
        {
            return false;
        }
        entity = *found;
        return true;
    }

    std::string Level::name() const
    {
        return _name;
    }

    uint32_t Level::num_cameras() const
    {
        return static_cast<uint32_t>(_cameras.size());
    }

    tr_camera Level::get_camera(uint32_t index) const
    {
        return _cameras[index];
    }

    Platform Level::platform() const
    {
        return _platform_and_version.platform;
    }

    void Level::load(const LoadCallbacks& callbacks)
    {
        // Clear the log before loading the level so we don't keep accumulating memory.
        _log->clear();

        // Load the level from the file.
        _name = trview::filename_without_path(_filename);

        trview::Activity activity(_log, "IO", "Load Level " + _name);

        try
        {
            activity.log(std::format("trview version {}", trview::version()));
            activity.log(std::format("Opening file \"{}\"", _filename));

            const bool is_packed = _filename.starts_with("pack") && _pack;
            const bool is_pack_preview = _filename.starts_with("pack-preview") && _pack;
            auto bytes = is_packed ? pack_entry(*_pack, std::stoi(_name)) : _files->load_file(_filename);
            if (!bytes.has_value())
            {
                throw LevelLoadException();
            }

            const auto& bytes_value = *bytes;
            std::basic_ispanstream<uint8_t> file{ { bytes_value } };
            file.exceptions(std::ios::failbit);
            log_file(activity, file, std::format("Opened file \"{}\"", _filename));

            read_header(file, *bytes, activity, callbacks);

            if (is_pack_preview)
            {
                return;
            }

            // Don't attempt to load nested packs if a packed file is detected as a pack.
            if (is_packed)
            {
                _platform_and_version.is_pack = false;
            }

            const std::unordered_map<PlatformAndVersion, std::function<void()>> loaders
            {
                {{.platform = Platform::PSX, .version = LevelVersion::Tomb1 }, [&]() { load_tr1_psx(file, activity, callbacks); }},
                {{.platform = Platform::PSX, .version = LevelVersion::Tomb2 }, [&]() { load_tr2_psx(file, activity, callbacks); }},
                {{.platform = Platform::PSX, .version = LevelVersion::Tomb3 }, [&]() { load_tr3_psx(file, activity, callbacks); }},
                {{.platform = Platform::PSX, .version = LevelVersion::Tomb4 }, [&]() { load_tr4_psx(file, activity, callbacks); }},
                {{.platform = Platform::PSX, .version = LevelVersion::Tomb5 }, [&]() { load_tr5_psx(file, activity, callbacks); }},
                {{.platform = Platform::PSX, .version = LevelVersion::Unknown, .is_pack = true }, [&]() { load_psx_pack(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb1 }, [&]() { load_tr1_pc(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb2 }, [&]() { load_tr2_pc(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb3 }, [&]() { load_tr3_pc(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb4 }, [&]() { load_tr4_pc(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb4, .remastered = true }, [&]() { load_tr4_pc_remastered(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb5 }, [&]() { load_tr5_pc(file, activity, callbacks); }},
                {{.platform = Platform::PC, .version = LevelVersion::Tomb5, .remastered = true }, [&]() { load_tr5_pc_remastered(file, activity, callbacks); }},
                {{.platform = Platform::Dreamcast, .version = LevelVersion::Tomb5 }, [&]() { load_tr5_dc(file, activity, callbacks); }},
                {{.platform = Platform::Saturn, .version = LevelVersion::Tomb1 }, [&]() { load_tr1_saturn(file, activity, callbacks); }},
            };

            const auto loader = loaders.find(_platform_and_version);
            if (loader != loaders.end())
            {
                loader->second();
                callbacks.on_progress("Loading complete");
                return;
            }

            throw std::exception(std::format("Unsupported level platform and version ({}:{}{})",
                to_string(_platform_and_version.platform),
                to_string(_platform_and_version.version),
                _platform_and_version.remastered ? " (Remastered)" : "").c_str());
        }
        catch (const LevelEncryptedException&)
        {
            activity.log(trview::Message::Status::Error, "Level is encrypted, aborting");
            throw;
        }
        catch (const std::exception& e)
        {   
            activity.log(trview::Message::Status::Error, std::format("Level failed to load: {}", e.what()));
            throw LevelLoadException(e.what());
        }
    }

    std::vector<tr_sound_source> Level::sound_sources() const
    {
        return _sound_sources;
    }

    std::vector<tr_x_sound_details> Level::sound_details() const
    {
        return _sound_details;
    }

    std::vector<int16_t> Level::sound_map() const
    {
        return _sound_map;
    }

    void Level::read_header(std::basic_ispanstream<uint8_t>& file, std::vector<uint8_t>& bytes, trview::Activity& activity, const LoadCallbacks& callbacks)
    {
        log_file(activity, file, "Reading version number from file");
        uint32_t raw_version = read<uint32_t>(file);

        // For levels where the version number is not the first bytes check the
        // appropriate location in the file.
        _platform_and_version = detect_level_version(file);

        // If specific version checks failed we can check the first bytes.
        if (_platform_and_version.version == LevelVersion::Unknown)
        {
            _platform_and_version = convert_level_version(raw_version);
        }

        log_file(activity, file, std::format("Version number is {} ({}), Platform is {}", _platform_and_version.raw_version, to_string(get_version()), to_string(platform())));
        if (_platform_and_version.version == LevelVersion::Unknown)
        {
            if (!_platform_and_version.is_pack)
            {
                throw LevelLoadException(std::format("Unknown level version ({})", _platform_and_version.raw_version));
            }
            log_file(activity, file, std::format("Version number is {} ({}), Platform is {}", _platform_and_version.raw_version, to_string(get_version()), to_string(platform())));
        }

        if (_platform_and_version.raw_version == 0x63345254)
        {
            callbacks.on_progress("Decrypting");
            log_file(activity, file, std::format("File is encrypted, decrypting"));
            _decrypter->decrypt(bytes);
            file.seekg(0, std::ios::beg);
            _platform_and_version = convert_level_version(read<uint32_t>(file));
            log_file(activity, file, std::format("Version number is {:X} ({})", _platform_and_version.raw_version, to_string(get_version())));
        }

        if (is_tr5(activity, get_version(), trview::to_utf16(_filename)))
        {
            _platform_and_version.version = LevelVersion::Tomb5;
            log_file(activity, file, std::format("Version number is {:X} ({})", _platform_and_version.raw_version, to_string(get_version())));
        }
    }

    void Level::read_palette_tr2_3(std::basic_ispanstream<uint8_t>& file, trview::Activity& activity, const LoadCallbacks& callbacks)
    {
        read_palette_tr1(file, activity, callbacks);
        callbacks.on_progress("Reading 16-bit palette");
        log_file(activity, file, "Reading 16-bit palette");
        _palette16 = read_vector<tr_colour4>(file, 256);
        log_file(activity, file, "Read 16-bit palette");
    }

    void Level::load_sound_fx(trview::Activity& activity, const LoadCallbacks& callbacks)
    {
        if (const auto main = load_main_sfx())
        {
            std::basic_ispanstream<uint8_t> sfx_file{ { *main } };
            sfx_file.exceptions(std::ios::failbit | std::ios::badbit | std::ios::eofbit);

            // Remastered has a sound map like structure at the start of main.sfx, so skip that if present:
            if (_platform_and_version.remastered)
            {
                _sound_map = read_sound_map(activity, sfx_file, callbacks);
                _sound_details = read_sound_details(activity, sfx_file, callbacks);
                _sample_indices = std::ranges::iota_view{ 0u, _sound_map.size() } | std::ranges::to<std::vector>();
                skip(sfx_file, 4000);
            }
            else
            {
                if (read<uint32_t>(sfx_file) != 0x46464952) // RIFF
                {
                    sfx_file.seekg(_sound_map.size() * 2, std::ios::beg);
                }
                else
                {
                    sfx_file.seekg(0, std::ios::beg);
                }
            }

            int16_t overall_index = 0;
            while (static_cast<std::size_t>(sfx_file.tellg()) < main->size())
            {
                skip(sfx_file, 4);
                uint32_t size = read<uint32_t>(sfx_file);
                sfx_file.seekg(-8, std::ios::cur);
                if (std::ranges::find(_sample_indices, static_cast<uint32_t>(overall_index)) != _sample_indices.end())
                {
                    _sound_samples.push_back({ main->begin() + sfx_file.tellg(), main->begin() + sfx_file.tellg() + size + 8 });
                }
                overall_index++;
                sfx_file.seekg(size + 8, std::ios::cur);
            }
        }
    }

    std::optional<std::vector<uint8_t>> Level::load_main_sfx() const
    {
        const auto path = trview::path_for_filename(_filename);
        const auto og_main = _files->load_file(std::format("{}/MAIN.SFX", path));
        if (og_main.has_value())
        {
            return og_main;
        }
        return _files->load_file(std::format("{}/../SFX/MAIN.SFX", path));
    }

    bool Level::trng() const
    {
        return _trng;
    }

    void Level::generate_mesh(tr_mesh& mesh, std::basic_ispanstream<uint8_t>& stream)
    {
        mesh.centre = read<tr_vertex>(stream);
        mesh.coll_radius = read<int32_t>(stream);
        mesh.vertices = read_vector<int16_t, tr_vertex>(stream);

        int16_t normals = read<int16_t>(stream);
        if (normals > 0)
        {
            mesh.normals = read_vector<tr_vertex>(stream, normals);
        }
        else
        {
            mesh.lights = read_vector<int16_t>(stream, abs(normals));
        }

        if (get_version() < LevelVersion::Tomb4)
        {
            mesh.textured_rectangles = convert_rectangles(read_vector<int16_t, tr_face4>(stream));
            mesh.textured_triangles = convert_triangles(read_vector<int16_t, tr_face3>(stream));
            mesh.coloured_rectangles = read_vector<int16_t, tr_face4>(stream);
            mesh.coloured_triangles = read_vector<int16_t, tr_face3>(stream);
        }
        else
        {
            mesh.textured_rectangles = read_vector<int16_t, tr4_mesh_face4>(stream);
            mesh.textured_triangles = read_vector<int16_t, tr4_mesh_face3>(stream);
        }
    }

    void Level::read_sprite_textures_psx(std::basic_ispanstream<uint8_t>& file, trview::Activity& activity, const LoadCallbacks& callbacks)
    {
        callbacks.on_progress("Reading sprite textures");
        log_file(activity, file, "Reading sprite textures");
        auto textures = read_vector<uint32_t, tr_sprite_texture_psx>(file);
        _sprite_textures = textures
            | std::views::transform([&](const auto texture) -> tr_sprite_texture
                {
                    const uint16_t tile = convert_textile4(texture.Tile, texture.Clut);
                    const uint16_t width = (texture.u1 - texture.u0) * 256 + 255;
                    const uint16_t height = (texture.v1 - texture.v0) * 256 + 255;
                    return { tile, texture.u0, texture.v0, width, height, texture.LeftSide, texture.TopSide, texture.RightSide, texture.BottomSide };
                })
            | std::ranges::to<std::vector>();
        log_file(activity, file, std::format("Read {} sprite textures", _sprite_textures.size()));
    }

    void Level::adjust_room_textures_psx()
    {
        for (auto& room : _rooms)
        {
            for (auto& face : room.data.rectangles)
            {
                face.texture += static_cast<uint16_t>(_object_textures_psx.size());
            }

            for (auto& face : room.data.triangles)
            {
                face.texture += static_cast<uint16_t>(_object_textures_psx.size());
            }
        }
    }

    void Level::adjust_room_textures()
    {
        for (auto& room : _rooms)
        {
            for (auto& face : room.data.rectangles)
            {
                face.texture += static_cast<uint16_t>(_object_textures.size());
            }

            for (auto& face : room.data.triangles)
            {
                face.texture += static_cast<uint16_t>(_object_textures.size());
            }
        }
    }

    PlatformAndVersion Level::platform_and_version() const
    {
        return _platform_and_version;
    }

    uint16_t Level::attribute_for_clut(uint16_t clut_id) const
    {
        if (clut_id >= _clut.size())
        {
            return 0;
        }
        return std::ranges::any_of(_clut[clut_id].Colour, [](auto&& c) { return c.Red == 0 && c.Green == 0 && c.Blue == 0; }) ? 1 : 0;
    }

    std::weak_ptr<IPack> Level::pack() const
    {
        return _pack;
    }

    void Level::generate_sounds(const LoadCallbacks& callbacks)
    {
        callbacks.on_progress("Generating sounds");
        for (auto sound_map_index = 0; sound_map_index < _sound_map.size(); ++sound_map_index)
        {
            const int16_t sound_details_index = _sound_map[sound_map_index];
            if (sound_details_index == -1)
            {
                continue;
            }

            const auto sound_detail = _sound_details[sound_details_index];
            const auto sample_count = ((_platform_and_version.version > LevelVersion::Tomb1 ? sound_detail.tr3_sound_details.Characteristics : sound_detail.tr_sound_details.Characteristics) >> 2) & 0xf;
            for (int s = 0; s < sample_count; ++s)
            {
                const uint16_t sample_index = static_cast<uint16_t>(sound_detail.tr_sound_details.Sample + s);
                if (sample_index < _sound_samples.size())
                {
                    callbacks.on_sound(static_cast<uint16_t>(sound_map_index), sound_details_index, sample_index, { _sound_samples[sample_index] });
                }
            }
        }

        _sound_data = {};
        _sound_samples = {};
    }
}
