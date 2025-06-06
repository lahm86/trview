#include <trview.app/Routing/Route.h>
#include <trview.app/Mocks/Graphics/ISelectionRenderer.h>
#include <trview.app/Mocks/Routing/IWaypoint.h>
#include <trview.app/Mocks/Camera/ICamera.h>
#include <trview.tests.common/Mocks.h>

using namespace trview;
using namespace trview::mocks;
using namespace trview::tests;
using namespace DirectX::SimpleMath;
using testing::Return;
using testing::NiceMock;
using testing::A;
using testing::SaveArg;

namespace
{
    auto register_test_module()
    {
        struct test_module
        {
            std::unique_ptr<ISelectionRenderer> selection_renderer = mock_unique<MockSelectionRenderer>();
            IWaypoint::Source waypoint_source = [](auto&&...) { return mock_unique<MockWaypoint>(); };
            UserSettings settings;

            test_module& with_selection_renderer(std::unique_ptr<ISelectionRenderer> selection_renderer)
            {
                this->selection_renderer = std::move(selection_renderer);
                return *this;
            }

            test_module& with_waypoint_source(IWaypoint::Source waypoint_source)
            {
                this->waypoint_source = waypoint_source;
                return *this;
            }

            test_module& with_settings(const UserSettings& settings)
            {
                this->settings = settings;
                return *this;
            }

            std::shared_ptr<Route> build()
            {
                return std::make_shared<Route>(std::move(selection_renderer), waypoint_source, settings);
            }
        };
        return test_module{};
    }

    struct WaypointDetails
    {
        Vector3 position;
        Vector3 normal;
        uint32_t room;
        IWaypoint::Type type;
        uint32_t index;
        Colour colour;
        Colour waypoint_colour;
    };

    /// <summary>
    /// Get a waypoint source that makes mock waypoints and gives each one a sequential number.
    /// </summary>
    /// <param name="test_index">Initial number, should be 0 and live as long or longer than the source.</param>
    /// <returns>Waypoint source.</returns>
    IWaypoint::Source indexed_source(uint32_t& test_index)
    {
        return [&](auto&&...)
        {
            auto waypoint = mock_unique<MockWaypoint>();
            waypoint->test_index = test_index++;
            return waypoint;
        };
    }

    /// <summary>
    /// Get the test index values of the waypoints in the order they are in the list.
    /// </summary>
    /// <param name="route">The route to test.</param>
    /// <returns>The ordered index values.</returns>
    std::vector<uint32_t> get_order(Route& route)
    {
        std::vector<uint32_t> waypoints;
        for (auto i = 0u; i < route.waypoints(); ++i)
        {
            if (auto waypoint = route.waypoint(i).lock())
            {
                waypoints.push_back(std::static_pointer_cast<MockWaypoint>(waypoint)->test_index);
            }
        }
        return waypoints;
    }
}

TEST(Route, Add)
{
    std::optional<WaypointDetails> waypoint_values;
    auto source = [&](auto&&... args)
    {
        waypoint_values = { args... };
        return mock_unique<MockWaypoint>();
    };

    auto route = register_test_module().with_waypoint_source(source).build();

    route->add(Vector3(0, 1, 0), Vector3::Down, 10);

    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->waypoints(), 1);
    ASSERT_TRUE(waypoint_values.has_value());
    ASSERT_EQ(waypoint_values.value().position, Vector3(0, 1, 0));
    ASSERT_EQ(waypoint_values.value().room, 10);
    ASSERT_EQ(waypoint_values.value().type, IWaypoint::Type::Position);
}

