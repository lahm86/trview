#include "RoomsWindow.h"
#include <trview.app/Elements/IRoom.h>
#include <trview.app/Elements/Item.h>
#include <trview.app/Elements/ITrigger.h>
#include <trview.common/Strings.h>
#include "../trview_imgui.h"

namespace trview
{
    namespace
    {
        void add_room_flags(IClipboard& clipboard, trlevel::LevelVersion version, const IRoom& room)
        {
            using trlevel::LevelVersion;
            const auto add_flag = [&](const std::string& name, bool flag) 
            {
                auto value = std::format("{}", flag);
                ImGui::TableNextColumn();
                if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
                {
                    clipboard.write(to_utf16(value));
                }
                ImGui::TableNextColumn();
                ImGui::Text(value.c_str());
            };
            const auto add_flag_min = [&](LevelVersion min_version, const std::string& name, const std::string& alt_name, bool flag)
            {
                add_flag(version >= min_version ? name : alt_name, flag);
            };

            add_flag("Water", room.water());
            add_flag("Bit 1", room.flag(IRoom::Flag::Bit1));
            add_flag("Bit 2", room.flag(IRoom::Flag::Bit2));
            add_flag_min(LevelVersion::Tomb2, "Outside / 3", "Bit 3", room.outside());
            add_flag("Bit 4", room.flag(IRoom::Flag::Bit4));
            add_flag_min(LevelVersion::Tomb2, "Wind / 5", "Bit 5", room.flag(IRoom::Flag::Wind));
            add_flag("Bit 6", room.flag(IRoom::Flag::Bit6));
            if (version == LevelVersion::Tomb3)
            {
                add_flag("Quicksand / 7", room.flag(IRoom::Flag::Quicksand));
            }
            else if (version > LevelVersion::Tomb3)
            {
                add_flag("Block Lens Flare / 7", room.flag(IRoom::Flag::NoLensFlare));
            }
            else
            {
                add_flag("Bit 7", room.flag(IRoom::Flag::Bit7));
            }
            add_flag_min(LevelVersion::Tomb3, "Caustics / 8", "Bit 8", room.flag(IRoom::Flag::Caustics));
            add_flag_min(LevelVersion::Tomb3, "Reflectivity / 9", "Bit 9", room.flag(IRoom::Flag::WaterReflectivity));
            add_flag_min(LevelVersion::Tomb4, "Snow (NGLE) / 10", "Bit 10", room.flag(IRoom::Flag::Bit10));
            add_flag_min(LevelVersion::Tomb4, "D / Rain / 11", "Bit 11", room.flag(IRoom::Flag::Bit11));
            add_flag_min(LevelVersion::Tomb4, "P / Cold / 12", "Bit 12", room.flag(IRoom::Flag::Bit12));
            add_flag("Bit 13", room.flag(IRoom::Flag::Bit13));
            add_flag("Bit 14", room.flag(IRoom::Flag::Bit14));
            add_flag("Bit 15", room.flag(IRoom::Flag::Bit15));
        }
    }

    RoomsWindow::RoomsWindow(const IMapRenderer::Source& map_renderer_source, const std::shared_ptr<IClipboard>& clipboard)
        : _map_renderer(map_renderer_source(Size(341, 341))), _clipboard(clipboard)
    {
        _map_renderer->set_render_mode(IMapRenderer::RenderMode::Texture);
        _token_store += _map_renderer->on_sector_hover += [this](const std::shared_ptr<ISector>& sector)
        {
            if (!sector)
            {
                _map_tooltip.set_visible(false);
                return;
            }

            std::string text = std::format("X: {}, Z: {}\n", sector->x(), sector->z());
            if (has_flag(sector->flags(), SectorFlag::RoomAbove))
            {
                text += std::format("Above: {}", sector->room_above());
            }
            if (has_flag(sector->flags(), SectorFlag::RoomBelow))
            {
                text += std::format("{}Below: {}", has_flag(sector->flags(), SectorFlag::RoomAbove) ? ", " : "", sector->room_below());
            }
            _map_tooltip.set_text(text);
            _map_tooltip.set_visible(!text.empty());
        };

        generate_filters();
    }

