#include <trview.app/Routing/Waypoint.h>
#include <trview.app/Mocks/Geometry/IMesh.h>

using namespace trview;
using namespace trview::mocks;
using namespace trview::tests;
using namespace DirectX::SimpleMath;

TEST(Waypoint, ConstructorProperties)
{
    Waypoint waypoint(mock_shared<MockMesh>(), Vector3(1, 2, 3), Vector3::Down, 12, IWaypoint::Type::Trigger, 23, Colour::Red);
    ASSERT_EQ(waypoint.position(), Vector3(1, 2, 3));
    ASSERT_EQ(waypoint.normal(), Vector3::Down);
    ASSERT_EQ(waypoint.room(), 12);
    ASSERT_EQ(waypoint.type(), IWaypoint::Type::Trigger);
    ASSERT_EQ(waypoint.index(), 23);
}

TEST(Waypoint, EmptySave)
{
    Waypoint waypoint(mock_shared<MockMesh>(), Vector3::Zero, Vector3::Down, 0, IWaypoint::Type::Position, 0, Colour::Red);
    ASSERT_FALSE(waypoint.has_save());
    waypoint.set_save_file({});
    ASSERT_FALSE(waypoint.has_save());
}

TEST(Waypoint, Notes)
{
    Waypoint waypoint(mock_shared<MockMesh>(), Vector3(1, 2, 3), Vector3::Down, 12, IWaypoint::Type::Trigger, 23, Colour::Red);
    waypoint.set_notes(L"Test notes\nNew line");
    ASSERT_EQ(waypoint.notes(), L"Test notes\nNew line");
}

TEST(Waypoint, SaveFile)
{
    Waypoint waypoint(mock_shared<MockMesh>(), Vector3::Zero, Vector3::Down, 0, IWaypoint::Type::Position, 0, Colour::Red);
    ASSERT_FALSE(waypoint.has_save());
    waypoint.set_save_file({ 0x1 });
    ASSERT_TRUE(waypoint.has_save());
    ASSERT_EQ(waypoint.save_file(), std::vector<uint8_t>{ 0x1 });
}

TEST(Waypoint, Visibility)
{
    Waypoint waypoint(mock_shared<MockMesh>(), Vector3::Zero, Vector3::Down, 0, IWaypoint::Type::Position, 0, Colour::Red);
    ASSERT_TRUE(waypoint.visible());
    waypoint.set_visible(false);
    ASSERT_FALSE(waypoint.visible());
    waypoint.set_visible(true);
    ASSERT_TRUE(waypoint.visible());
}

TEST(Waypoint, RandomizerProperties)
{
    Waypoint waypoint(mock_shared<MockMesh>(), Vector3::Zero, Vector3::Down, 0, IWaypoint::Type::Position, 0, Colour::Red);
    auto existing = waypoint.randomizer_settings();
    ASSERT_TRUE(existing.empty());
    existing["test1"] = std::string("Test");
    waypoint.set_randomizer_settings(existing);
    auto updated = waypoint.randomizer_settings();
    ASSERT_EQ(existing.size(), 1);
    ASSERT_EQ(std::get<std::string>(updated["test1"]), "Test");
}