TEST(Route, AddBindsWaypoint)
{
    auto waypoint = mock_shared<MockWaypoint>();
    EXPECT_CALL(*waypoint, set_route).Times(1);
    EXPECT_CALL(*waypoint, set_route_colour(Colour::Yellow)).Times(1);
    EXPECT_CALL(*waypoint, set_waypoint_colour(Colour::Green)).Times(1);

    auto route = register_test_module().build();
    route->set_colour(Colour::Yellow);
    route->set_waypoint_colour(Colour::Green);

    int raised_count = 0;
    auto token = route->on_changed += [&]() { ++raised_count; };

    route->add(waypoint);

    ASSERT_EQ(raised_count, 1);
    waypoint->on_changed();
    ASSERT_EQ(raised_count, 2);
}

TEST(Route, AddSpecificType)
{
    std::optional<WaypointDetails> waypoint_values;
    auto source = [&](auto&&... args)
    {
        waypoint_values = { args... };
        return mock_unique<MockWaypoint>();
    };
    auto route = register_test_module().with_waypoint_source(source).build();
    route->add(Vector3(0, 1, 0), Vector3::Down, 10, IWaypoint::Type::Trigger, 100);
    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->waypoints(), 1);
    ASSERT_EQ(waypoint_values.value().position, Vector3(0, 1, 0));
    ASSERT_EQ(waypoint_values.value().room, 10);
    ASSERT_EQ(waypoint_values.value().type, IWaypoint::Type::Trigger);
    ASSERT_EQ(waypoint_values.value().index, 100);
}

TEST(Route, Clear)
{
    auto route = register_test_module().build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    ASSERT_TRUE(route->is_unsaved());
    route->set_unsaved(false);
    route->clear();
    ASSERT_TRUE(route->is_unsaved());
}

TEST(Route, ClearAlreadyEmpty)
{
    auto route = register_test_module().build();
    route->clear();
    ASSERT_FALSE(route->is_unsaved());
}

TEST(Route, InsertAtPosition)
{
    uint32_t test_index = 0;
    auto route = register_test_module().with_waypoint_source(indexed_source(test_index)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 1);
    route->set_unsaved(false);
    route->insert(Vector3(0, 1, 0), Vector3::Down, 2, 1);
    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->waypoints(), 3);

    const auto order = get_order(*route);
    const auto expected = std::vector<uint32_t>{ 0u, 2u, 1u };
    ASSERT_EQ(order, expected);
}

TEST(Route, InsertSpecificTypeAtPosition)
{
    uint32_t test_index = 0;
    auto route = register_test_module().with_waypoint_source(indexed_source(test_index)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 1);
    route->set_unsaved(false);
    route->insert(Vector3(0, 1, 0), Vector3::Down, 2, 1, IWaypoint::Type::Entity, 100);
    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->waypoints(), 3);

    const auto order = get_order(*route);
    const auto expected = std::vector<uint32_t>{ 0u, 2u, 1u };
    ASSERT_EQ(order, expected);
}

TEST(Route, Insert)
{
    uint32_t test_index = 0;
    auto route = register_test_module().with_waypoint_source(indexed_source(test_index)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    auto wp1 = route->add(Vector3::Zero, Vector3::Down, 1);
    route->set_unsaved(false);
    route->select_waypoint(wp1);
    auto index = route->insert(Vector3(0, 1, 0), Vector3::Down, 2);
    ASSERT_EQ(index, 2);
    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->waypoints(), 3);
    
    const auto order = get_order(*route);
    const auto expected = std::vector<uint32_t>{ 0u, 1u, 2u };
    ASSERT_EQ(order, expected);
}

TEST(Route, IsUnsaved)
{
    auto route = register_test_module().build();
    ASSERT_FALSE(route->is_unsaved());
    route->set_unsaved(true);
    ASSERT_TRUE(route->is_unsaved());
}

TEST(Route, MoveBackwards)
{
    uint32_t test_index = 0;
    auto route = register_test_module().with_waypoint_source(indexed_source(test_index)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 1);
    route->add(Vector3::Zero, Vector3::Down, 2);
    route->set_unsaved(false);
    route->move(2, 0);
    const auto order = get_order(*route);
    const auto expected = std::vector<uint32_t>{ 2u, 0u, 1u };
    ASSERT_EQ(order, expected);
    ASSERT_TRUE(route->is_unsaved());
}

