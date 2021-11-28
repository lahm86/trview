#include "ItemsWindow.h"
#include <trview.app/Resources/resource.h>
#include <trview.ui/Checkbox.h>
#include <trview.ui/Button.h>
#include <trview.common/Strings.h>
#include <trview.common/Windows/Clipboard.h>

using namespace trview::graphics;

namespace trview
{
    namespace
    {
        const float Height{ 420 };

        ui::Listbox::Item create_listbox_item(const Item& item)
        {
            return { {{ L"#", std::to_wstring(item.number()) },
                     { L"ID", std::to_wstring(item.type_id()) },
                     { L"Room", std::to_wstring(item.room()) },
                     { L"Type", item.type() },
                     { L"Hide", std::to_wstring(!item.visible()) }} };
        }
    }

    const std::string ItemsWindow::Names::add_to_route_button{ "AddToRoute" };
    const std::string ItemsWindow::Names::items_listbox{ "Items" };
    const std::string ItemsWindow::Names::stats_listbox{ "Stats" };
    const std::string ItemsWindow::Names::sync_item_checkbox{ "SyncItem" };
    const std::string ItemsWindow::Names::track_room_checkbox{ "TrackRoom" };
    const std::string ItemsWindow::Names::triggers_listbox{ "Triggers" };
    const std::string ItemsWindow::Names::expander{ "Expander" };

    ItemsWindow::ItemsWindow(const IDeviceWindow::Source& device_window_source, const ui::render::IRenderer::Source& renderer_source, const ui::IInput::Source& input_source, const Window& parent,
        const std::shared_ptr<IClipboard>& clipboard, const IBubble::Source& bubble_source, const ui::UiSource& ui_source)
        : CollapsiblePanel(device_window_source, renderer_source(Size(450, Height)), parent, L"trview.items", L"Items", input_source, Size(450, Height)), _clipboard(clipboard),
        _bubble(bubble_source(*_ui))
    {
        CollapsiblePanel::on_window_closed += IItemsWindow::on_window_closed;
        set_panels(create_left_panel(ui_source), create_right_panel(ui_source));
    }

    void ItemsWindow::set_items(const std::vector<Item>& items)
    {
        _all_items = items;
        populate_items(items);
    }

    void ItemsWindow::update_items(const std::vector<Item>& items)
    {
        set_items(items);
        if (_track_room)
        {
            _filter_applied = false;
            set_current_room(_current_room);
        }
    }

