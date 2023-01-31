#pragma once

#include <external/lua/src/lua.h>
#include <external/lua/src/lauxlib.h>
#include "../../Elements/ILevel.h"

namespace trview
{
    struct IApplication;

    namespace lua
    {
        void trview_register(lua_State* L, IApplication* application);
    }
}
