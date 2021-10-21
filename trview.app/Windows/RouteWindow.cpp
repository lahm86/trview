#include "RouteWindow.h"
#include <trview.app/Routing/Route.h>
#include <trview.ui/GroupBox.h>
#include <trview.ui/Button.h>
#include <trview.common/Strings.h>
#include <trview.common/Windows/Clipboard.h>
#include <trview.ui/Layouts/StackLayout.h>
#include <trview.ui/Label.h>
#include <trview.ui/Layouts/GridLayout.h>

namespace trview
{
    using namespace ui;
    using namespace DirectX::SimpleMath;

    namespace
    {
        std::wstring pos_to_string(const Vector3& position)
        {
            return std::to_wstring(static_cast<int>(position.x * 1024)) + L", " +
                std::to_wstring(static_cast<int>(position.y * 1024)) + L", " + 
                std::to_wstring(static_cast<int>(position.z * 1024));
        }

        Listbox::Item make_item(const std::wstring& name, const std::wstring& value)
        {
            return Listbox::Item{ { { L"Name", name }, { L"Value", value } } };
        };

        bool has_randomizer_elements(const IRoute& route)
        {
            for (uint32_t i = 0u; i < route.waypoints(); ++i)
            {
                if (route.waypoint(i).type() == IWaypoint::Type::RandoLocation)
                {
                    return true;
                }
            }
            return false;
        }
    }

    const std::string RouteWindow::Names::export_button = "export_button";
    const std::string RouteWindow::Names::import_button = "import_button";
    const std::string RouteWindow::Names::clear_save = "clear_save";
    const std::string RouteWindow::Names::notes_area = "notes_area";
    const std::string RouteWindow::Names::select_save_button = "select_save_button";
    const std::string RouteWindow::Names::waypoint_stats = "waypoint_stats";
    const std::string RouteWindow::Names::requires_glitch = "requires_glitch";
    const std::string RouteWindow::Names::vehicle_required = "vehicle_required";
    const std::string RouteWindow::Names::is_item = "is_item";
    const std::string RouteWindow::Names::difficulty = "difficulty";

    namespace Colours
    {
        const Colour Divider{ 1.0f, 0.0f, 0.0f, 0.0f };
        const Colour LeftPanel{ 1.0f, 0.25f, 0.25f, 0.25f };
        const Colour ItemDetails{ 1.0f, 0.225f, 0.225f, 0.225f };
        const Colour Notes{ 1.0f, 0.20f, 0.20f, 0.20f };
        const Colour NotesTextArea{ 1.0f, 0.15f, 0.15f, 0.15f };
        const Colour DetailsBorder{ 0.0f, 0.0f, 0.0f, 0.0f };
    }

    using namespace graphics;

    RouteWindow::RouteWindow(const IDeviceWindow::Source& device_window_source, const ui::render::IRenderer::Source& renderer_source,
        const ui::IInput::Source& input_source, const trview::Window& parent, const std::shared_ptr<IClipboard>& clipboard,
        const std::shared_ptr<IDialogs>& dialogs, const std::shared_ptr<IFiles>& files, const IBubble::Source& bubble_source)
        : CollapsiblePanel(device_window_source, renderer_source(Size(470, 400)), parent, L"trview.route", L"Route", input_source, Size(470, 400)), _clipboard(clipboard), _dialogs(dialogs), _files(files),
        _bubble(bubble_source(*_ui))
    {
        CollapsiblePanel::on_window_closed += IRouteWindow::on_window_closed;
        set_panels(create_left_panel(), create_right_panel());
    }

    void RouteWindow::render(bool vsync)
    {
        CollapsiblePanel::render(vsync);
    }

    void RouteWindow::set_route(IRoute* route) 
    {
        _route = route;
        _selected_index = 0u;

        std::vector<Listbox::Item> items;
        for (uint32_t i = 0; i < _route->waypoints(); ++i)
        {
            items.push_back(create_listbox_item(i, _route->waypoint(i)));
        }
        _waypoints->set_items(items);
        load_waypoint_details(_selected_index);

        const auto colour = route->colour();
        _colour->set_text_colour(colour);
        _colour->set_text_background_colour(colour);
        _colour->set_selected_value(colour.name());
    }

