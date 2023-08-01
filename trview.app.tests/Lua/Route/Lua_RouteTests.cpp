#include <trview.app/Lua/Route/Lua_Route.h>
#include <trview.app/Lua/Colour.h>
#include <trview.tests.common/Mocks.h>
#include <external/lua/src/lua.h>
#include <external/lua/src/lauxlib.h>
#include <trview.app/Mocks/Routing/IRoute.h>
#include <trview.app/Mocks/Routing/IRandomizerRoute.h>
#include <trview.app/Mocks/Elements/ILevel.h>
#include <trview.app/Lua/Route/Lua_Waypoint.h>
#include <trview.app/Mocks/Routing/IWaypoint.h>
#include <trview.app/Lua/Elements/Level/Lua_Level.h>
#include <trview.app/Mocks/Routing/IRandomizerRoute.h>

using namespace trview;
using namespace trview::mocks;
using namespace trview::tests;
using namespace testing;

TEST(Lua_Route, Add)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, add(A<const std::shared_ptr<IWaypoint>&>())).Times(1);

    lua_State* L = luaL_newstate();
    lua::create_waypoint(L, mock_shared<MockWaypoint>());
    lua_setglobal(L, "w");
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:add(w)"));
}

TEST(Lua_Route, Clear)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, clear).Times(1);

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:clear()"));
}

TEST(Lua_Route, Colour)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, colour).Times(1).WillOnce(Return(Colour(1, 2, 3, 4)));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "x = r.colour return x"));
    ASSERT_EQ(LUA_TTABLE, lua_type(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.a"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(1.0f, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.r"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(2.0f, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.g"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(3.0f, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.b"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(4.0f, lua_tonumber(L, -1));
}

TEST(Lua_Route, IsRandomizer)
{
    auto rando_route = mock_shared<MockRandomizerRoute>();

    lua_State* L = luaL_newstate();
    lua::create_route(L, rando_route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "x = r.is_randomizer return x"));
    ASSERT_EQ(LUA_TBOOLEAN, lua_type(L, -1));
    ASSERT_TRUE(lua_toboolean(L, -1));

    auto route = mock_shared<MockRoute>();

    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "x = r.is_randomizer return x"));
    ASSERT_EQ(LUA_TBOOLEAN, lua_type(L, -1));
    ASSERT_FALSE(lua_toboolean(L, -1));
}

TEST(Lua_Route, Level)
{
    auto level = mock_shared<MockLevel>();
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, level).WillRepeatedly(Return(level));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "return r.level"));
    ASSERT_EQ(LUA_TUSERDATA, lua_type(L, -1));
    ASSERT_EQ(level.get(), *reinterpret_cast<ILevel**>(lua_touserdata(L, -1)));
}

TEST(Lua_Route, New)
{
    auto route = mock_shared<MockRoute>();

    lua_State* L = luaL_newstate();
    lua::route_register(L, [=](auto&&...) { return route; }, [](auto&&...){ return mock_shared<MockRandomizerRoute>(); }, mock_shared<MockDialogs>(), mock_shared<MockFiles>());

    ASSERT_EQ(0, luaL_dostring(L, "r = Route.new() return r"));
    ASSERT_EQ(LUA_TUSERDATA, lua_type(L, -1));
    ASSERT_EQ(route.get(), *reinterpret_cast<void**>(lua_touserdata(L, -1)));
}

TEST(Lua_Route, NewRandoRoute)
{
    auto route = mock_shared<MockRandomizerRoute>();

    lua_State* L = luaL_newstate();
    lua::route_register(L, [](auto&&...) { return mock_shared<MockRoute>(); }, [=](auto&&...) { return route; }, mock_shared<MockDialogs>(), mock_shared<MockFiles>());

    ASSERT_EQ(0, luaL_dostring(L, "r = Route.new({is_randomizer = true}) return r"));
    ASSERT_EQ(LUA_TUSERDATA, lua_type(L, -1));
    ASSERT_EQ(route.get(), *reinterpret_cast<void**>(lua_touserdata(L, -1)));
}

TEST(Lua_Route, Reload)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, reload);

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:reload()"));
}

TEST(Lua_Route, Remove)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, remove(A<const std::shared_ptr<IWaypoint>&>())).Times(1);

    lua_State* L = luaL_newstate();
    lua::create_waypoint(L, mock_shared<MockWaypoint>());
    lua_setglobal(L, "w");
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:remove(w)"));
}

TEST(Lua_Route, Save)
{
    auto route = mock_shared<MockRoute>();
    ON_CALL(*route, filename).WillByDefault(Return("test"));
    EXPECT_CALL(*route, save).Times(1);

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:save()"));
}

TEST(Lua_Route, SaveNoFilename)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, save).Times(0);
    EXPECT_CALL(*route, set_filename).Times(0);

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:save()"));
}

TEST(Lua_Route, SaveChooseFile)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, save).Times(1);
    EXPECT_CALL(*route, set_filename).Times(1);

    auto dialogs = mock_shared<MockDialogs>();
    EXPECT_CALL(*dialogs, save_file).WillRepeatedly(Return(IDialogs::FileResult{ .filename = "test", .filter_index = 1 }));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:save()"));
}

TEST(Lua_Route, SaveAsWithFileAllowed)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, save_as).Times(1);
    EXPECT_CALL(*route, set_filename).Times(1);

    auto dialogs = mock_shared<MockDialogs>();
    EXPECT_CALL(*dialogs, message_box).WillRepeatedly(Return(true));
    EXPECT_CALL(*dialogs, save_file).WillRepeatedly(Return(IDialogs::FileResult{.filename = "test", .filter_index = 1 }));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:save_as()"));
}