TEST(Route, MoveForward)
{
    uint32_t test_index = 0;
    auto route = register_test_module().with_waypoint_source(indexed_source(test_index)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 1);
    route->add(Vector3::Zero, Vector3::Down, 2);
    route->set_unsaved(false);
    route->move(0, 2);
    const auto order = get_order(*route);
    const auto expected = std::vector<uint32_t>{ 1u, 2u, 0u };
    ASSERT_EQ(order, expected);
    ASSERT_TRUE(route->is_unsaved());
}

TEST(Route, Remove)
{
    auto route = register_test_module().build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    ASSERT_EQ(route->waypoints(), 1);
    route->set_unsaved(false);
    route->remove(0);
    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->waypoints(), 0);
}

TEST(Route, SetColour)
{
    auto route = register_test_module().build();
    route->set_colour(Colour::Red);
    ASSERT_TRUE(route->is_unsaved());
    ASSERT_EQ(route->colour(), Colour::Red);
}

TEST(Route, SelectedWaypoint)
{
    auto route = register_test_module().build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    auto wp1 = route->add(Vector3::Zero, Vector3::Down, 0);
    route->set_unsaved(false);
    route->select_waypoint(wp1);
    ASSERT_FALSE(route->is_unsaved());
    ASSERT_EQ(route->selected_waypoint(), 1);
    ASSERT_EQ(route->waypoint(route->selected_waypoint()).lock(), wp1);
}

TEST(Route, SelectedWaypointAdjustedByRemove)
{
    auto route = register_test_module().build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    auto wp1 = route->add(Vector3::Zero, Vector3::Down, 0);
    route->set_unsaved(false);
    route->select_waypoint(wp1);
    ASSERT_FALSE(route->is_unsaved());
    ASSERT_EQ(route->selected_waypoint(), 1);
    route->remove(1);
    ASSERT_EQ(route->selected_waypoint(), 0);
}

TEST(Route, Render)
{
    auto [selection_renderer_ptr, selection_renderer] = create_mock<MockSelectionRenderer>();
    EXPECT_CALL(selection_renderer, render).Times(1);

    auto [w1_ptr, w1] = create_mock<MockWaypoint>();
    auto [w2_ptr, w2] = create_mock<MockWaypoint>();
    EXPECT_CALL(w1, render).Times(1);
    EXPECT_CALL(w1, render_join).Times(1);
    EXPECT_CALL(w2, render).Times(1);
    EXPECT_CALL(w2, render_join).Times(0);

    auto w1_ptr_actual = std::move(w1_ptr);
    auto w2_ptr_actual = std::move(w2_ptr);
    auto source = [&](auto&&...)
    {
        return w1_ptr_actual ? std::move(w1_ptr_actual) : w2_ptr_actual ? std::move(w2_ptr_actual) : nullptr;
    };

    auto route = register_test_module().with_selection_renderer(std::move(selection_renderer_ptr)).with_waypoint_source(source).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 0);
 
    NiceMock<MockCamera> camera;
    route->render(camera, true);
}

