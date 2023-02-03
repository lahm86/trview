#include "Lua_Room.h"
#include "../../../Elements/ILevel.h"
#include "../Item/Lua_Item.h"
#include "../Trigger/Lua_Trigger.h"
#include "../Sector/Lua_Sector.h"
#include "../CameraSink/Lua_CameraSink.h"
#include "../Light/Lua_Light.h"
#include "../../Lua.h"

namespace trview
{
    namespace lua
    {
        namespace
        {
            std::unordered_map<IRoom**, std::shared_ptr<IRoom>> rooms;

            int room_index(lua_State* L)
            {
                auto room = lua::get_self<IRoom>(L);

                const std::string key = lua_tostring(L, 2);
                if (key == "alternate_mode")
                {
                    lua_pushstring(L, to_string(room->alternate_mode()).c_str());
                    return 1;
                }
                else if (key == "alternate_group")
                {
                    lua_pushinteger(L, room->alternate_group());
                    return 1;
                }
                else if (key == "alternate_room")
                {
                    if (auto level = room->level().lock())
                    {
                        return create_room(L, level->room(room->alternate_room()).lock());
                    }
                    lua_pushnil(L);
                    return 1;
                }
                else if (key == "cameras_and_sinks")
                {
                    return push_list(L, room->camera_sinks(), { create_camera_sink });
                }
                else if (key == "items")
                {
                    return push_list(L, room->items(), { create_item });
                }
                else if (key == "lights")
                {
                    return push_list(L, room->lights(), { create_light });
                }
                else if (key == "number")
                {
                    lua_pushinteger(L, room->number());
                    return 1;
                }
                else if (key == "sectors")
                {
                    return push_list(L, room->sectors(), { create_sector });
                }
                else if (key == "triggers")
                {
                    return push_list(L, room->triggers(), { create_trigger });
                }
                else if (key == "visible")
                {
                    lua_pushboolean(L, room->visible());
                    return 1;
                }

                return 0;
            }

            int room_newindex(lua_State* L)
            {
                auto room = lua::get_self<IRoom>(L);

                const std::string key = lua_tostring(L, 2);
                if (key == "visible")
                {
                    if (auto level = room->level().lock())
                    {
                        level->set_room_visibility(room->number(), lua_toboolean(L, -1));
                    }
                }

                return 0;
            }

            int room_gc(lua_State*L)
            {
                luaL_checktype(L, 1, LUA_TUSERDATA);
                IRoom** userdata = static_cast<IRoom**>(lua_touserdata(L, 1));
                rooms.erase(userdata);
                return 0;
            }
        }

        int create_room(lua_State* L, std::shared_ptr<IRoom> room)
        {
            if (!room)
            {
                lua_pushnil(L);
                return 1;
            }

            IRoom** userdata = static_cast<IRoom**>(lua_newuserdata(L, sizeof(room.get())));
            *userdata = room.get();
            rooms[userdata] = room;

            lua_newtable(L);
            lua_pushcfunction(L, room_index);
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, room_newindex);
            lua_setfield(L, -2, "__newindex");
            lua_pushcfunction(L, room_gc);
            lua_setfield(L, -2, "__gc");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
}