TEST(Lua_Route, SaveAsWithFileRejected)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, save_as).Times(0);
    EXPECT_CALL(*route, set_filename).Times(0);

    auto dialogs = mock_shared<MockDialogs>();
    EXPECT_CALL(*dialogs, save_file).WillRepeatedly(Return(IDialogs::FileResult{.filename = "test", .filter_index = 1 }));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:save_as()"));
}

TEST(Lua_Route, SaveAsChoosesFile)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, save_as).Times(1);
    EXPECT_CALL(*route, set_filename).Times(1);

    auto dialogs = mock_shared<MockDialogs>();
    EXPECT_CALL(*dialogs, save_file).WillRepeatedly(Return(IDialogs::FileResult{.filename = "test", .filter_index = 1 }));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r:save_as()"));
}

TEST(Lua_Route, SetColour)
{
    Colour called_colour;
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, set_colour).Times(1).WillRepeatedly(SaveArg<0>(&called_colour));

    lua_State* L = luaL_newstate();
    lua::colour_register(L);
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r.colour = Colour.new(1, 2, 3)"));

    ASSERT_FLOAT_EQ(called_colour.r, 1.0f);
    ASSERT_FLOAT_EQ(called_colour.g, 2.0f);
    ASSERT_FLOAT_EQ(called_colour.b, 3.0f);
    ASSERT_FLOAT_EQ(called_colour.a, 1.0f);
}

TEST(Lua_Route, SetLevel)
{
    auto level = mock_shared<MockLevel>();
    auto route = mock_shared<MockRoute>();
    
    std::weak_ptr<ILevel> value;
    EXPECT_CALL(*route, set_level).WillRepeatedly(SaveArg<0>(&value));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");
    lua::create_level(L, level);
    lua_setglobal(L, "l");

    ASSERT_EQ(0, luaL_dostring(L, "r.level = l"));
    ASSERT_EQ(value.lock(), level);
}

TEST(Lua_Route, SetWaypointColour)
{
    Colour called_colour;
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, set_waypoint_colour).Times(1).WillRepeatedly(SaveArg<0>(&called_colour));

    lua_State* L = luaL_newstate();
    lua::colour_register(L);
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "r.waypoint_colour = Colour.new(1, 2, 3)"));

    ASSERT_FLOAT_EQ(called_colour.r, 1.0f);
    ASSERT_FLOAT_EQ(called_colour.g, 2.0f);
    ASSERT_FLOAT_EQ(called_colour.b, 3.0f);
    ASSERT_FLOAT_EQ(called_colour.a, 1.0f);
}

TEST(Lua_Route, SetWaypoints)
{
    auto route = mock_shared<MockRoute>();
    auto waypoint1 = mock_shared<MockWaypoint>();
    auto waypoint2 = mock_shared<MockWaypoint>();
    EXPECT_CALL(*route, clear);
    EXPECT_CALL(*route, add(Eq(waypoint1)));
    EXPECT_CALL(*route, add(Eq(waypoint2)));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");
    lua::create_waypoint(L, waypoint1);
    lua_setglobal(L, "w1");
    lua::create_waypoint(L, waypoint2);
    lua_setglobal(L, "w2");

    ASSERT_EQ(0, luaL_dostring(L, "r.waypoints = { w1, w2 }"));
}

TEST(Lua_Route, WaypointColour)
{
    auto route = mock_shared<MockRoute>();
    EXPECT_CALL(*route, waypoint_colour).Times(1).WillOnce(Return(Colour(1, 2, 3, 4)));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "x = r.waypoint_colour return x"));
    ASSERT_EQ(LUA_TTABLE, lua_type(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.a"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(1.0f, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.r"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(2.0f, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.g"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(3.0f, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x.b"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(4.0f, lua_tonumber(L, -1));
}

TEST(Lua_Route, Waypoints)
{
    auto waypoint1 = mock_shared<MockWaypoint>();
    ON_CALL(*waypoint1, notes).WillByDefault(Return("Waypoint 1"));
    auto waypoint2 = mock_shared<MockWaypoint>();
    ON_CALL(*waypoint2, notes).WillByDefault(Return("Waypoint 2"));

    auto route = mock_shared<MockRoute>();
    ON_CALL(*route, waypoints).WillByDefault(Return(2));
    ON_CALL(*route, waypoint(0)).WillByDefault(Return(waypoint1));
    ON_CALL(*route, waypoint(1)).WillByDefault(Return(waypoint2));

    lua_State* L = luaL_newstate();
    lua::create_route(L, route);
    lua_setglobal(L, "r");

    ASSERT_EQ(0, luaL_dostring(L, "x = r.waypoints return x"));
    ASSERT_EQ(LUA_TTABLE, lua_type(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return #x"));
    ASSERT_EQ(LUA_TNUMBER, lua_type(L, -1));
    ASSERT_EQ(2, lua_tonumber(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x[1].notes"));
    ASSERT_EQ(LUA_TSTRING, lua_type(L, -1));
    ASSERT_STREQ("Waypoint 1", lua_tostring(L, -1));
    ASSERT_EQ(0, luaL_dostring(L, "return x[2].notes"));
    ASSERT_EQ(LUA_TSTRING, lua_type(L, -1));
    ASSERT_STREQ("Waypoint 2", lua_tostring(L, -1));
}