    std::unique_ptr<Control> RouteWindow::create_left_panel()
    {
        auto left_panel = std::make_unique<ui::Window>(Size(200, window().size().height), Colours::LeftPanel);
        left_panel->set_layout(std::make_unique<StackLayout>(3.0f, StackLayout::Direction::Vertical, SizeMode::Manual));

        auto buttons = std::make_unique<ui::Window>(Size(200, 20), Colours::LeftPanel);
        buttons->set_layout(std::make_unique<StackLayout>(0.0f, StackLayout::Direction::Horizontal));

        _colour = buttons->add_child(std::make_unique<Dropdown>(Size(20, 20)));
        _colour->set_text_colour(Colour::Green);
        _colour->set_text_background_colour(Colour::Green);
        _colour->set_values(
            {
                Dropdown::Value { Colour::Green.name(), Colour::Green, Colour::Green },
                { Colour::Red.name(), Colour::Red, Colour::Red },
                { Colour::Blue.name(), Colour::Blue, Colour::Blue },
                { Colour::Yellow.name(), Colour::Yellow, Colour::Yellow },
                { Colour::Cyan.name(), Colour::Cyan, Colour::Cyan },
                { Colour::Magenta.name(), Colour::Magenta, Colour::Magenta },
                { Colour::Black.name(), Colour::Black, Colour::Black },
                { Colour::White.name(), Colour::White, Colour::White }
            });
        _colour->set_selected_value(Colour::Green.name());
        _colour->set_dropdown_scope(_ui.get());

        _token_store += _colour->on_value_selected += [=](const auto& value)
        {
            const auto new_colour = named_colour(value);
            _colour->set_text_colour(new_colour);
            _colour->set_text_background_colour(new_colour);
            on_colour_changed(new_colour);
        };

        auto import = buttons->add_child(std::make_unique<Button>(Size(90, 20), L"Import"));
        import->set_name(Names::import_button);
        _token_store += import->on_click += [&]()
        {
            std::vector<IDialogs::FileFilter> filters{ { L"trview route", { L"*.tvr" } } };
            if (_randomizer_enabled)
            {
                filters.push_back({ L"Randomizer Locations", { L"*.json" } });
            }

            const auto filename = _dialogs->open_file(L"Import route", filters, OFN_FILEMUSTEXIST);
            if (filename.has_value())
            {
                on_route_import(filename.value().filename, filename.value().filter_index == 2);
            }
        };
        auto export_button = buttons->add_child(std::make_unique<Button>(Size(90, 20), L"Export"));
        export_button->set_name(Names::export_button);
        _token_store += export_button->on_click += [&]()
        {
            std::vector<IDialogs::FileFilter> filters { { L"trview route", { L"*.tvr" } } };
            uint32_t filter_index = 1;
            if (_randomizer_enabled)
            {
                filters.push_back({ L"Randomizer Locations", { L"*.json" } });
                if (has_randomizer_elements(*_route))
                {
                    filter_index = 2;
                }
            }

            const auto filename = _dialogs->save_file(L"Export route", filters, filter_index);
            if (filename.has_value())
            {
                on_route_export(filename.value().filename, filename.value().filter_index == 2);
            }
        };
        auto _buttons = left_panel->add_child(std::move(buttons));

        // List box to show the waypoints in the route.
        auto waypoints = std::make_unique<Listbox>(Size(200, window().size().height - _buttons->size().height), Colours::LeftPanel);
        waypoints->set_enable_sorting(false);
        waypoints->set_columns(
            {
                { Listbox::Column::Type::Number, L"#", 30 },
                { Listbox::Column::Type::String, L"Type", 160 }
            }
        );
        _token_store += waypoints->on_item_selected += [&](const auto& item) {
            auto index = std::stoi(item.value(L"#"));
            load_waypoint_details(index);
            on_waypoint_selected(index);
        };
        _token_store += waypoints->on_delete += [&]() {
            on_waypoint_deleted(_selected_index);
        };

        _waypoints = left_panel->add_child(std::move(waypoints));
        return left_panel;
    }

