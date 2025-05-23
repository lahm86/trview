#include "RoomsWindow.h"
#include <trview.app/Elements/IRoom.h>
#include <trview.app/Elements/ITrigger.h>
#include <trview.common/Strings.h>
#include "../trview_imgui.h"
#include "../Elements/Floordata.h"
#include "RowCounter.h"
#include "../Elements/ILevel.h"

namespace trview
{
    namespace
    {
        bool room_is_no_space(const IRoom& room)
        {
            for (const auto& sector : room.sectors())
            {
                if (is_no_space(sector->flags()))
                {
                    return true;
                }
            }
            return false;
        }

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
            add_flag("No Space", room_is_no_space(room));
        }

        float ambient_percentage(int16_t ambient)
        {
            return 1.0f - static_cast<float>(ambient) / static_cast<float>(0x1fff);
        }

        std::size_t item_count(std::vector<std::weak_ptr<IItem>>& items, const IRoom& room)
        {
            return std::ranges::count_if(items, [&room](const auto& item)
                {
                    if (auto i = item.lock())
                    {
                        return item_room(i) == room.number();
                    }
                    return false;
                });
        }
    }

    IRoomsWindow::~IRoomsWindow()
    {
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

        _token_store += _track.on_toggle<Type::Item>() += [&](bool value)
        {
            if (value && _global_selected_item.lock())
            {
                set_selected_item(_global_selected_item);
            }
        };
        _token_store += _track.on_toggle<Type::Trigger>() += [&](bool) { set_selected_trigger(_global_selected_trigger); };
        _token_store += _track.on_toggle<Type::Light>() += [&](bool) { set_selected_light(_global_selected_light); };
        _token_store += _track.on_toggle<Type::CameraSink>() += [&](bool) { set_selected_camera_sink(_global_selected_camera_sink); };
    }

    void RoomsWindow::set_current_room(const std::weak_ptr<IRoom>& room)
    {
        auto room_ptr = room.lock();
        auto current = _current_room.lock();
        if (room_ptr == current)
        {
            return;
        }

        set_selected_sector(nullptr);
        _current_room = room;
        _scroll_to_room = true;
        if (_sync_room && room_ptr)
        {
            select_room(_current_room);
            load_room_details(_current_room);
        }
    }

    void RoomsWindow::load_room_details(std::weak_ptr<IRoom> room)
    {
        _map_renderer->load(room.lock());
    }

    void RoomsWindow::set_items(const std::vector<std::weak_ptr<IItem>>& items)
    {
        _all_items = items;
        _global_selected_item.reset();
        _local_selected_item.reset();
        _force_sort = true;
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
        _current_room.reset();
        _triggers.clear();
        _lights.clear();
        _camera_sinks.clear();
        generate_filters();
        _force_sort = true;
        set_selected_sector(nullptr);
        calculate_column_widths();
    }

    void RoomsWindow::set_selected_item(const std::weak_ptr<IItem>& item)
    {
        _global_selected_item = item;
        if (_track.enabled<Type::Item>())
        {
            if (_selected_room.lock()!= _current_room.lock())
            {
                if (auto item_ptr = item.lock())
                {
                    select_room(item_ptr->room());
                    _scroll_to_room = true;
                    load_room_details(item_ptr->room());
                }
            }

            _local_selected_item = item;
            _scroll_to_item = true;
        }
    }

    void RoomsWindow::set_selected_trigger(const std::weak_ptr<ITrigger>& trigger)
    {
        _global_selected_trigger = trigger;
        if (_track.enabled<Type::Trigger>())
        {
            if (const auto trigger_ptr = trigger.lock())
            {
                if (_selected_room.lock() != _current_room.lock())
                {
                    select_room(trigger_ptr->room());
                    _scroll_to_room = true;
                    load_room_details(trigger_ptr->room());
                }

                _local_selected_trigger = trigger;
                _scroll_to_trigger = true;
            }
        }
    }

    void RoomsWindow::set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers)
    {
        _local_selected_trigger.reset();
        _triggers = triggers;
        _force_sort = true;
    }

    void RoomsWindow::set_sync_room(bool value)
    {
        if (_sync_room != value)
        {
            _sync_room = value;
            auto room = _current_room;
            if (_sync_room)
            {
                // Force room to be different to current room so that the
                // room details are loaded.
                _current_room.reset();
            }
            set_current_room(room);
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
            _force_sort = false;

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
        calculate_column_widths();
        if (ImGui::BeginChild(Names::rooms_panel.c_str(), ImVec2(0, 0), ImGuiChildFlags_AutoResizeX, ImGuiWindowFlags_NoScrollbar))
        {
            _auto_hider.check_focus();

            _filters.render();

            ImGui::SameLine();
            _track.render();

            ImGui::SameLine();
            bool sync_room = _sync_room;
            if (ImGui::Checkbox("Sync##syncroom", &sync_room))
            {
                set_sync_room(sync_room);
            }

            _auto_hider.render();

            auto filtered_rooms =
                _all_rooms |
                std::views::filter([&](auto&& room)
                    {
                        const auto room_ptr = room.lock();
                        return !(!room_ptr || !_filters.match(*room_ptr));
                    }) |
                std::views::transform([](auto&& room) { return room.lock(); }) |
                std::ranges::to<std::vector>();

            if (_auto_hider.apply(_all_rooms, filtered_rooms, _filters))
            {
                on_scene_changed();
            }

            RowCounter counter{ "room", _all_rooms.size() };
            if (ImGui::BeginTable(Names::rooms_list.c_str(), 5, ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY, ImVec2(0, -counter.height())))
            {
                imgui_header_row(
                    {
                        { "#", _column_sizer.size(0) },
                        { "Room", _column_sizer.size(1) },
                        { "Triggers", _column_sizer.size(2) },
                        { "Statics", _column_sizer.size(3) },
                        { .name = "Hide", .width = _column_sizer.size(4), .set_checked = [&](bool v)
                            {
                                std::ranges::for_each(filtered_rooms, [=](auto&& room) { room->set_visible(!v); });
                                on_scene_changed();
                            }, .checked = std::ranges::all_of(filtered_rooms, [](auto&& room) { return !room->visible(); })
                        }
                    });

                auto trigger_count = [&](const IRoom& room)
                {
                    return room.triggers().size();
                };

                auto static_mesh_count = [](const IRoom& room) { return room.static_meshes().size(); };

                imgui_sort_weak(_all_rooms,
                    {
                        [](auto&& l, auto&& r) { return l.number() < r.number(); },
                        [&](auto&& l, auto&& r) { return std::tuple(item_count(_all_items, l), l.number()) < std::tuple(item_count(_all_items, r), r.number()); },
                        [&](auto&& l, auto&& r) { return std::tuple(trigger_count(l), l.number()) < std::tuple(trigger_count(r), r.number()); },
                        [&](auto&& l, auto&& r) { return std::tuple(static_mesh_count(l), l.number()) < std::tuple(static_mesh_count(r), r.number()); },
                        [&](auto&& l, auto&& r) { return std::tuple(l.visible(), l.number()) < std::tuple(r.visible(), r.number()); }
                    }, _force_sort);

                const auto selected_room = _selected_room.lock();
                for (const auto& room : filtered_rooms)
                {
                    counter.count();
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    bool selected = room == selected_room;

                    ImGuiScroller scroller;
                    if (selected && _scroll_to_room)
                    {
                        scroller.scroll_to_item();
                        _scroll_to_room = false;
                    }

                    ImGui::SetNextItemAllowOverlap();
                    if (ImGui::Selectable(std::format("{0}##{0}", room->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                    {
                        scroller.fix_scroll();
                        select_room(room);
                        _map_renderer->load(room);
                        if (_sync_room)
                        {
                            on_room_selected(_selected_room);
                        }
                        _scroll_to_room = false;
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text(std::to_string(item_count(_all_items, *room)).c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text(std::to_string(trigger_count(*room)).c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text(std::to_string(static_mesh_count(*room)).c_str());
                    ImGui::TableNextColumn();
                    bool hidden = !room->visible();
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                    if (ImGui::Checkbox(std::format("##hide-{}", room->number()).c_str(), &hidden))
                    {
                        room->set_visible(!hidden);
                        on_scene_changed();
                    }
                    ImGui::PopStyleVar();
                }
                ImGui::EndTable();
                counter.render();
            }
        }
        ImGui::EndChild();
    }

    void RoomsWindow::render_room_details()
    {
        if (ImGui::BeginChild(Names::details_panel.c_str(), ImVec2(), true))
        {
            if (auto room = _selected_room.lock())
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
                    if (io.WantCaptureMouse && ImGui::IsWindowHovered(ImGuiHoveredFlags_None))
                    {
                        const auto mouse_pos = io.MousePos;
                        auto top_left = ImGui::GetCursorScreenPos();
                        auto adjusted = mouse_pos - top_left;
                        _map_renderer->set_cursor_position(Point(adjusted.x, adjusted.y));

                        auto sector = _map_renderer->sector_at_cursor();
                        on_sector_hover(sector);
                        if (sector)
                        {
                            if (io.MouseClicked[0])
                            {
                                if (_in_floordata_mode)
                                {
                                    set_selected_sector(sector);
                                }
                                else
                                {
                                    if (has_flag(sector->flags(), SectorFlag::Portal))
                                    {
                                        if (auto level = room->level().lock())
                                        {
                                            on_room_selected(level->room(sector->portal()));
                                        }
                                    }
                                    else if (sector->room_below() != 0xff)
                                    {
                                        if (auto level = room->level().lock())
                                        {
                                            on_room_selected(level->room(sector->room_below()));
                                        }
                                    }
                                    else
                                    {
                                        // Select triggers
                                        for (const auto& trigger : _triggers)
                                        {
                                            auto trigger_ptr = trigger.lock();
                                            if (trigger_ptr && trigger_ptr->sector_id() == sector->id())
                                            {
                                                on_trigger_selected(trigger);
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            else if (io.MouseClicked[1] && !_in_floordata_mode)
                            {
                                if (sector->room_above() != 0xff)
                                {
                                    if (auto level = room->level().lock())
                                    {
                                        on_room_selected(level->room(sector->room_above()));
                                    }
                                }
                            }
                        }
                    }
                    ImGui::Image(_map_texture.view().Get(), ImVec2(_map_texture.size().width, _map_texture.size().height));
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::ceil(remainder));
                    ImGui::Separator();
                }

                if (ImGui::BeginTabBar("TabBar"))
                {
                    if (ImGui::BeginTabItem(Names::properties_tab.c_str()))
                    {
                        render_properties_tab(room);
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Neighbours"))
                    {
                        render_neighbours_tab(room);
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Items", 0, _scroll_to_item ? ImGuiTabItemFlags_SetSelected : 0))
                    {
                        render_items_tab(room);
                        ImGui::EndTabItem();
                        _scroll_to_item = false;
                    }

                    if (ImGui::BeginTabItem("Triggers", 0, _scroll_to_trigger ? ImGuiTabItemFlags_SetSelected : 0))
                    {
                        render_triggers_tab();
                        ImGui::EndTabItem();
                        _scroll_to_trigger = false;
                    }

                    if (ImGui::BeginTabItem("Camera/Sink", 0, _scroll_to_camera_sink ? ImGuiTabItemFlags_SetSelected : 0))
                    {
                        render_camera_sink_tab();
                        ImGui::EndTabItem();
                        _scroll_to_camera_sink = false;
                    }

                    if (ImGui::BeginTabItem("Lights", 0, _scroll_to_light ? ImGuiTabItemFlags_SetSelected : 0))
                    {
                        render_lights_tab();
                        ImGui::EndTabItem();
                        _scroll_to_light = false;
                    }

                    if (ImGui::BeginTabItem("Floordata"))
                    {
                        _in_floordata_mode = true;
                        render_floordata_tab(room);
                        ImGui::EndTabItem();
                    }
                    else
                    {
                        _in_floordata_mode = false;
                    }

                    if (ImGui::BeginTabItem("Statics##Statics Tab"))
                    {
                        render_statics_tab();
                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
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
        _filters.add_getter<float>("X", [](auto&& room) { return static_cast<float>(room.info().x); });
        _filters.add_getter<float>("Y", [](auto&& room) { return static_cast<float>(room.info().yBottom); });
        _filters.add_getter<float>("Z", [](auto&& room) { return static_cast<float>(room.info().z); });
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
                for (const auto& trigger : room.triggers())
                {
                    if (const auto trigger_ptr = trigger.lock())
                    {
                        results.push_back(static_cast<float>(trigger_ptr->number()));
                    }
                }
                return results;
            });

        std::set<std::string> available_trigger_types;
        for (const auto& room : _all_rooms)
        {
            if (auto room_ptr = room.lock())
            {
                for (const auto& trigger : room_ptr->triggers())
                {
                    if (auto trigger_ptr = trigger.lock())
                    {
                        available_trigger_types.insert(to_string(trigger_ptr->type()));
                    }
                }
            }
        }
        _filters.add_multi_getter<std::string>("Trigger Type", { available_trigger_types.begin(), available_trigger_types.end() }, [&](auto&& room)
            {
                std::vector<std::string> results;
                for (const auto& trigger : room.triggers())
                {
                    if (const auto trigger_ptr = trigger.lock())
                    {
                        results.push_back(to_string(trigger_ptr->type()));
                    }
                }
                return results;
            });
        _filters.add_multi_getter<float>("Item Index", [&](auto&& room)
            {
                std::vector<float> results;
                for (const auto& item : _all_items)
                {
                    if (const auto item_ptr = item.lock())
                    {
                        if (item_room(item_ptr) == room.number())
                        {
                            results.push_back(static_cast<float>(item_ptr->number()));
                        }
                    }
                }
                return results;
            });

        std::set<std::string> available_item_types;
        for (const auto& item : _all_items)
        {
            if (auto item_ptr = item.lock())
            {
                available_item_types.insert(item_ptr->type());
            }
        }
        _filters.add_multi_getter<std::string>("Item Type", { available_item_types.begin(), available_item_types.end() }, [&](auto&& room)
            {
                std::vector<std::string> results;
                for (const auto& item : _all_items)
                {
                    if (auto item_ptr = item.lock())
                    {
                        if (item_room(item_ptr) == room.number())
                        {
                            results.push_back(item_ptr->type());
                        }
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
        _filters.add_getter<float>("Alternate", [](auto&& room) { return room.alternate_room(); }, [](auto&& room) { return room.alternate_mode() != IRoom::AlternateMode::None; });
        _filters.add_getter<float>("Alternate Group", [](auto&& room) { return room.alternate_group(); }, [](auto&& room) { return room.alternate_mode() != IRoom::AlternateMode::None; });
        _filters.add_getter<bool>("No Space", room_is_no_space);
        if (_level_version < trlevel::LevelVersion::Tomb4)
        {
            _filters.add_getter<float>("Ambient Intensity", [](auto&& room) { return room.ambient_intensity_1(); });
            _filters.add_getter<float>("Ambient Intensity %", [](auto&& room) { return ambient_percentage(room.ambient_intensity_1()) * 100.0f; });
            if (_level_version > trlevel::LevelVersion::Tomb1)
            {
                if (_level_version == trlevel::LevelVersion::Tomb2)
                {
                    _filters.add_getter<float>("Ambient Intensity 2", [](auto&& room) { return room.ambient_intensity_2(); });
                    _filters.add_getter<float>("Ambient Intensity 2 %", [](auto&& room) { return ambient_percentage(room.ambient_intensity_2()) * 100.0f; });
                }
                _filters.add_getter<float>("Light Mode", [](auto&& room) { return room.light_mode(); });
            }
        }
        else
        {
            _filters.add_getter<float>("Ambient R", [](auto&& room) { return room.ambient().r; });
            _filters.add_getter<float>("Ambient G", [](auto&& room) { return room.ambient().g; });
            _filters.add_getter<float>("Ambient B", [](auto&& room) { return room.ambient().b; });
        }

        const auto available_floordata_types = 
              std::views::iota(std::to_underlying(Floordata::Command::Function::None) + 1, std::to_underlying(Floordata::Command::Function::Count) + 0)
            | std::views::transform([](auto c) { return to_string(static_cast<Floordata::Command::Function>(c)); })
            | std::ranges::to<std::set>();

        _filters.add_multi_getter<std::string>("Floordata Type", { available_floordata_types.begin(), available_floordata_types.end() }, [&](auto&& room)
            {
                const auto& sectors = room.sectors();
                return sectors
                    | std::views::transform([&](auto&& s) { return parse_floordata(_floordata, s->floordata_index(), FloordataMeanings::None, _trng).commands; })
                    | std::views::join
                    | std::views::transform([](auto&& c) { return c.type; })
                    | std::ranges::to<std::unordered_set>()
                    | std::views::transform([](auto&& s) { return to_string(s); })
                    | std::ranges::to<std::vector>();
            });
    }

    void RoomsWindow::render_properties_tab(const std::shared_ptr<IRoom>& room)
    {
        const auto add_stat = [&]<typename T>(const std::string & name, const T && value, std::function<void()> click = {})
        {
            const auto string_value = get_string(value);
            ImGui::TableNextColumn();
            if (ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_SpanAllColumns))
            {
                if (click)
                {
                    click();
                }
                else
                {
                    _clipboard->write(to_utf16(string_value));
                    _tooltip_timer = 0.0f;
                }
            }
            ImGui::TableNextColumn();
            ImGui::Text(string_value.c_str());
        };

        if (ImGui::BeginTable(Names::properties.c_str(), 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Value");
            ImGui::TableNextRow();

            add_stat("X", room->info().x);
            add_stat("Y", room->info().yBottom);
            add_stat("Z", room->info().z);
            if (room->alternate_mode() != Room::AlternateMode::None)
            {
                add_stat("Alternate", room->alternate_room(), [this, room]() 
                    { 
                        if (auto level = room->level().lock())
                        {
                            on_room_selected(level->room(room->alternate_room()));
                        }
                    });
                if (room->alternate_group() != 0xff)
                {
                    add_stat("Alternate Group", room->alternate_group());
                }
            }

            auto format_colour = [](const Colour& colour)
            {
                return std::format("R: {}, G: {}, B: {}", static_cast<int>(colour.r * 255), static_cast<int>(colour.g * 255), static_cast<int>(colour.b * 255));
            };

            if (_level_version < trlevel::LevelVersion::Tomb4)
            {
                add_stat("Ambient Intensity", room->ambient_intensity_1());
                float ambient_intensity = ambient_percentage(room->ambient_intensity_1());
                ImGui::SameLine();
                ImGui::Text("%.2f%%", ambient_intensity * 100.0f);
                ImGui::SameLine();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
                ImGui::ColorButton("##ambientintensitybutton", ImVec4(ambient_intensity, ambient_intensity, ambient_intensity, 1.0f), 0, ImVec2(16, 16));
                if (_level_version > trlevel::LevelVersion::Tomb1)
                {
                    if (_level_version == trlevel::LevelVersion::Tomb2)
                    {
                        add_stat("Ambient Intensity 2", room->ambient_intensity_2());
                        float ambient_intensity2 = ambient_percentage(room->ambient_intensity_2());
                        ImGui::SameLine();
                        ImGui::Text("%.2f%%", ambient_intensity2 * 100.0f);
                        ImGui::SameLine();
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
                        ImGui::ColorButton("##ambientintensity2button", ImVec4(ambient_intensity2, ambient_intensity2, ambient_intensity2, 1.0f), 0, ImVec2(16, 16));
                    }
                    add_stat("Light Mode", std::format("{} ({})", room->light_mode(), light_mode_name(room->light_mode())));
                }
            }
            else
            {
                add_stat("Ambient", format_colour(room->ambient()));
                auto ambient = room->ambient();
                ImGui::SameLine();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 2.0f);
                ImGui::ColorButton("##ambientbutton", ImVec4(ambient.r, ambient.g, ambient.b, 1.0f), 0, ImVec2(16, 16));
            }
            add_room_flags(*_clipboard, _level_version, *room);
            ImGui::EndTable();
        }
    }

    void RoomsWindow::render_neighbours_tab(const std::shared_ptr<IRoom>& room)
    {
        if (ImGui::BeginTable("Neighbours", 1, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            for (auto& neighbour : room->neighbours())
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                bool selected = false;
                if (ImGui::Selectable(std::to_string(neighbour).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                {
                    auto level = room->level().lock();
                    if (level)
                    {
                        const auto neighbour_room = level->room(neighbour);
                        select_room(neighbour_room);
                        if (_sync_room)
                        {
                            on_room_selected(neighbour_room);
                        }
                    }
                }
            }

            ImGui::EndTable();
        }
    }

    void RoomsWindow::render_items_tab(const std::shared_ptr<IRoom>& room)
    {
        if (ImGui::BeginTable("Items", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            imgui_sort_weak(_all_items,
                {
                    [](auto&& l, auto&& r) { return l.number() < r.number(); },
                    [](auto&& l, auto&& r) { return std::tuple(l.type(), l.number()) < std::tuple(r.type(), r.number()); },
                }, _force_sort);

            const auto filtered = _all_items |
                std::views::transform([](auto&& i) { return i.lock(); }) |
                std::views::filter([this, room](auto&& i)
                    {
                        return i && i->room().lock() == room && (i->ng_plus().value_or(_ng_plus) == _ng_plus);
                    }) |
                std::ranges::to<std::vector>();

            for (const auto& item : filtered)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                auto selected_item = _local_selected_item.lock();
                bool selected = selected_item && selected_item == item;

                ImGuiScroller scroller;
                if (selected && _scroll_to_item)
                {
                    scroller.scroll_to_item();
                    _scroll_to_item = false;
                }

                if (ImGui::Selectable(std::to_string(item->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                {
                    scroller.fix_scroll();
                    _local_selected_item = item;
                    on_item_selected(item);
                    _scroll_to_item = false;
                }

                ImGui::TableNextColumn();
                ImGui::Text(item->type().c_str());
            }

            ImGui::EndTable();
        }
    }

    void RoomsWindow::render_triggers_tab()
    {
        if (ImGui::BeginTable("Triggers", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            imgui_sort_weak(_triggers,
                {
                    [](auto&& l, auto&& r) { return l.number() < r.number(); },
                    [&](auto&& l, auto&& r) { return std::tuple(to_string(l.type()), l.number()) < std::tuple(to_string(r.type()), r.number()); }
                }, _force_sort);

            for (const auto& trigger : _triggers)
            {
                if (const auto trigger_ptr = trigger.lock())
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

                    if (ImGui::Selectable(std::format("{0}##{0}", trigger_ptr->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                    {
                        scroller.fix_scroll();
                        _local_selected_trigger = trigger_ptr;
                        on_trigger_selected(trigger);
                        _scroll_to_trigger = false;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text(to_string(trigger_ptr->type()).c_str());
                }
            }

            ImGui::EndTable();
        }
    }

    void RoomsWindow::render_floordata_tab(const std::shared_ptr<IRoom>&)
    {
        ImGui::Checkbox(Names::simple_mode.c_str(), &_simple_mode);

        auto selected_sector = _selected_sector.lock();
        if (!selected_sector)
        {
            ImGui::Text("Select a sector to view floordata");
        }
        else if (ImGui::BeginTable("##floordata", _simple_mode ? 2 : 3, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Value");
            if (!_simple_mode)
            {
                ImGui::TableSetupColumn("Meaning");
            }
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            if (selected_sector)
            {
                const Floordata floordata = parse_floordata(_floordata, selected_sector->floordata_index(), FloordataMeanings::Generate, _all_items, _trng);

                uint32_t index = selected_sector->floordata_index();
                for (const auto& command : floordata.commands)
                {
                    for (std::size_t i = 0; i < command.data.size(); ++i, ++index)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        bool selected = _selected_floordata == index;
                        if (ImGui::Selectable(std::format("{}", index).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns))
                        {
                            _selected_floordata = index;
                        }
                        if (ImGui::BeginPopupContextItem())
                        {
                            if (ImGui::MenuItem("Copy"))
                            {
                                _clipboard->write(to_utf16(std::format("{} {:04X} {}", index, command.data[i],
                                    _simple_mode ? "" : command.meanings[i])));
                            }
                            if (ImGui::MenuItem("Copy All"))
                            {
                                std::string data;
                                for (uint32_t d = 0; d < command.data.size(); ++d)
                                {
                                    data += std::format("{} {:04X} {}\n", index, command.data[d],
                                        _simple_mode ? "" : command.meanings[d]);
                                }
                                _clipboard->write(to_utf16(data));
                            }
                            ImGui::EndPopup();
                        }
                        ImGui::TableNextColumn();
                        ImGui::Text("%04X", command.data[i]);
                        if (!_simple_mode)
                        {
                            ImGui::TableNextColumn();
                            ImGui::Text(command.meanings[i].c_str());
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
    }

    void RoomsWindow::set_floordata(const std::vector<uint16_t>& data)
    {
        _floordata = data;
    }

    void RoomsWindow::set_selected_sector(const std::shared_ptr<ISector>& sector)
    {
        _selected_sector = sector;
        _map_renderer->set_selection(sector);
    }

    void RoomsWindow::set_selected_light(const std::weak_ptr<ILight>& light)
    {
        _global_selected_light = light;
        if (_track.enabled<Type::Light>())
        {
            if (const auto light_ptr = light.lock())
            {
                if (_selected_room.lock() != _current_room.lock())
                {
                    select_room(light_ptr->room());
                    _scroll_to_room = true;
                    load_room_details(light_ptr->room());
                }

                _local_selected_light = light;
                _scroll_to_light = true;
            }
        }
    }
    
    void RoomsWindow::set_selected_camera_sink(const std::weak_ptr<ICameraSink>& camera_sink)
    {
        _global_selected_camera_sink = camera_sink;
        if (_track.enabled<Type::CameraSink>())
        {
            if (const auto camera_sink_ptr = camera_sink.lock())
            {
                if (_selected_room.lock() != _current_room.lock())
                {
                    const auto actual = actual_room(*camera_sink_ptr);
                    select_room(actual);
                    _scroll_to_room = true;
                    load_room_details(actual);
                }

                _local_selected_camera_sink = camera_sink;
                _scroll_to_camera_sink = true;
            }
        }
    }

    void RoomsWindow::render_camera_sink_tab()
    {
        if (ImGui::BeginTable("Camera/Sinks", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            imgui_sort_weak(_camera_sinks,
                {
                    [](auto&& l, auto&& r) { return l.number() < r.number(); },
                    [&](auto&& l, auto&& r) { return std::tuple(to_string(l.type()), l.number()) < std::tuple(to_string(r.type()), r.number()); }
                }, _force_sort);

            for (const auto& camera_sink : _camera_sinks)
            {
                if (const auto camera_sink_ptr = camera_sink.lock())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    const auto local_selection = _local_selected_camera_sink.lock();
                    bool selected = local_selection && local_selection->number() == camera_sink_ptr->number();

                    ImGuiScroller scroller;
                    if (selected && _scroll_to_camera_sink)
                    {
                        scroller.scroll_to_item();
                        _scroll_to_camera_sink = false;
                    }

                    if (ImGui::Selectable(std::format("{0}##{0}", camera_sink_ptr->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                    {
                        scroller.fix_scroll();
                        _local_selected_camera_sink = camera_sink_ptr;
                        on_camera_sink_selected(camera_sink);
                        _scroll_to_camera_sink = false;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text(to_string(camera_sink_ptr->type()).c_str());
                }
            }

            ImGui::EndTable();
        }
    }
    
    void RoomsWindow::render_lights_tab()
    {
        if (ImGui::BeginTable("Lights", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            imgui_sort_weak(_lights,
                {
                    [](auto&& l, auto&& r) { return l.number() < r.number(); },
                    [&](auto&& l, auto&& r) { return std::tuple(to_string(l.type()), l.number()) < std::tuple(to_string(r.type()), r.number()); }
                }, _force_sort);

            for (const auto& light : _lights)
            {
                if (auto light_ptr = light.lock())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    const auto local_selection = _local_selected_light.lock();
                    bool selected = local_selection && local_selection->number() == light_ptr->number();

                    ImGuiScroller scroller;
                    if (selected && _scroll_to_light)
                    {
                        scroller.scroll_to_item();
                        _scroll_to_light = false;
                    }

                    if (ImGui::Selectable(std::format("{0}##{0}", light_ptr->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                    {
                        scroller.fix_scroll();
                        _local_selected_light = light_ptr;
                        on_light_selected(light);
                        _scroll_to_light = false;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text(to_string(light_ptr->type()).c_str());
                }
            }

            ImGui::EndTable();
        }
    }

    void RoomsWindow::clear_selected_light()
    {
        _local_selected_light.reset();
        _global_selected_light.reset();
    }

    void RoomsWindow::clear_selected_camera_sink()
    {
        _local_selected_camera_sink.reset();
        _global_selected_camera_sink.reset();
    }

    void RoomsWindow::set_camera_sinks(const std::vector<std::weak_ptr<ICameraSink>>& camera_sinks)
    {
        _local_selected_camera_sink.reset();
        _camera_sinks = camera_sinks;
        _force_sort = true;
    }

    void RoomsWindow::set_lights(const std::vector<std::weak_ptr<ILight>>& lights)
    {
        _local_selected_light.reset();
        _lights = lights;
        _force_sort = true;
    }

    void RoomsWindow::select_room(std::weak_ptr<IRoom> room)
    {
        _selected_room = room;
        if (auto room_ptr = room.lock())
        {
            set_triggers(room_ptr->triggers());
            set_lights(room_ptr->lights());
            set_camera_sinks(room_ptr->camera_sinks());
            set_static_meshes(room_ptr->static_meshes());
            return;
        }

        _lights.clear();
        _camera_sinks.clear();
        _triggers.clear();
        _static_meshes.clear();
    }

    void RoomsWindow::render_statics_tab()
    {
        if (ImGui::BeginTable("Statics", 2, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("ID");
            ImGui::TableSetupScrollFreeze(1, 1);
            ImGui::TableHeadersRow();

            for (const auto& static_mesh : _static_meshes)
            {
                if (auto static_mesh_ptr = static_mesh.lock())
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();

                    const auto local_selection = _local_selected_static_mesh.lock();
                    bool selected = local_selection && local_selection == static_mesh_ptr;

                    ImGuiScroller scroller;
                    if (selected && _scroll_to_static_mesh)
                    {
                        scroller.scroll_to_item();
                        _scroll_to_static_mesh = false;
                    }

                    if (ImGui::Selectable(std::format("{0}##{0}", static_mesh_ptr->number()).c_str(), &selected, ImGuiSelectableFlags_SpanAllColumns | static_cast<int>(ImGuiSelectableFlags_SelectOnNav)))
                    {
                        scroller.fix_scroll();
                        _local_selected_static_mesh = static_mesh_ptr;
                        on_static_mesh_selected(static_mesh);
                        _scroll_to_static_mesh = false;
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text(std::format("{}", static_mesh_ptr->id()).c_str());
                }
            }

            ImGui::EndTable();
        }
    }

    void RoomsWindow::set_static_meshes(const std::vector<std::weak_ptr<IStaticMesh>>& static_meshes)
    {
        _local_selected_static_mesh.reset();
        _static_meshes = static_meshes;
        _force_sort = true;
    }

    void RoomsWindow::calculate_column_widths()
    {
        _column_sizer.reset();
        _column_sizer.measure("#__", 0);
        _column_sizer.measure("Items__", 1);
        _column_sizer.measure("Triggers__", 2);
        _column_sizer.measure("Statics__", 3);
        _column_sizer.measure("Hide______", 4);

        for (const auto& room : _all_rooms)
        {
            if (auto room_ptr = room.lock())
            {
                _column_sizer.measure(std::format("{0}", room_ptr->number()), 0);
                _column_sizer.measure(std::to_string(item_count(_all_items, *room_ptr)), 1);
                _column_sizer.measure(std::to_string(room_ptr->triggers().size()), 2);
                _column_sizer.measure(std::to_string(room_ptr->static_meshes().size()), 3);
            }
        }
    }

    void RoomsWindow::set_ng_plus(bool value)
    {
        _ng_plus = value;
    }

    void RoomsWindow::set_trng(bool value)
    {
        _trng = value;
    }

    std::string RoomsWindow::name() const
    {
        return _id;
    }

    void RoomsWindow::set_filters(std::vector<Filters<IRoom>::Filter> filters)
    {
        _filters.set_filters(filters);
    }
}