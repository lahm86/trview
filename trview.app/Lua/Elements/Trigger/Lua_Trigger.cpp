#include "Lua_Trigger.h"
#include "../../Lua.h"
#include "../../../Elements/ITrigger.h"
#include "../../../Elements/ILevel.h"
#include "../../Vector3.h"
#include "../Room/Lua_Room.h"
#include "../Sector/Lua_Sector.h"

namespace trview
{
    namespace lua
    {
        namespace
        {
            std::unordered_map<ITrigger**, std::shared_ptr<ITrigger>> triggers;

            void create_command(lua_State* L, const Command& command)
            {
                lua_newtable(L);
                lua_pushinteger(L, command.number());
                lua_setfield(L, -2, "number");
                lua_pushinteger(L, command.index());
                lua_setfield(L, -2, "index");
                lua_pushstring(L, command_type_name(command.type()).c_str());
                lua_setfield(L, -2, "type");
            }

            int trigger_index(lua_State* L)
            {
                auto trigger = lua::get_self<ITrigger>(L);

                const std::string key = lua_tostring(L, 2);
                if (key == "commands")
                {
                    return push_list(L, trigger->commands(), create_command);
                }
                else if (key == "flags")
                {
                    lua_pushinteger(L, trigger->flags());
                    return 1;
                }
                else if (key == "number")
                {
                    lua_pushinteger(L, trigger->number());
                    return 1;
                }
                else if (key == "only_once")
                {
                    lua_pushboolean(L, trigger->only_once());
                    return 1;
                }
                else if (key == "position")
                {
                    return create_vector3(L, trigger->position() * trlevel::Scale);
                }
                else if (key == "room")
                {
                    if (auto level = trigger->level().lock())
                    {
                        return create_room(L, level->room(trigger->room()).lock());
                    }
                    lua_pushnil(L);
                    return 1;
                }
                else if (key == "sector")
                {
                    if (auto level = trigger->level().lock())
                    {
                        if (auto room = level->room(trigger->room()).lock())
                        {
                            const auto sectors = room->sectors();
                            if (trigger->sector_id() < sectors.size())
                            {
                                return create_sector(L, sectors[trigger->sector_id()]);
                            }
                        }
                    }
                    lua_pushnil(L);
                    return 1;
                }
                else if (key == "timer")
                {
                    lua_pushinteger(L, trigger->timer());
                    return 1;
                }
                else if (key == "type")
                {
                    lua_pushstring(L, trigger_type_name(trigger->type()).c_str());
                    return 1;
                }
                else if (key == "visible")
                {
                    lua_pushboolean(L, trigger->visible());
                    return 1;
                }

                return 0;
            }

            int trigger_newindex(lua_State* L)
            {
                auto trigger = lua::get_self<ITrigger>(L);

                const std::string key = lua_tostring(L, 2);
                if (key == "visible")
                {
                    if (auto level = trigger->level().lock())
                    {
                        level->set_trigger_visibility(trigger->number(), lua_toboolean(L, -1));
                    }
                }

                return 0;
            }

            int trigger_gc(lua_State* L)
            {
                luaL_checktype(L, 1, LUA_TUSERDATA);
                ITrigger** userdata = static_cast<ITrigger**>(lua_touserdata(L, 1));
                triggers.erase(userdata);
                return 0;
            }
        }

        int create_trigger(lua_State* L, const std::shared_ptr<ITrigger>& trigger)
        {
            if (!trigger)
            {
                lua_pushnil(L);
                return 1;
            }

            ITrigger** userdata = static_cast<ITrigger**>(lua_newuserdata(L, sizeof(trigger.get())));
            *userdata = trigger.get();
            triggers[userdata] = trigger;

            lua_newtable(L);
            lua_pushcfunction(L, trigger_index);
            lua_setfield(L, -2, "__index");
            lua_pushcfunction(L, trigger_newindex);
            lua_setfield(L, -2, "__newindex");
            lua_pushcfunction(L, trigger_gc);
            lua_setfield(L, -2, "__gc");
            lua_setmetatable(L, -2);
            return 1;
        }
    }
}