    std::unique_ptr<Control> RouteWindow::create_right_panel()
    {
        const float panel_width = 270;
        auto right_panel = std::make_unique<ui::Window>(Size(panel_width, window().size().height), Colours::ItemDetails);
        auto right_panel_layout = std::make_unique<StackLayout>(0.0f, StackLayout::Direction::Vertical, SizeMode::Manual);
        right_panel_layout->set_margin(Size(0, 8));
        right_panel->set_layout(std::move(right_panel_layout));

        auto group_box = std::make_unique<GroupBox>(Size(panel_width, 160), Colours::ItemDetails, Colours::DetailsBorder, L"Waypoint Details");
        auto details_panel = std::make_unique<ui::Window>(Size(panel_width - 20, 140), Colours::ItemDetails);
        details_panel->set_layout(std::make_unique<StackLayout>(8.0f, StackLayout::Direction::Vertical, SizeMode::Manual));

        _stats = details_panel->add_child(std::make_unique<Listbox>(Size(panel_width - 20, 80), Colours::ItemDetails));
        _stats->set_name(Names::waypoint_stats);
        _stats->set_show_highlight(false);
        _stats->set_show_headers(false);
        _stats->set_show_scrollbar(true);
        _stats->set_columns(
            {
                { Listbox::Column::Type::String, L"Name", 100 },
                { Listbox::Column::Type::String, L"Value", 140 }
            });
        _token_store += _stats->on_item_selected += [&](const auto& item)
        {
            if (item.value(L"Name") == L"Room Position" || 
                item.value(L"Name") == L"Position")
            {
                _clipboard->write(window(), item.value(L"Value"));
                _bubble->show(client_cursor_position(window()) - Point(0, 20));
                return;
            }

            const auto index = _route->waypoint(_selected_index).index();
            switch (_selected_type)
            {
            case IWaypoint::Type::Entity:
                if (index < _all_items.size())
                {
                    on_item_selected(_all_items[index]);
                }
                break;
            case IWaypoint::Type::Trigger:
                if (index < _all_triggers.size())
                {
                    on_trigger_selected(_all_triggers[index]);
                }
                break;
            }
        };

        auto save_area = details_panel->add_child(std::make_unique<ui::Window>(Size(panel_width - 20, 20), Colours::ItemDetails));
        save_area->set_layout(std::make_unique<StackLayout>(0.0f, StackLayout::Direction::Vertical, SizeMode::Manual));

        _select_save = save_area->add_child(std::make_unique<Button>(Size(panel_width - 40, 20), L"Attach Save"));
        _select_save->set_name(Names::select_save_button);
        _token_store += _select_save->on_click += [&]()
        {
            if (!(_route && _selected_index < _route->waypoints()))
            {
                return;
            }

            if (!_route->waypoint(_selected_index).has_save())
            {
                const auto filename = _dialogs->open_file(L"Select Save", { { L"Savegame File", { L"*.*" } } }, OFN_FILEMUSTEXIST);
                if (filename.has_value())
                {
                    // Load bytes from file.
                    try
                    {
                        const auto bytes = _files->load_file(filename.value().filename);
                        if (bytes.has_value() && !bytes.value().empty())
                        {
                            _route->waypoint(_selected_index).set_save_file(bytes.value());
                            _route->set_unsaved(true);
                            _select_save->set_text(L"SAVEGAME.0");
                        }
                    }
                    catch (...)
                    {
                        _dialogs->message_box(window(), L"Failed to attach save", L"Error", IDialogs::Buttons::OK);
                    }
                }
            }
            else
            {
                const auto filename = _dialogs->save_file(L"Export Save", { { L"Savegame File", { L"*.*" } } }, 1);
                if (filename.has_value())
                {
                    try
                    {
                        _files->save_file(filename.value().filename, _route->waypoint(_selected_index).save_file());
                    }
                    catch (...)
                    {
                        _dialogs->message_box(window(), L"Failed to export save", L"Error", IDialogs::Buttons::OK);
                    }
                }
            }
        };

        _clear_save = save_area->add_child(std::make_unique<Button>(Size(20, 20), L"X"));
        _clear_save->set_name(Names::clear_save);
        _token_store += _clear_save->on_click += [&]()
        {
            if (!(_route && _selected_index < _route->waypoints()))
            {
                return;
            }

            auto& waypoint = _route->waypoint(_selected_index);
            if (waypoint.has_save())
            {
                waypoint.set_save_file({});
                _select_save->set_text(L"Attach Save");
                _route->set_unsaved(true);
            }
        };

        _delete_waypoint = details_panel->add_child(std::make_unique<Button>(Size(panel_width - 20, 20), L"Delete Waypoint"));
        _token_store += _delete_waypoint->on_click += [&]()
        {
            if (_route && _selected_index < _route->waypoints())
            {
                on_waypoint_deleted(_selected_index);
            }
        };

        _select_save->set_visible(false);
        _clear_save->set_visible(false);
        _delete_waypoint->set_visible(false);

        group_box->add_child(std::move(details_panel));
        right_panel->add_child(std::move(group_box));

        right_panel->add_child(std::make_unique<ui::Window>(Size(panel_width, 5), Colours::Notes));
        // Notes area.
        _lower_box = right_panel->add_child(std::make_unique<GroupBox>(Size(panel_width, window().size().height - 160), Colours::Notes, Colours::DetailsBorder, L"Notes"));
        _notes_area = _lower_box->add_child(std::make_unique<TextArea>(Size(panel_width - 20, _lower_box->size().height - 41), Colours::NotesTextArea, Colour(1.0f, 1.0f, 1.0f)));
        _notes_area->set_name(Names::notes_area);
        _notes_area->set_scrollbar_visible(true);

        _token_store += _notes_area->on_text_changed += [&](const std::wstring& text)
        {
            if (_route && _selected_index < _route->waypoints())
            {
                if (_route->waypoint(_selected_index).notes() != text)
                {
                    _route->waypoint(_selected_index).set_notes(text);
                    _route->set_unsaved(true);
                }
            }
        };

        _rando_area = _lower_box->add_child(std::make_unique<ui::Window>(Size(panel_width, window().size().height - 160), Colours::Notes));
        _rando_area->set_layout(std::make_unique<StackLayout>(5.0f));
        _rando_area->set_visible(false);

        auto grid = _rando_area->add_child(std::make_unique<ui::Window>(Size(panel_width, 50), Colours::Notes));
        grid->set_layout(std::make_unique<GridLayout>(2, 2));

        _requires_glitch = grid->add_child(std::make_unique<Checkbox>(Colour::Transparent, L"Requires Glitch"));
        _requires_glitch->set_name(Names::requires_glitch);
        _token_store += _requires_glitch->on_state_changed += [&](bool state)
        {
            if (_route && _selected_index < _route->waypoints())
            {
                _route->waypoint(_selected_index).set_requires_glitch(state);
                _route->set_unsaved(true);
            }
        };
        _vehicle_required = grid->add_child(std::make_unique<Checkbox>(Colour::Transparent, L"Vehicle Required"));
        _vehicle_required->set_name(Names::vehicle_required);
        _token_store += _vehicle_required->on_state_changed += [&](bool state)
        {
            if (_route && _selected_index < _route->waypoints())
            {
                _route->waypoint(_selected_index).set_vehicle_required(state);
                _route->set_unsaved(true);
            }
        };
        _is_item = grid->add_child(std::make_unique<Checkbox>(Colour::Transparent, L"Is Item"));
        _is_item->set_name(Names::is_item);
        _token_store += _is_item->on_state_changed += [&](bool state)
        {
            if (_route && _selected_index < _route->waypoints())
            {
                _route->waypoint(_selected_index).set_is_item(state);
                _route->set_unsaved(true);
            }
        };
        _difficulty = _rando_area->add_child(std::make_unique<Dropdown>(Size(panel_width / 2.0f, 24)));
        _difficulty->set_name(Names::difficulty);
        _difficulty->set_values({ L"Easy", L"Medium", L"Hard" });
        _difficulty->set_dropdown_scope(_ui.get());
        _token_store += _difficulty->on_value_selected += [&](const std::wstring& value)
        {
            if (_route && _selected_index < _route->waypoints())
            {
                _route->waypoint(_selected_index).set_difficulty(to_utf8(value));
                _route->set_unsaved(true);
            }
        };

        return right_panel;
    }

