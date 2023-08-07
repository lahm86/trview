#include "Lua_Level.h"
#include "../../Lua.h"
#include "../../../Elements/ILevel.h"
#include "../Room/Lua_Room.h"
#include "../Item/Lua_Item.h"
#include "../CameraSink/Lua_CameraSink.h"
#include "../Trigger/Lua_Trigger.h"
#include "../Light/Lua_Light.h"
#include "../StaticMesh/Lua_StaticMesh.h"

namespace trview
{
    namespace lua
    {
        namespace
        {
            int level_index(lua_State* L)
            {
                auto level = lua::get_self<ILevel>(L);

                const std::string key = lua_tostring(L, 2);
                if (key == "cameras_and_sinks")
                {
                    return push_list_p(L, level->camera_sinks(), create_camera_sink);
                }
                else if (key == "alternate_mode")
                {
                    lua_pushboolean(L, level->alternate_mode());
                    return 1;
                }
                else if (key == "filename")
                {
                    lua_pushstring(L, level->filename().c_str());
                    return 1;
                }
                else if (key == "floordata")
                {
                    lua_newtable(L);
                    // TODO: Floordata
                    return 1;
                }
                else if (key == "items")
                {
                    return push_list_p(L, level->items(), create_item);
                }
                else if (key == "lights")
                {
                    return push_list_p(L, level->lights(), create_light);
                }
                else if (key == "rooms")
                {
                    return push_list_p(L, level->rooms(), create_room);
                }
                else if (key == "selected_item")
                {
                    auto item = level->selected_item();
                    if (item)
                    {
                        return create_item(L, level->item(item.value()).lock());
                    }
                    lua_pushnil(L);
                    return 1;
                }
                else if (key == "selected_room")
                {
                    return create_room(L, level->room(level->selected_room()).lock());
                }
                else if (key == "selected_trigger")
                {
                    auto trigger = level->selected_trigger();
                    if (trigger)
                    {
                        return create_trigger(L, level->trigger(trigger.value()).lock());
                    }
                    lua_pushnil(L);
                    return 1;
                }
                else if (key == "static_meshes")
                {
                    return push_list_p(L, level->static_meshes(), create_static_mesh);
                }
                else if (key == "triggers")
                {
                    return push_list_p(L, level->triggers(), create_trigger);
                }
                else if (key == "version")
                {
                    lua_pushinteger(L, static_cast<int>(level->version()));
                    return 1;
                }
                
                return 0;
            }

            int level_newindex(lua_State* L)
            {
                auto level = lua::get_self<ILevel>(L);

                const std::string key = lua_tostring(L, 2);
                if (key == "alternate_mode")
                {
                    luaL_checktype(L, -1, LUA_TBOOLEAN);
                    level->set_alternate_mode(lua_toboolean(L, -1));
                }
                else if (key == "selected_item")
                {
                    if (auto item = to_item(L, -1))
                    {
                        level->set_selected_item(item->number());
                    }
                }
                else if (key == "selected_room")
                {
                    if (auto room = to_room(L, -1))
                    {
                        level->set_selected_room(static_cast<uint16_t>(room->number()));
                    }
                }
                else if (key == "selected_trigger")
                {
                    if (auto trigger = to_trigger(L, -1))
                    {
                        level->set_selected_trigger(trigger->number());
                    }
                }

                return 0;
            }
        }

        int create_level(lua_State* L, const std::shared_ptr<ILevel>& level)
        {
            return create(L, level, level_index, level_newindex);
        }

        std::shared_ptr<ILevel> to_level(lua_State* L, int index)
        {
            return get_self<ILevel>(L, index);
        }
    }
}
