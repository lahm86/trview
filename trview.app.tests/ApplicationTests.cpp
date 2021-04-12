#include <trview.app/Application.h>
#include <trview.app/Mocks/Menus/IUpdateChecker.h>
#include <trview.app/Mocks/Menus/IFileDropper.h>
#include <trview.app/Mocks/Menus/ILevelSwitcher.h>
#include <trview.app/Mocks/Menus/IRecentFiles.h>
#include <trview.app/Mocks/Routing/IRoute.h>
#include <trview.app/Mocks/Settings/ISettingsLoader.h>
#include <trview.app/Mocks/Windows/IViewer.h>
#include <trview.app/Mocks/Windows/IItemsWindowManager.h>
#include <trview.app/Mocks/Windows/ITriggersWindowManager.h>
#include <trview.app/Mocks/Windows/IRouteWindowManager.h>
#include <trview.app/Mocks/Windows/IRoomsWindowManager.h>
#include <trview.common/Mocks/Windows/IShortcuts.h>
#include <trlevel/Mocks/ILevelLoader.h>
#include <trlevel/Mocks/ILevel.h>
#include <trview.app/Mocks/Elements/ILevel.h>

using namespace trview;
using namespace trview::tests;
using namespace testing;
using namespace trview::mocks;
using namespace trlevel::mocks;
using testing::_;

TEST(Application, ChecksForUpdates)
{
    auto [update_checker_ptr, update_checker] = create_mock<MockUpdateChecker>();
    EXPECT_CALL(update_checker, check_for_updates).Times(1);
    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::move(update_checker_ptr),
        std::make_unique<MockSettingsLoader>(),
        std::make_unique<MockFileDropper>(),
        std::make_unique<MockLevelLoader>(),
        std::make_unique<MockLevelSwitcher>(),
        std::make_unique<MockRecentFiles>(),
        std::make_unique<MockViewer>(),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());
}

TEST(Application, SettingsLoadedAndSaved)
{
    auto [settings_loader_ptr, settings_loader] = create_mock<MockSettingsLoader>();
    EXPECT_CALL(settings_loader, load_user_settings).Times(1);
    EXPECT_CALL(settings_loader, save_user_settings).Times(1);
    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::move(settings_loader_ptr),
        std::make_unique<MockFileDropper>(),
        std::make_unique<MockLevelLoader>(),
        std::make_unique<MockLevelSwitcher>(),
        std::make_unique<MockRecentFiles>(),
        std::make_unique<MockViewer>(),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());
}

TEST(Application, FileDropperOpensFile)
{
    auto [file_dropper_ptr, file_dropper] = create_mock<MockFileDropper>();
    auto [level_loader_ptr, level_loader] = create_mock<MockLevelLoader>();

    EXPECT_CALL(level_loader, load_level("test_path.tr2"))
        .Times(1)
        .WillRepeatedly(Throw(std::exception()));

    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::make_unique<MockSettingsLoader>(),
        std::move(file_dropper_ptr),
        std::move(level_loader_ptr),
        std::make_unique<MockLevelSwitcher>(),
        std::make_unique<MockRecentFiles>(),
        std::make_unique<MockViewer>(),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());
    file_dropper.on_file_dropped("test_path.tr2");
}

TEST(Application, LevelLoadedOnSwitchLevel)
{
    auto [level_loader_ptr, level_loader] = create_mock<MockLevelLoader>();
    auto [level_switcher_ptr, level_switcher] = create_mock<MockLevelSwitcher>();

    EXPECT_CALL(level_loader, load_level("test_path.tr2"))
        .Times(1)
        .WillRepeatedly(Throw(std::exception()));

    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::make_unique<MockSettingsLoader>(),
        std::make_unique<MockFileDropper>(),
        std::move(level_loader_ptr),
        std::move(level_switcher_ptr),
        std::make_unique<MockRecentFiles>(),
        std::make_unique<MockViewer>(),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());

    level_switcher.on_switch_level("test_path.tr2");
}

TEST(Application, LevelLoadedOnRecentFileOpen)
{
    auto [level_loader_ptr, level_loader] = create_mock<MockLevelLoader>();
    auto [recent_files_ptr, recent_files] = create_mock<MockRecentFiles>();

    EXPECT_CALL(level_loader, load_level("test_path.tr2"))
        .Times(1)
        .WillRepeatedly(Throw(std::exception()));

    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::make_unique<MockSettingsLoader>(),
        std::make_unique<MockFileDropper>(),
        std::move(level_loader_ptr),
        std::make_unique<MockLevelSwitcher>(),
        std::move(recent_files_ptr),
        std::make_unique<MockViewer>(),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());

    recent_files.on_file_open("test_path.tr2");
}

TEST(Application, RecentFilesUpdatedOnFileOpen)
{
    auto [recent_files_ptr, recent_files] = create_mock<MockRecentFiles>();
    auto [level_loader_ptr, level_loader] = create_mock<MockLevelLoader>();

    EXPECT_CALL(recent_files, set_recent_files(std::list<std::string>{})).Times(1);
    EXPECT_CALL(recent_files, set_recent_files(std::list<std::string>{"test_path.tr2"})).Times(1);
    EXPECT_CALL(level_loader, load_level("test_path.tr2")).WillOnce(Return(ByMove(std::make_unique<trlevel::mocks::MockLevel>())));

    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::make_unique<MockSettingsLoader>(),
        std::make_unique<MockFileDropper>(),
        std::move(level_loader_ptr),
        std::make_unique<MockLevelSwitcher>(),
        std::move(recent_files_ptr),
        std::make_unique<MockViewer>(),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());

    application.open("test_path.tr2");
}

