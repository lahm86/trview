#include <trview.app/Windows/RoomsWindow.h>
#include <trview.ui.render/Mocks/IRenderer.h>
#include <trview.ui/Button.h>
#include <trview.ui/Listbox.h>
#include <trview.app/Elements/Types.h>
#include <trview.graphics/mocks/IDeviceWindow.h>
#include <trview.app/Mocks/Geometry/IMesh.h>
#include <trview.ui.render/Mocks/IMapRenderer.h>
#include <trview.app/Mocks/Elements/ILevel.h>
#include <trlevel/Mocks/ILevel.h>
#include <trview.app/Mocks/Graphics/ILevelTextureStorage.h>
#include <trview.app/Mocks/Graphics/IMeshStorage.h>
#include <trview.ui/Mocks/Input/IInput.h>
#include <trview.input/Mocks/IMouse.h>
#include <external/boost/di.hpp>

using namespace trview;
using namespace trview::tests;
using namespace trview::graphics;
using namespace trview::graphics::mocks;
using namespace trview::ui::mocks;
using namespace trview::ui::render::mocks;
using namespace trview::mocks;
using namespace trview::input::mocks;
using testing::ReturnRef;

namespace
{
    MockMouse mouse;

    auto register_test_module()
    {
        using namespace boost;
        return di::make_injector(
            di::bind<IDeviceWindow::Source>.to([&](auto&&) { return [&](auto&&) { return std::make_unique<MockDeviceWindow>(); }; }),
            di::bind<ui::render::IRenderer::Source>.to([&](auto&&) { return [&](auto&&) { return std::make_unique<MockRenderer>(); }; }),
            di::bind<ui::render::IMapRenderer::Source>.to([&](auto&&) { return [&](auto&&) { return std::make_unique<MockMapRenderer>(); }; }),
            di::bind<ui::IInput::Source>.to([&](auto&&) { return [&](auto&&, auto&&) 
                { 
                    auto input = std::make_unique<MockInput>();
                    EXPECT_CALL(*input, mouse).WillRepeatedly(ReturnRef(mouse));
                    return std::move(input);
                };
            }),
            di::bind<Window>.to(create_test_window(L"ItemsWindowTests")),
            di::bind<RoomsWindow>()
        );
    }
}

TEST(RoomsWindow, ClearSelectedTriggerClearsSelection)
{
    auto window = register_test_module().create<std::unique_ptr<RoomsWindow>>();

    std::optional<Trigger*> raised_trigger;
    auto token = window->on_trigger_selected += [&raised_trigger](const auto& trigger) { raised_trigger = trigger; };

    auto [trlevel_ptr, trlevel] = create_mock<trlevel::mocks::MockLevel>();
    auto [level_ptr, level] = create_mock<MockLevel>();
    auto [texture_storage_ptr, texture_storage] = create_mock<MockLevelTextureStorage>();
    auto [mesh_storage_ptr, mesh_storage] = create_mock<MockMeshStorage>();
    trlevel::tr3_room tr_room{};

    auto room = std::make_unique<Room>(
        [](auto, auto, auto, auto, auto) { return std::make_unique<MockMesh>(); },
        trlevel, tr_room, texture_storage, mesh_storage, 0, level);

    auto trigger1 = std::make_unique<Trigger>(0, 0, 100, 200, TriggerInfo{ 0, 0, 0, TriggerType::Trigger, 0, {  } }, [](auto, auto) { return std::make_unique<MockMesh>(); });
    room->add_trigger(trigger1.get());

    window->set_rooms({ room.get() });
    window->set_triggers({ trigger1.get() });

    auto list = window->root_control()->find<ui::Listbox>(RoomsWindow::Names::rooms_listbox);
    ASSERT_NE(list, nullptr);

    auto row = list->find<ui::Control>(ui::Listbox::Names::row_name_format + "0");
    ASSERT_NE(row, nullptr);

    auto cell = row->find<ui::Button>(ui::Listbox::Row::Names::cell_name_format + "#");
    ASSERT_NE(cell, nullptr);
    cell->clicked(Point());

    auto triggers_list = window->root_control()->find<ui::Listbox>(RoomsWindow::Names::triggers_listbox);
    ASSERT_NE(triggers_list, nullptr);

    auto triggers_row = triggers_list->find<ui::Control>(ui::Listbox::Names::row_name_format + "0");
    ASSERT_NE(triggers_row, nullptr);

    auto triggers_cell = triggers_row->find<ui::Button>(ui::Listbox::Row::Names::cell_name_format + "#");
    ASSERT_NE(triggers_cell, nullptr);

    ASSERT_FALSE(triggers_list->selected_item().has_value());

    triggers_cell->clicked(Point());
    ASSERT_TRUE(triggers_list->selected_item().has_value());

    window->clear_selected_trigger();
    ASSERT_FALSE(triggers_list->selected_item().has_value());
}

TEST(RoomsWindow, SetTriggersClearsSelection)
{
    auto window = register_test_module().create<std::unique_ptr<RoomsWindow>>();

    std::optional<Trigger*> raised_trigger;
    auto token = window->on_trigger_selected += [&raised_trigger](const auto& trigger) { raised_trigger = trigger; };

    auto [trlevel_ptr, trlevel] = create_mock<trlevel::mocks::MockLevel>();
    auto [level_ptr, level] = create_mock<MockLevel>();
    auto [texture_storage_ptr, texture_storage] = create_mock<MockLevelTextureStorage>();
    auto [mesh_storage_ptr, mesh_storage] = create_mock<MockMeshStorage>();
    trlevel::tr3_room tr_room{};

    auto room = std::make_unique<Room>(
        [](auto, auto, auto, auto, auto) { return std::make_unique<MockMesh>(); },
        trlevel, tr_room, texture_storage, mesh_storage, 0, level);

    auto trigger1 = std::make_unique<Trigger>(0, 0, 100, 200, TriggerInfo{ 0, 0, 0, TriggerType::Trigger, 0, {  } }, [](auto, auto) { return std::make_unique<MockMesh>(); });
    room->add_trigger(trigger1.get());

    window->set_rooms({ room.get() });
    window->set_triggers({ trigger1.get() });

    auto list = window->root_control()->find<ui::Listbox>(RoomsWindow::Names::rooms_listbox);
    ASSERT_NE(list, nullptr);

    auto row = list->find<ui::Control>(ui::Listbox::Names::row_name_format + "0");
    ASSERT_NE(row, nullptr);

    auto cell = row->find<ui::Button>(ui::Listbox::Row::Names::cell_name_format + "#");
    ASSERT_NE(cell, nullptr);
    cell->clicked(Point());

    auto triggers_list = window->root_control()->find<ui::Listbox>(RoomsWindow::Names::triggers_listbox);
    ASSERT_NE(triggers_list, nullptr);

    auto triggers_row = triggers_list->find<ui::Control>(ui::Listbox::Names::row_name_format + "0");
    ASSERT_NE(triggers_row, nullptr);

    auto triggers_cell = triggers_row->find<ui::Button>(ui::Listbox::Row::Names::cell_name_format + "#");
    ASSERT_NE(triggers_cell, nullptr);

    ASSERT_FALSE(triggers_list->selected_item().has_value());

    triggers_cell->clicked(Point());
    ASSERT_TRUE(triggers_list->selected_item().has_value());

    window->set_triggers({});
    ASSERT_FALSE(triggers_list->selected_item().has_value());
}