#pragma once

#include "ITypeInfoLookup.h"
#include <unordered_map>
#include <unordered_set>

namespace trview
{
    class TypeInfoLookup : public ITypeInfoLookup
    {
    public:
        explicit TypeInfoLookup(const std::string& type_name_json);
        virtual ~TypeInfoLookup() = default;
        TypeInfo lookup(trlevel::LevelVersion level_version, uint32_t type_id, int16_t flags) const override;
    private:
        std::unordered_map<trlevel::LevelVersion, std::unordered_map<uint32_t, TypeInfo>> _type_names;
    };
}