TEST(Route, RenderDoesNotJoinWhenRouteLineDisabled)
{
    auto [selection_renderer_ptr, selection_renderer] = create_mock<MockSelectionRenderer>();
    EXPECT_CALL(selection_renderer, render).Times(1);

    auto [w1_ptr, w1] = create_mock<MockWaypoint>();
    auto [w2_ptr, w2] = create_mock<MockWaypoint>();
    auto [w3_ptr, w3] = create_mock<MockWaypoint>();
    EXPECT_CALL(w1, render).Times(1);
    EXPECT_CALL(w1, render_join).Times(0);
    EXPECT_CALL(w2, render).Times(1);
    EXPECT_CALL(w2, type).WillRepeatedly(Return(IWaypoint::Type::Position));
    EXPECT_CALL(w2, render_join).Times(0);
    EXPECT_CALL(w3, render).Times(1);

    auto w1_ptr_actual = std::move(w1_ptr);
    auto w2_ptr_actual = std::move(w2_ptr);
    auto w3_ptr_actual = std::move(w3_ptr);
    auto source = [&](auto&&...) -> std::unique_ptr<IWaypoint>
    {
        if (w1_ptr_actual) { return std::move(w1_ptr_actual); }
        else if (w2_ptr_actual) { return std::move(w2_ptr_actual); }
        else if (w3_ptr_actual) { return std::move(w3_ptr_actual); }
        return nullptr;
    };

    auto route = register_test_module().with_selection_renderer(std::move(selection_renderer_ptr)).with_waypoint_source(source).build();
    route->set_show_route_line(false);
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 0);

    NiceMock<MockCamera> camera;
    route->render(camera, true);
}

TEST(Route, RenderShowsSelection)
{
    auto [selection_renderer_ptr, selection_renderer] = create_mock<MockSelectionRenderer>();
    EXPECT_CALL(selection_renderer, render).Times(1);

    NiceMock<MockCamera> camera;

    auto route = register_test_module().with_selection_renderer(std::move(selection_renderer_ptr)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->render(camera, true);
}

TEST(Route, RenderDoesNotShowSelection)
{
    auto [selection_renderer_ptr, selection_renderer] = create_mock<MockSelectionRenderer>();
    EXPECT_CALL(selection_renderer, render).Times(0);

    NiceMock<MockCamera> camera;

    auto route = register_test_module().with_selection_renderer(std::move(selection_renderer_ptr)).build();
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->render(camera, false);
}

TEST(Route, AddWaypointUsesColours)
{
    UserSettings settings;
    settings.route_colour = Colour::Red;
    settings.waypoint_colour = Colour::Green;

    std::optional<WaypointDetails> waypoint_values;
    auto source = [&](auto&&... args)
    {
        waypoint_values = { args... };
        return mock_unique<MockWaypoint>();
    };

    auto route = register_test_module().with_settings(settings).with_waypoint_source(source).build();
    route->add(Vector3::Zero, Vector3::Zero, 0);

    ASSERT_TRUE(waypoint_values.has_value());
    ASSERT_EQ(waypoint_values.value().colour, Colour::Red);
    ASSERT_EQ(waypoint_values.value().waypoint_colour, Colour::Green);
}

TEST(Route, Reload)
{
    const std::string contents = "{\"colour\":\"4278255360\",\"waypoint_colour\":\"4294967295\",\"waypoints\":[{\"index\":0,\"normal\":\"0,0,0\",\"notes\":\"\",\"position\":\"0,0,0\",\"room\":0,\"type\":\"Position\"},{\"index\":0,\"normal\":\"0,0,0\",\"notes\":\"\",\"position\":\"0,0,0\",\"room\":0,\"type\":\"Position\"}]}";
    std::vector<uint8_t> bytes = contents
        | std::views::transform([](const auto v) { return static_cast<uint8_t>(v); })
        | std::ranges::to<std::vector>();
    UserSettings settings{};
    auto files = mock_shared<MockFiles>();
    EXPECT_CALL(*files, load_file(A<const std::string&>())).WillRepeatedly(Return(bytes));

    auto route = register_test_module().build();

    route->set_filename("test.tvr");
    route->reload(files, settings);

    ASSERT_EQ(route->waypoints(), 2);
}

TEST(Route, RouteUsesDefaultColours)
{
    UserSettings settings;
    settings.route_colour = Colour::Yellow;
    settings.waypoint_colour = Colour::Cyan;
    auto route = register_test_module().with_settings(settings).build();
    ASSERT_EQ(route->colour(), settings.route_colour);
    ASSERT_EQ(route->waypoint_colour(), settings.waypoint_colour);
}

TEST(Route, Save)
{
    auto route = register_test_module().build();
    auto files = mock_shared<MockFiles>();

    std::string contents;
    EXPECT_CALL(*files, save_file("test.tvr", A<const std::string&>()))
        .WillOnce(SaveArg<1>(&contents));

    UserSettings settings{};

    route->set_filename("test.tvr");
    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 1);
    route->save(files, settings);

    ASSERT_EQ(contents, "{\"colour\":\"4278255360\",\"waypoint_colour\":\"4294967295\",\"waypoints\":[{\"index\":0,\"normal\":\"0,0,0\",\"notes\":\"\",\"position\":\"0,0,0\",\"room\":0,\"type\":\"Position\"},{\"index\":0,\"normal\":\"0,0,0\",\"notes\":\"\",\"position\":\"0,0,0\",\"room\":0,\"type\":\"Position\"}]}");
}