    Listbox::Item RouteWindow::create_listbox_item(uint32_t index, const IWaypoint& waypoint)
    {
        std::wstring type = waypoint_type_to_string(waypoint.type());
        if (waypoint.type() == IWaypoint::Type::Entity)
        {
            if (waypoint.index() < _all_items.size())
            {
                type = _all_items[waypoint.index()].type();
            }
            else
            {
                type = L"Invalid entity";
            }
        }
        else if (waypoint.type() == IWaypoint::Type::Trigger)
        {
            if (waypoint.index() < _all_triggers.size())
            {
                if (auto trigger = _all_triggers[waypoint.index()].lock())
                {
                    type = trigger_type_name(trigger->type());
                }
            }
            else
            {
                type = L"Invalid trigger";
            }
        }
        return { {{ L"#", std::to_wstring(index) },
                 { L"Type", type}} };
    }

    void RouteWindow::load_waypoint_details(uint32_t index)
    {
        if (index >= _route->waypoints())
        {
            _stats->set_items({});
            _notes_area->set_text(L"");
            _select_save->set_visible(false);
            _clear_save->set_visible(false);
            _delete_waypoint->set_visible(false);
            return;
        }

        _select_save->set_visible(true);
        _clear_save->set_visible(true);
        _delete_waypoint->set_visible(true);

        const auto& waypoint = _route->waypoint(index);

        auto get_room_pos = [&waypoint, this]()
        {
            if (waypoint.room() < _all_rooms.size())
            {
                const auto room = _all_rooms[waypoint.room()].lock();
                if (!room)
                {
                    return waypoint.position();
                }
                const auto info = room->info();
                const Vector3 bottom_left = Vector3(info.x, info.yBottom, info.z) / trlevel::Scale_X;
                return waypoint.position() - bottom_left;
            }
            return waypoint.position();
        };

        
        std::vector<Listbox::Item> stats;
        stats.push_back(make_item(L"Type", waypoint_type_to_string(waypoint.type())));
        stats.push_back(make_item(L"Position", pos_to_string(waypoint.position())));
        stats.push_back(make_item(L"Room", std::to_wstring(waypoint.room())));
        stats.push_back(make_item(L"Room Position", pos_to_string(get_room_pos())));

        _selected_type = waypoint.type();
        _selected_index = index;

        if (waypoint.type() == IWaypoint::Type::Entity || waypoint.type() == IWaypoint::Type::Trigger)
        {
            stats.push_back(make_item(L"Target Index", std::to_wstring(waypoint.index())));
            if (waypoint.type() == IWaypoint::Type::Entity)
            {
                std::wstring type = L"Invalid entity";
                if (waypoint.index() < _all_items.size())
                {
                    type = _all_items[waypoint.index()].type();
                }
                stats.push_back(make_item(L"Entity", type));
            }
            else if (waypoint.type() == IWaypoint::Type::Trigger)
            {
                std::wstring type = L"Invalid trigger";
                if (waypoint.index() < _all_triggers.size())
                {
                    if (auto trigger = _all_triggers[waypoint.index()].lock())
                    {
                        type = trigger_type_name(trigger->type());
                    }
                }
                stats.push_back(make_item(L"Trigger Type", type));
            }
        }

        _stats->set_items(stats);

        // Handling for randomizer features:
        if (waypoint.type() == IWaypoint::Type::RandoLocation)
        {
            _rando_area->set_visible(true);
            _notes_area->set_visible(false);
            _lower_box->set_title(L"Randomizer");

            _requires_glitch->set_state(waypoint.requires_glitch());
            _difficulty->set_selected_value(to_utf16(waypoint.difficulty()));
            _vehicle_required->set_state(waypoint.vehicle_required());
            _is_item->set_state(waypoint.is_item());
        }
        else
        {
            _rando_area->set_visible(false);
            _notes_area->set_visible(true);
            _lower_box->set_title(L"Notes");

            _notes_area->set_text(waypoint.notes());

            if (waypoint.has_save())
            {
                _select_save->set_text(L"SAVEGAME.0");
            }
            else
            {
                _select_save->set_text(L"Attach Save");
            }
        }
    }

    void RouteWindow::select_waypoint(uint32_t index)
    {
        _selected_index = index;

        if (!_route)
        {
            return;
        }

        if (index < _route->waypoints())
        {
            _waypoints->set_selected_item(create_listbox_item(index, _route->waypoint(index)));
            load_waypoint_details(index);
        }
    }

    /// Set the items to that are in the level.
    /// @param items The items to show.
    void RouteWindow::set_items(const std::vector<Item>& items)
    {
        _all_items = items;
    }

    void RouteWindow::set_rooms(const std::vector<std::weak_ptr<IRoom>>& rooms)
    {
        _all_rooms = rooms;
    }

    void RouteWindow::set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers)
    {
        _all_triggers = triggers;
    }

    void RouteWindow::focus()
    {
        SetForegroundWindow(window());
    }

    void RouteWindow::update(float delta)
    {
        _ui->update(delta);
    }

    void RouteWindow::set_randomizer_enabled(bool value)
    {
        _randomizer_enabled = value;
    }
}
