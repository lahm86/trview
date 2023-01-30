#pragma once

#include "../../../Elements/Light.h"

struct lua_State;

namespace trview
{
    namespace lua
    {
        void create_light(lua_State* L, const std::shared_ptr<ILight>& light);
    }
}