TEST(Route, SaveNoFileName)
{
    auto route = register_test_module().build();
    auto files = mock_shared<MockFiles>();

    EXPECT_CALL(*files, save_file(A<const std::string&>(), A<const std::string&>())).Times(0);

    UserSettings settings{};
    route->save(files, settings);
}

TEST(Route, SaveAs)
{
    auto route = register_test_module().build();
    auto files = mock_shared<MockFiles>();

    std::string contents;
    EXPECT_CALL(*files, save_file("test.tvr", A<const std::string&>()))
        .WillOnce(SaveArg<1>(&contents));

    UserSettings settings{};

    route->add(Vector3::Zero, Vector3::Down, 0);
    route->add(Vector3::Zero, Vector3::Down, 1);
    route->save_as(files, "test.tvr", settings);

    ASSERT_EQ(contents, "{\"colour\":\"4278255360\",\"waypoint_colour\":\"4294967295\",\"waypoints\":[{\"index\":0,\"normal\":\"0,0,0\",\"notes\":\"\",\"position\":\"0,0,0\",\"room\":0,\"type\":\"Position\"},{\"index\":0,\"normal\":\"0,0,0\",\"notes\":\"\",\"position\":\"0,0,0\",\"room\":0,\"type\":\"Position\"}]}");
}

TEST(Route, SetColourUpdatesWaypoints)
{
    auto [waypoint_ptr, waypoint] = create_mock<MockWaypoint>();
    auto source = [&](auto&&... args)
    {
        return std::move(waypoint_ptr);
    };

    EXPECT_CALL(waypoint, set_route_colour(Colour::Green)).Times(1);
    EXPECT_CALL(waypoint, set_route_colour(Colour::Yellow)).Times(1);
    auto route = register_test_module().with_waypoint_source(source).build();
    route->set_colour(Colour::Green);
    route->add(Vector3::Zero, Vector3::Zero, 0);
    route->set_colour(Colour::Yellow);
}

TEST(Route, SetWaypointColourUpdatesWaypoints)
{
    auto [waypoint_ptr, waypoint] = create_mock<MockWaypoint>();
    auto source = [&](auto&&... args)
    {
        return std::move(waypoint_ptr);
    };

    EXPECT_CALL(waypoint, set_waypoint_colour(Colour::Green)).Times(1);
    EXPECT_CALL(waypoint, set_waypoint_colour(Colour::Cyan)).Times(1);
    auto route = register_test_module().with_waypoint_source(source).build();
    route->set_waypoint_colour(Colour::Green);
    route->add(Vector3::Zero, Vector3::Zero, 0);
    route->set_waypoint_colour(Colour::Cyan);
}

TEST(Route, SetShowRouteLine)
{
    auto route = register_test_module().build();

    bool raised = false;
    auto token = route->on_changed += [&]() { raised = true; };

    route->set_show_route_line(false);

    ASSERT_TRUE(raised);
    ASSERT_FALSE(route->show_route_line());
}