    void RoomsWindow::set_current_room(uint32_t room)
    {
        if (room == _current_room)
        {
            return;
        }

        _current_room = room;
        _scroll_to_room = true;
        if (_sync_room && _current_room < _all_rooms.size())
        {
            _selected_room = _current_room;
            load_room_details(_current_room);
        }
    }

    void RoomsWindow::load_room_details(uint32_t room_number)
    {
        const auto room = std::find_if(_all_rooms.begin(), _all_rooms.end(),
            [&](auto&& r)
            {
                const auto locked = r.lock();
                return locked && locked->number() == room_number;
            });
        if (room != _all_rooms.end())
        {
            _map_renderer->load(room->lock());
        }
    }

    void RoomsWindow::set_items(const std::vector<Item>& items)
    {
        _all_items = items;
        _global_selected_item.reset();
        _local_selected_item.reset();
    }

    void RoomsWindow::set_level_version(trlevel::LevelVersion version)
    {
        _level_version = version;
    }

    void RoomsWindow::set_map_colours(const MapColours& colours)
    {
        _map_renderer->set_colours(colours);
    }

    void RoomsWindow::set_rooms(const std::vector<std::weak_ptr<IRoom>>& rooms)
    {
        _all_rooms = rooms;
        _current_room = 0xffffffff;
        generate_filters();
    }

    void RoomsWindow::set_selected_item(const Item& item)
    {
        _global_selected_item = item;
        if (_track_item)
        {
            _local_selected_item = item;
            _selected_room = item.room();
            _scroll_to_room = true;
            _scroll_to_item = true;
            if (!_sync_room)
            {
                load_room_details(item.room());
            }
        }
    }

    void RoomsWindow::set_selected_trigger(const std::weak_ptr<ITrigger>& trigger)
    {
        _global_selected_trigger = trigger;
        if (_track_trigger)
        {
            _local_selected_trigger = trigger;
            if (const auto trigger_ptr = trigger.lock())
            {
                _selected_room = trigger_ptr->room();
                _scroll_to_room = true;
                _scroll_to_trigger = true;
                if (!_sync_room)
                {
                    load_room_details(trigger_ptr->room());
                }
            }
        }
    }

