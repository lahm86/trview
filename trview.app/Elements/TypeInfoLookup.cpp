#include "TypeInfoLookup.h"
#include <trview.common/Json.h>
#include "IItem.h"

using namespace trlevel;

namespace trview
{
    namespace
    {
        const std::unordered_map<int, std::string> mutant_names
        {
            { 20, "Winged" },
            { 21, "Shooter" },
            { 22, "Grounded" },
            { 23, "Centaur" },
            { 34, "Torso" }
        };

        std::string mutant_name(uint16_t flags)
        {
            auto mutant_name = mutant_names.find(mutant_egg_contents((flags & 0x3E00) >> 9));
            return std::format("Mutant Egg ({})", mutant_name == mutant_names.end() ? "Winged" : mutant_name->second);
        }

        constexpr std::string game_name(trlevel::PlatformAndVersion version)
        {
            switch (version.version)
            {
            case trlevel::LevelVersion::Tomb1:
                return (is_tr1_version_26(version) || is_tr1_version_27(version)) ? "tr1_version_26" :
                       is_tr1_version_21(version) ? "tr1_version_21" :
                       is_tr1_may_1996(version) ? "tr1_may_1996" :
                       version.is_tr2_saturn ? "tr2_saturn" : "tr1";
                break;
            case trlevel::LevelVersion::Tomb2:
                return is_tr2_version_44(version) ? "tr2_beta" :
                       is_tr2_e3(version) ? "tr2_e3" : "tr2";
                break;
            case trlevel::LevelVersion::Tomb3:
                return is_tr3_ects(version) ? "tr3_ects" :
                    is_tr3_demo_55(version) ? "tr3_demo_55" : "tr3";
                break;
            case trlevel::LevelVersion::Tomb4:
                return is_tr4_opsm_90(version) ? "tr4_opsm90" : "tr4";
                break;
            case trlevel::LevelVersion::Tomb5:
                return version.raw_version == -224 ? "tr5_version_224" : "tr5";
                break;
            default:
                return {};
            }
        }
    }

    ITypeInfoLookup::~ITypeInfoLookup()
    {
    }

    TypeInfoLookup::TypeInfoLookup(const std::string& type_name_json, const std::optional<std::string>& extra_type_name_json)
    {
        auto parse_json = [&](const auto& data)
            {
                auto json = nlohmann::json::parse(data.begin(), data.end());
                for (const auto& [key, value] : json["games"].items())
                {
                    auto& type_names = _type_names[key];
                    for (const auto& element : value)
                    {
                        auto name = element.at("name").get<std::string>();
                        type_names[element.at("id").get<uint32_t>()] = 
                            {
                                name,
                                read_attribute<std::unordered_set<std::string>>(element, "categories")
                            };
                    }
                }
            };

        parse_json(type_name_json);
        parse_json(extra_type_name_json.value_or("{}"));
    }

    TypeInfo TypeInfoLookup::lookup(trlevel::PlatformAndVersion level_version, uint32_t type_id, int16_t flags) const
    {
        const auto& game_types = _type_names.find(game_name(level_version));
        if (game_types == _type_names.end())
        {
            return { .name = std::to_string(type_id) };
        }

        const auto found_type = game_types->second.find(type_id);
        if (found_type == game_types->second.end())
        {
            return { .name = std::to_string(type_id) };
        }

        TypeInfo result = found_type->second;
        if (level_version.version == LevelVersion::Tomb1 && is_mutant_egg(type_id))
        {
            result.name = mutant_name(flags);
        }
        return result;
    }
}