TEST(Application, FileOpenedInViewer)
{
    auto [level_loader_ptr, level_loader] = create_mock<MockLevelLoader>();
    auto [viewer_ptr, viewer] = create_mock<MockViewer>();
    auto [level_ptr, level] = create_mock<trlevel::mocks::MockLevel>();

    EXPECT_CALL(level_loader, load_level("test_path.tr2")).WillOnce(Return(ByMove(std::move(level_ptr))));
    EXPECT_CALL(viewer, open(NotNull())).Times(1);

    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::make_unique<MockSettingsLoader>(),
        std::make_unique<MockFileDropper>(),
        std::move(level_loader_ptr),
        std::make_unique<MockLevelSwitcher>(),
        std::make_unique<MockRecentFiles>(),
        std::move(viewer_ptr),
        []() { return std::make_unique<MockRoute>(); },
        std::make_unique<Shortcuts>(window),
        std::make_unique<MockItemsWindowManager>(),
        std::make_unique<MockTriggersWindowManager>(),
        std::make_unique<MockRouteWindowManager>(),
        std::make_unique<MockRoomsWindowManager>(),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());

    application.open("test_path.tr2");
}

TEST(Application, WindowContentsResetBeforeViewerLoaded)
{
    auto [level_loader_ptr, level_loader] = create_mock<MockLevelLoader>();
    auto [viewer_ptr, viewer] = create_mock<MockViewer>();
    auto [level_ptr, level] = create_mock<trlevel::mocks::MockLevel>();
    auto [items_window_manager_ptr, items_window_manager] = create_mock<MockItemsWindowManager>();
    auto [rooms_window_manager_ptr, rooms_window_manager] = create_mock<MockRoomsWindowManager>();
    auto [triggers_window_manager_ptr, triggers_window_manager] = create_mock<MockTriggersWindowManager>();
    auto [route_window_manager_ptr, route_window_manager] = create_mock<MockRouteWindowManager>();
    auto [route_ptr, route] = create_mock<MockRoute>();
    auto route_ptr_src = std::move(route_ptr);

    std::vector<std::string> events;

    EXPECT_CALL(level_loader, load_level("test_path.tr2")).WillOnce(Return(ByMove(std::move(level_ptr))));
    
    EXPECT_CALL(items_window_manager, set_items(A<const std::vector<Item>&>())).Times(1).WillOnce([&](auto) { events.push_back("items_items"); });
    EXPECT_CALL(items_window_manager, set_triggers(A<const std::vector<Trigger*>&>())).Times(1).WillOnce([&](auto) { events.push_back("items_triggers"); });
    EXPECT_CALL(triggers_window_manager, set_items(A<const std::vector<Item>&>())).Times(1).WillOnce([&](auto) { events.push_back("triggers_items"); });
    EXPECT_CALL(triggers_window_manager, set_triggers(A<const std::vector<Trigger*>&>())).Times(1).WillOnce([&](auto) { events.push_back("triggers_triggers"); });
    EXPECT_CALL(rooms_window_manager, set_items(A<const std::vector<Item>&>())).Times(1).WillOnce([&](auto) { events.push_back("rooms_items"); });
    EXPECT_CALL(rooms_window_manager, set_triggers(A<const std::vector<Trigger*>&>())).Times(1).WillOnce([&](auto) { events.push_back("rooms_triggers"); });
    EXPECT_CALL(rooms_window_manager, set_rooms(A<const std::vector<Room*>&>())).Times(1).WillOnce([&](auto) { events.push_back("rooms_rooms"); });
    EXPECT_CALL(route_window_manager, set_items(A<const std::vector<Item>&>())).Times(1).WillOnce([&](auto) { events.push_back("route_items"); });
    EXPECT_CALL(route_window_manager, set_triggers(A<const std::vector<Trigger*>&>())).Times(1).WillOnce([&](auto) { events.push_back("route_triggers"); });
    EXPECT_CALL(route_window_manager, set_rooms(A<const std::vector<Room*>&>())).Times(1).WillOnce([&](auto) { events.push_back("route_rooms"); });
    EXPECT_CALL(route_window_manager, set_route(A<IRoute*>())).Times(1).WillOnce([&](auto) { events.push_back("route_route"); });
    EXPECT_CALL(route, clear()).Times(1).WillOnce([&] { events.push_back("route_clear"); });
    EXPECT_CALL(viewer, open(NotNull())).Times(1).WillOnce([&](auto) { events.push_back("viewer"); });

    auto window = create_test_window(L"ApplicationTests");
    Application application(window,
        std::make_unique<MockUpdateChecker>(),
        std::make_unique<MockSettingsLoader>(),
        std::make_unique<MockFileDropper>(),
        std::move(level_loader_ptr),
        std::make_unique<MockLevelSwitcher>(),
        std::make_unique<MockRecentFiles>(),
        std::move(viewer_ptr),
        [&]() { return std::move(route_ptr_src); },
        std::make_unique<Shortcuts>(window),
        std::move(items_window_manager_ptr),
        std::move(triggers_window_manager_ptr),
        std::move(route_window_manager_ptr),
        std::move(rooms_window_manager_ptr),
        [](auto) { return std::make_unique<trview::mocks::MockLevel>(); },
        std::wstring());

    application.open("test_path.tr2");

    ASSERT_EQ(events.size(), 13);
    ASSERT_EQ(events.back(), "viewer");
}