    void RoomsWindow::set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers)
    {
        _global_selected_trigger.reset();
        _local_selected_trigger.reset();
        _all_triggers = triggers;
    }

    void RoomsWindow::set_sync_room(bool value)
    {
        if (_sync_room != value)
        {
            _sync_room = value;
            set_current_room(_current_room);
        }
    }

    void RoomsWindow::set_track_item(bool value)
    {
        if (_track_item != value)
        {
            _track_item = value;
            if (_track_item && _global_selected_item.has_value())
            {
                set_selected_item(_global_selected_item.value());
            }
        }
    }

    void RoomsWindow::set_track_trigger(bool value)
    {
        if (_track_trigger != value)
        {
            _track_trigger = value;
            set_selected_trigger(_global_selected_trigger);
        }
    }

    void RoomsWindow::render()
    {
        if (!render_rooms_window())
        {
            on_window_closed();
            return;
        }
    }

    bool RoomsWindow::render_rooms_window()
    {
        bool stay_open = true;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(620, 760));
        if (ImGui::Begin(_id.c_str(), &stay_open))
        {
            render_rooms_list();
            ImGui::SameLine();
            render_room_details();

            if (_tooltip_timer.has_value())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Copied");
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        _map_tooltip.render();
        return stay_open;
    }

    void RoomsWindow::render_rooms_list()
    {
        if (ImGui::BeginChild(Names::rooms_panel.c_str(), ImVec2(270, 0), true))
        {
            if (ImGui::BeginTable("##controls", 2, 0, ImVec2(-1, 0)))
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                _filters.render();
                ImGui::TableNextColumn();

                bool sync_room = _sync_room;
                if (ImGui::Checkbox("Sync##syncroom", &sync_room))
                {
                    set_sync_room(sync_room);
                }
                ImGui::TableNextColumn();

                bool track_item = _track_item;
                if (ImGui::Checkbox("Track Item##trackitem", &track_item))
                {
                    set_track_item(track_item);
                }
                ImGui::TableNextColumn();

                bool track_trigger = _track_trigger;
                if (ImGui::Checkbox("Track Trigger##tracktrigger", &track_trigger))
                {
                    set_track_trigger(track_trigger);
                }
                ImGui::EndTable();
            }

            if (ImGui::BeginTable(Names::rooms_list.c_str(), 4, ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedSame, ImVec2(-1, -1)))
            {
                ImGui::TableSetupColumn("#");
                ImGui::TableSetupColumn("Items");
                ImGui::TableSetupColumn("Triggers");
                ImGui::TableSetupColumn("Hide");
                ImGui::TableSetupScrollFreeze(1, 1);
                ImGui::TableHeadersRow();

                auto item_count = [&](const IRoom& room)
                {
                    return std::count_if(_all_items.begin(), _all_items.end(), [&room](const auto& item) { return item.room() == room.number(); });
                };

                auto trigger_count = [&](const IRoom& room)
                {
                    return std::count_if(_all_triggers.begin(), _all_triggers.end(), [&room](const auto& trigger) { return trigger.lock()->room() == room.number(); });
                };

                imgui_sort_weak(_all_rooms,
                    {
                        [](auto&& l, auto&& r) { return l.number() < r.number(); },
                        [&](auto&& l, auto&& r) { return item_count(l) < item_count(r); },
                        [&](auto&& l, auto&& r) { return trigger_count(l) < trigger_count(r); },
                        [&](auto&& l, auto&& r) { return l.visible() < r.visible(); }
                    });

                for (const auto& room : _all_rooms)
                {
                    auto room_ptr = room.lock();

                    if (!_filters.match(*room_ptr))
                    {
                        continue;
                    }

                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    bool selected = room_ptr->number() == _selected_room;

                    ImGuiScroller scroller;
                    if (selected && _scroll_to_room)
                    {
                        scroller.scroll_to_item();
                        _scroll_to_room = false;
                    }

                    if (ImGui::Selectable(std::format("{0}##{0}", room_ptr->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_SelectOnNav))
                    {
                        scroller.fix_scroll();
                        _selected_room = room_ptr->number();
                        _map_renderer->load(room_ptr);
                        if (_sync_room)
                        {
                            on_room_selected(_selected_room);
                        }
                        _scroll_to_room = false;
                    }
                    ImGui::SetItemAllowOverlap();
                    ImGui::TableNextColumn();
                    ImGui::Text(std::to_string(item_count(*room_ptr)).c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text(std::to_string(trigger_count(*room_ptr)).c_str());
                    ImGui::TableNextColumn();
                    bool hidden = !room_ptr->visible();
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    if (ImGui::Checkbox(std::format("##hide-{}", room_ptr->number()).c_str(), &hidden))
                    {
                        on_room_visibility(room, !hidden);
                    }
                    ImGui::PopStyleVar();
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }

    void RoomsWindow::render_room_details()
    {
        auto current_room = [&]() -> std::weak_ptr<IRoom>
        {
            for (const auto& room : _all_rooms)
            {
                if (room.lock()->number() == _selected_room)
                {
                    return room;
                }
            }
            return {};
        };


        if (ImGui::BeginChild(Names::details_panel.c_str(), ImVec2(), true))
        {
            if (_selected_room < _all_rooms.size())
            {
                auto room = current_room().lock();
                if (room)
                {
                    if (_map_renderer->loaded())
                    {
                        _map_renderer->render();
                        _map_texture = _map_renderer->texture();
                        if (!_map_texture.has_content())
                        {
                            return;
                        }
                        _map_renderer->set_window_size(_map_texture.size());

                        float remainder = (341 - _map_texture.size().height) * 0.5f;
                        ImGui::SetCursorPosX(std::round((ImGui::GetWindowSize().x - _map_texture.size().width) * 0.5f));
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::floor(remainder));
                        
                        const auto io = ImGui::GetIO();
                        if (io.WantCaptureMouse)
                        {
                            const auto mouse_pos = io.MousePos;
                            auto top_left = ImGui::GetCursorScreenPos();
                            auto adjusted = mouse_pos - top_left;
                            _map_renderer->set_cursor_position(Point(adjusted.x, adjusted.y));

                            auto sector = _map_renderer->sector_at_cursor();
                            if (sector)
                            {
                                if (io.MouseClicked[0])
                                {
                                    if (has_flag(sector->flags(), SectorFlag::Portal))
                                    {
                                        on_room_selected(sector->portal());
                                    }
                                    else if (sector->room_below() != 0xff)
                                    {
                                        on_room_selected(sector->room_below());
                                    }
                                    else
                                    {
                                        // Select triggers
                                        for (const auto& trigger : _all_triggers)
                                        {
                                            auto trigger_ptr = trigger.lock();
                                            if (trigger_ptr && trigger_ptr->room() == _current_room && trigger_ptr->sector_id() == sector->id())
                                            {
                                                on_trigger_selected(trigger);
                                                break;
                                            }
                                        }
                                    }
                                }
                                else if (io.MouseClicked[1])
                                {
                                    if (sector->room_above() != 0xff)
                                    {
                                        on_room_selected(sector->room_above());
                                    }
                                }
                            }
                        }
                        ImGui::Image(_map_texture.view().Get(), ImVec2(_map_texture.size().width, _map_texture.size().height));
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::ceil(remainder));
                        ImGui::Separator();
                    }

                    auto add_stat = [&]<typename T>(const std::string& name, const T&& value)
                    {
                        const auto string_value = get_string(value);
                        ImGui::TableNextColumn();
                        if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            _clipboard->write(to_utf16(string_value));
                            _tooltip_timer = 0.0f;
                        }
                        ImGui::TableNextColumn();
                        ImGui::Text(string_value.c_str());
                    };

                    if (ImGui::BeginTable(Names::bottom.c_str(), 2))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();

                        ImGui::Text("Properties");
                        if (ImGui::BeginTable(Names::properties.c_str(), 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 150)))
                        {
                            ImGui::TableSetupColumn("Name");
                            ImGui::TableSetupColumn("Value");
                            ImGui::TableNextRow();

                            add_stat("X", room->info().x);
                            add_stat("Y", room->info().yBottom);
                            add_stat("Z", room->info().z);
                            if (room->alternate_mode() != Room::AlternateMode::None)
                            {
                                add_stat("Alternate", room->alternate_room());
                                if (room->alternate_group() != 0xff)
                                {
                                    add_stat("Alternate Group", room->alternate_group());
                                }
                            }
                            add_room_flags(*_clipboard, _level_version, *room);
                            ImGui::EndTable();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text("Items");
                        if (ImGui::BeginTable("Items", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 150)))
                        {
                            ImGui::TableSetupColumn("#");
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupScrollFreeze(1, 1);
                            ImGui::TableHeadersRow();

                            imgui_sort(_all_items,
                                {
                                    [](auto&& l, auto&& r) { return l.number() < r.number(); },
                                    [](auto&& l, auto&& r) { return l.type() < r.type(); },
                                });
                            
                            for (const auto& item : _all_items)
                            {
                                if (item.room() == room->number())
                                {
                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();
                                    bool selected = _local_selected_item.has_value() && _local_selected_item.value().number() == item.number();
                                    
                                    ImGuiScroller scroller;
                                    if (selected && _scroll_to_item)
                                    {
                                        scroller.scroll_to_item();
                                        _scroll_to_item = false;
                                    }

                                    if (ImGui::Selectable(std::to_string(item.number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_SelectOnNav))
                                    {
                                        scroller.fix_scroll();
                                        _local_selected_item = item;
                                        on_item_selected(item);
                                        _scroll_to_item = false;
                                    }

                                    ImGui::TableNextColumn();
                                    ImGui::Text(item.type().c_str());
                                }
                            }

                            ImGui::EndTable();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text("Neighbours");
                        if (ImGui::BeginTable("Neighbours", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 150)))
                        {
                            ImGui::TableSetupColumn("#");
                            ImGui::TableSetupScrollFreeze(1, 1);
                            ImGui::TableHeadersRow();

                            for (auto& neighbour : room->neighbours())
                            {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                bool selected = false;
                                if (ImGui::Selectable(std::to_string(neighbour).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_SelectOnNav))
                                {
                                    _selected_room = neighbour;
                                    if (_sync_room)
                                    {
                                        on_room_selected(neighbour);
                                    }
                                }
                            }

                            ImGui::EndTable();
                        }

                        ImGui::TableNextColumn();
                        ImGui::Text("Triggers");
                        if (ImGui::BeginTable("Triggers", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit, ImVec2(0, 150)))
                        {
                            ImGui::TableSetupColumn("#");
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupScrollFreeze(1, 1);
                            ImGui::TableHeadersRow();

                            imgui_sort_weak(_all_triggers,
                                {
                                    [](auto&& l, auto&& r) { return l.number() < r.number(); },
                                    [&](auto&& l, auto&& r) { return l.type() < r.type(); }
                                });

                            for (const auto& trigger : _all_triggers)
                            {
                                const auto trigger_ptr = trigger.lock();
                                if (trigger_ptr->room() == room->number())
                                {
                                    ImGui::TableNextRow();
                                    ImGui::TableNextColumn();
                                    const auto local_selection = _local_selected_trigger.lock();
                                    bool selected = local_selection && local_selection->number() == trigger_ptr->number();
                                    
                                    ImGuiScroller scroller;
                                    if (selected && _scroll_to_trigger)
                                    {
                                        scroller.scroll_to_item();
                                        _scroll_to_trigger = false;
                                    }

                                    if (ImGui::Selectable(std::format("{0}##{0}", trigger_ptr->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_SelectOnNav))
                                    {
                                        scroller.fix_scroll();
                                        _local_selected_trigger = trigger_ptr;
                                        on_trigger_selected(trigger);
                                        _scroll_to_trigger = false;
                                    }

                                    ImGui::TableNextColumn();
                                    ImGui::Text(trigger_type_name(trigger_ptr->type()).c_str());
                                }
                            }

                            ImGui::EndTable();
                        }

                        ImGui::EndTable();
                    }
                }
            }
        }
        ImGui::EndChild();
    }

    void RoomsWindow::clear_selected_trigger()
    {
        _local_selected_trigger.reset();
        _global_selected_trigger.reset();
    }

    void RoomsWindow::update(float delta)
    {
        if (_tooltip_timer.has_value())
        {
            _tooltip_timer = _tooltip_timer.value() + delta;
            if (_tooltip_timer.value() > 0.6f)
            {
                _tooltip_timer.reset();
            }
        }
    }

    void RoomsWindow::set_number(int32_t number)
    {
        _id = "Rooms " + std::to_string(number);
    }

    void RoomsWindow::generate_filters()
    {
        _filters.clear_all_getters();
        _filters.add_getter<float>("X", [](auto&& room) { return room.info().x; });
        _filters.add_getter<float>("Y", [](auto&& room) { return room.info().yBottom; });
        _filters.add_getter<float>("Z", [](auto&& room) { return room.info().z; });
        _filters.add_multi_getter<float>("Neighbours", [](auto&& room)
            {
                std::vector<float> results;
                const auto neighbours = room.neighbours();
                std::copy(neighbours.begin(), neighbours.end(), std::back_inserter(results));
                return results;
            });
        _filters.add_multi_getter<float>("Trigger Index", [&](auto&& room)
            {
                std::vector<float> results;
                for (const auto& trigger : _all_triggers)
                {
                    const auto trigger_ptr = trigger.lock();
                    if (trigger_ptr && trigger_ptr->room() == room.number())
                    {
                        results.push_back(trigger_ptr->number());
                    }
                }
                return results;
            });

        std::set<std::string> available_trigger_types;
        for (const auto& trigger : _all_triggers)
        {
            if (auto trigger_ptr = trigger.lock())
            {
                available_trigger_types.insert(trigger_type_name(trigger_ptr->type()));
            }
        }
        _filters.add_multi_getter<std::string>("Trigger Type", { available_trigger_types.begin(), available_trigger_types.end() }, [&](auto&& room)
            {
                std::vector<std::string> results;
                for (const auto& trigger : _all_triggers)
                {
                    const auto trigger_ptr = trigger.lock();
                    if (trigger_ptr && trigger_ptr->room() == room.number())
                    {
                        results.push_back(trigger_type_name(trigger_ptr->type()));
                    }
                }
                return results;
            });
        _filters.add_multi_getter<float>("Item Index", [&](auto&& room)
            {
                std::vector<float> results;
                for (const auto& item : _all_items)
                {
                    if (item.room() == room.number())
                    {
                        results.push_back(item.number());
                    }
                }
                return results;
            });

        std::set<std::string> available_item_types;
        for (const auto& item : _all_items)
        {
            available_item_types.insert(item.type());
        }
        _filters.add_multi_getter<std::string>("Item Type", { available_item_types.begin(), available_item_types.end() }, [&](auto&& room)
            {
                std::vector<std::string> results;
                for (const auto& item : _all_items)
                {
                    if (item.room() == room.number())
                    {
                        results.push_back(item.type());
                    }
                }
                return results;
            });
        _filters.add_getter<bool>("Water", [](auto&& room) { return room.water(); });
        _filters.add_getter<bool>("Bit 1", [](auto&& room) { return room.flag(IRoom::Flag::Bit1); });
        _filters.add_getter<bool>("Bit 2", [](auto&& room) { return room.flag(IRoom::Flag::Bit2); });
        _filters.add_getter<bool>("Outside/Bit 3", [](auto&& room) { return room.outside(); });
        _filters.add_getter<bool>("Bit 4", [](auto&& room) { return room.flag(IRoom::Flag::Bit4); });
        _filters.add_getter<bool>("Wind/Bit 5", [](auto&& room) { return room.flag(IRoom::Flag::Wind); });
        _filters.add_getter<bool>("Bit 6", [](auto&& room) { return room.flag(IRoom::Flag::Bit6); });
        _filters.add_getter<bool>("Quicksand/Block Lens Flare/Bit 7", [](auto&& room) { return room.flag(IRoom::Flag::Bit7); });
        _filters.add_getter<bool>("Caustics/Bit 8", [](auto&& room) { return room.flag(IRoom::Flag::Caustics); });
        _filters.add_getter<bool>("Reflectivity/Bit 9", [](auto&& room) { return room.flag(IRoom::Flag::WaterReflectivity); });
        _filters.add_getter<bool>("Snow (NGLE)/Bit 10", [](auto&& room) { return room.flag(IRoom::Flag::Bit10); });
        _filters.add_getter<bool>("D/Rain/Bit 11", [](auto&& room) { return room.flag(IRoom::Flag::Bit11); });
        _filters.add_getter<bool>("P/Cold/Bit 12", [](auto&& room) { return room.flag(IRoom::Flag::Bit12); });
        _filters.add_getter<bool>("Bit 13", [](auto&& room) { return room.flag(IRoom::Flag::Bit13); });
        _filters.add_getter<bool>("Bit 14", [](auto&& room) { return room.flag(IRoom::Flag::Bit14); });
        _filters.add_getter<bool>("Bit 15", [](auto&& room) { return room.flag(IRoom::Flag::Bit15); });
    }
}