    void ItemsWindow::set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers)
    {
        _all_triggers = triggers;
        _trigger_list->set_items({});
    }

    void ItemsWindow::clear_selected_item()
    {
        _selected_item.reset();
        _stats_list->set_items({});
        _trigger_list->set_items({});
    }

    void ItemsWindow::populate_items(const std::vector<Item>& items)
    {
        using namespace ui;
        std::vector<Listbox::Item> list_items;
        std::transform(items.begin(), items.end(), std::back_inserter(list_items), create_listbox_item);
        _items_list->set_items(list_items);
    }

    void ItemsWindow::set_current_room(uint32_t room)
    {
        if (_track_room && (!_filter_applied || _current_room != room))
        {
            _filter_applied = true;

            std::vector<Item> filtered_items;
            for (const auto& item : _all_items)
            {
                if (item.room() == room)
                {
                    filtered_items.push_back(item);
                }
            }
            populate_items(filtered_items);
        }

        _current_room = room;
    }

    void ItemsWindow::update_layout()
    {
        CollapsiblePanel::update_layout();
        _items_list->set_size(Size(_items_list->size().width, _left_panel->size().height - _items_list->position().y));
    }

    std::unique_ptr<ui::Control> ItemsWindow::create_left_panel(const ui::UiSource& ui_source)
    {
        using namespace ui;

        auto left_panel = ui_source(IDR_UI_ITEMS_WINDOW_LEFT_PANEL);
        _track_room_checkbox = left_panel->find<Checkbox>(Names::track_room_checkbox);
        _token_store += _track_room_checkbox->on_state_changed += [this](bool value)
        {
            set_track_room(value);
        };

        auto sync_item = left_panel->find<Checkbox>(Names::sync_item_checkbox);
        sync_item->set_state(_sync_item);
        _token_store += sync_item->on_state_changed += [this](bool value) { set_sync_item(value); };

        set_expander(left_panel->find<Button>(Names::expander));

        _items_list = left_panel->find<Listbox>(Names::items_listbox);
        _token_store += _items_list->on_item_selected += [&](const auto& item)
        {
            auto index = std::stoi(item.value(L"#"));
            load_item_details(_all_items[index]);
            if (_sync_item)
            {
                on_item_selected(_all_items[index]);
            }
        };
        _token_store += _items_list->on_state_changed += [&](const auto& item, const std::wstring& column, bool state)
        {
            if (column == L"Hide")
            {
                auto index = std::stoi(item.value(L"#"));
                on_item_visibility(_all_items[index], !state);
            }
        };

        // Fix items list size now that it has been added to the panel.
        _items_list->set_size(Size(250, left_panel->size().height - _items_list->position().y));

        return left_panel;
    }

    std::unique_ptr<ui::Control> ItemsWindow::create_right_panel(const ui::UiSource& ui_source)
    {
        using namespace ui;

        auto right_panel = ui_source(IDR_UI_ITEMS_WINDOW_RIGHT_PANEL);
        _stats_list = right_panel->find<Listbox>(Names::stats_listbox);
        _token_store += _stats_list->on_item_selected += [this](const ui::Listbox::Item& item)
        {
            _clipboard->write(window(), item.value(L"Value"));
            _bubble->show(client_cursor_position(window()) - Point(0, 20));
        };

        auto add_to_route = right_panel->find<Button>(Names::add_to_route_button);
        _token_store += add_to_route->on_click += [&]()
        {
            if (_selected_item.has_value())
            {
                on_add_to_route(_selected_item.value());
            }
        };

        _trigger_list = right_panel->find<Listbox>(Names::triggers_listbox);
        _token_store += _trigger_list->on_item_selected += [&](const auto& item)
        {
            auto index = std::stoi(item.value(L"#"));
            set_track_room(false);
            on_trigger_selected(_all_triggers[index]);
        };

        return right_panel;
    }

    void ItemsWindow::load_item_details(const Item& item)
    {
        auto make_item = [](const auto& name, const auto& value)
        {
            return Listbox::Item { { { L"Name", name }, { L"Value", value } } };
        };

        auto position_text = [&item]()
        {
            std::wstringstream stream;
            stream << std::fixed << std::setprecision(0) <<
                item.position().x * trlevel::Scale_X << L", " <<
                item.position().y * trlevel::Scale_Y << L", " <<
                item.position().z * trlevel::Scale_Z;
            return stream.str();
        };

        using namespace ui;
        std::vector<Listbox::Item> stats;
        stats.push_back(make_item(L"Type", item.type()));
        stats.push_back(make_item(L"#", std::to_wstring(item.number())));
        stats.push_back(make_item(L"Position", position_text()));
        stats.push_back(make_item(L"Type ID", std::to_wstring(item.type_id())));
        stats.push_back(make_item(L"Room", std::to_wstring(item.room())));
        stats.push_back(make_item(L"Clear Body", format_bool(item.clear_body_flag())));
        stats.push_back(make_item(L"Invisible", format_bool(item.invisible_flag())));
        stats.push_back(make_item(L"Flags", format_binary(item.activation_flags())));
        stats.push_back(make_item(L"OCB", std::to_wstring(item.ocb())));
        _stats_list->set_items(stats);

        std::vector<Listbox::Item> triggers;
        for (auto& trigger : item.triggers())
        {
            auto trigger_ptr = trigger.lock();
            if (!trigger_ptr)
            {
                continue;
            }
            triggers.push_back(
                {
                    {
                        { L"#", std::to_wstring(trigger_ptr->number()) },
                        { L"Room", std::to_wstring(trigger_ptr->room()) },
                        { L"Type", trigger_type_name(trigger_ptr->type()) },
                    }
                });
        }
        _trigger_list->set_items(triggers);
    }

    void ItemsWindow::set_track_room(bool value)
    {
        if (_track_room != value)
        {
            _track_room = value;
            if (_track_room)
            {
                set_current_room(_current_room);
            }
            else
            {
                _filter_applied = false;
                set_items(_all_items);
            }
        }

        if (_track_room_checkbox->state() != _track_room)
        {
            _track_room_checkbox->set_state(_track_room);
        }
    }

    void ItemsWindow::set_sync_item(bool value)
    {
        if (_sync_item != value)
        {
            _sync_item = value;
            if (_selected_item.has_value())
            {
                set_selected_item(_selected_item.value());
            }
        }
    }

    void ItemsWindow::set_selected_item(const Item& item)
    {
        _selected_item = item;
        if (_sync_item)
        {
            const auto& list_item = create_listbox_item(item);
            if (_items_list->set_selected_item(list_item))
            {
                load_item_details(item);
            }
            else
            {
                _selected_item.reset();
            }
        }
    }

    std::optional<Item> ItemsWindow::selected_item() const
    {
        return _selected_item;
    }

    void ItemsWindow::render(bool vsync)
    {
        CollapsiblePanel::render(vsync);
    }

    void ItemsWindow::update(float delta)
    {
        _ui->update(delta);
    }
}
