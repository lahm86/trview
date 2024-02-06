#include "Viewer.h"

#include <trlevel/ILevel.h>
#include <trview.graphics/RenderTargetStore.h>
#include <trview.graphics/ISprite.h>
#include <trview.graphics/ViewportStore.h>
#include <trview.common/Strings.h>

using namespace DirectX::SimpleMath;

namespace trview
{
    namespace
    {
        const float _CAMERA_MOVEMENT_SPEED_MULTIPLIER = 23.0f;
    }

    IViewer::~IViewer()
    {
    }

    Viewer::Viewer(const Window& window, const std::shared_ptr<graphics::IDevice>& device, std::unique_ptr<IViewerUI> ui, std::unique_ptr<IPicking> picking,
        std::unique_ptr<input::IMouse> mouse, const std::shared_ptr<IShortcuts>& shortcuts, const std::shared_ptr<IRoute> route, const graphics::ISprite::Source& sprite_source,
        std::unique_ptr<ICompass> compass, std::unique_ptr<IMeasure> measure, const graphics::IRenderTarget::SizeSource& render_target_source, const graphics::IDeviceWindow::Source& device_window_source,
        std::unique_ptr<ISectorHighlight> sector_highlight, const std::shared_ptr<IClipboard>& clipboard)
        : MessageHandler(window), _shortcuts(shortcuts), _camera(window.size()), _free_camera(window.size()), _timer(default_time_source()), _keyboard(window),
        _mouse(std::move(mouse)), _window_resizer(window), _alternate_group_toggler(window),
        _menu_detector(window), _device(device), _route(route), _ui(std::move(ui)), _picking(std::move(picking)), _compass(std::move(compass)), _measure(std::move(measure)),
        _render_target_source(render_target_source), _sector_highlight(std::move(sector_highlight)), _clipboard(clipboard)
    {
        apply_camera_settings();

        _scene_target = _render_target_source(static_cast<uint32_t>(window.size().width), static_cast<uint32_t>(window.size().height), graphics::IRenderTarget::DepthStencilMode::Enabled);
        _scene_sprite = sprite_source(window.size());
        _token_store += _free_camera.on_view_changed += [&]() { _scene_changed = true; };
        _token_store += _camera.on_view_changed += [&]() { _scene_changed = true; };

        _main_window = device_window_source(window);

        _token_store += _window_resizer.on_resize += [=]()
        {
            _main_window->resize();
            resize_elements();
        };

        _token_store += _alternate_group_toggler.on_alternate_group += [&](uint32_t group)
        {
            if (!_ui->is_input_active())
            {
                set_alternate_group(group, !alternate_group(group));
            }
        };

        initialise_input();


        std::unordered_map<std::string, std::function<void(bool)>> toggles;
        toggles[Options::highlight] = [this](bool) { toggle_highlight(); };
        toggles[Options::geometry] = [this](bool value) { set_show_geometry(value); };
        toggles[Options::water] = [this](bool value) { set_show_water(value); };
        toggles[Options::wireframe] = [this](bool value) { set_show_wireframe(value); };
        toggles[Options::triggers] = [this](bool value) { set_show_triggers(value); };
        toggles[Options::show_bounding_boxes] = [this](bool value) { set_show_bounding_boxes(value); };
        toggles[Options::flip] = [this](bool value) { set_alternate_mode(value); };
        toggles[Options::depth_enabled] = [this](bool value) { if (auto level = _level.lock()) { level->set_highlight_mode(ILevel::RoomHighlightMode::Neighbours, value); } };
        toggles[Options::lights] = [this](bool value) { set_show_lights(value); };
        toggles[Options::items] = [this](bool value) { set_show_items(value); };
        toggles[Options::rooms] = [this](bool value) { set_show_rooms(value); };
        toggles[Options::camera_sinks] = [this](bool value) { set_show_camera_sinks(value); };
        toggles[Options::lighting] = [this](bool value) { set_show_lighting(value); };
        toggles[Options::notes] = [](bool) {};

        const auto persist_toggle_value = [&](const std::string& name, bool value)
        {
            if (equals_any(name, Options::flip, Options::highlight, Options::depth_enabled))
            {
                return;
            }
            _settings.toggles[name] = value;
            on_settings(_settings);
        };

        std::unordered_map<std::string, std::function<void(int32_t)>> scalars;
        scalars[Options::depth] = [this](int32_t value) { if (auto level = _level.lock()) { level->set_neighbour_depth(value); } };

        _token_store += _ui->on_select_item += [&](const auto& item) { on_item_selected(item); };
        _token_store += _ui->on_select_room += [&](const auto& room)
            {
                if (auto r = room.lock())
                {
                    on_room_selected(r->number());
                }
            };
        _token_store += _ui->on_toggle_changed += [this, toggles, persist_toggle_value](const std::string& name, bool value)
        {
            auto toggle = toggles.find(name);
            if (toggle == toggles.end())
            {
                return;
            }
            toggle->second(value);
            persist_toggle_value(name, value);
        };
        _token_store += _ui->on_alternate_group += [&](uint32_t group, bool value) { set_alternate_group(group, value); };
        _token_store += _ui->on_scalar_changed += [this, scalars](const std::string& name, int32_t value)
        {
            auto scalar = scalars.find(name);
            if (scalar == scalars.end())
            {
                return;
            }
            scalar->second(value);
        };
        _token_store += _ui->on_camera_reset += [&]() { _camera.reset(); };
        _token_store += _ui->on_camera_mode += [&](CameraMode mode) { set_camera_mode(mode); };
        _token_store += _ui->on_camera_projection_mode += [&](ProjectionMode mode) { set_camera_projection_mode(mode); };
        _token_store += _ui->on_sector_hover += [&](const std::shared_ptr<ISector>& sector) { set_sector_highlight(sector); };
        _token_store += _ui->on_add_waypoint += [&]()
        {
            auto type = _context_pick.type == PickResult::Type::Entity ? IWaypoint::Type::Entity : _context_pick.type == PickResult::Type::Trigger ? IWaypoint::Type::Trigger : IWaypoint::Type::Position;
            if (_context_pick.triangle.normal.y != 0)
            {
                _context_pick.triangle.normal.x = 0;
                _context_pick.triangle.normal.z = 0;
                _context_pick.triangle.normal.Normalize();
            }

            if (auto level = _level.lock())
            {
                if (_context_pick.type == PickResult::Type::Entity)
                {
                    if (const auto item = level->item(_context_pick.index).lock())
                    {
                        _context_pick.position = item->position();
                    }
                }
                else if (_context_pick.type == PickResult::Type::Trigger)
                {
                    if (const auto trigger = level->trigger(_context_pick.index).lock())
                    {
                        _context_pick.position = trigger->position();
                    }
                }
            }
            on_waypoint_added(_context_pick.position, _context_pick.triangle.normal, room_from_pick(_context_pick), type, _context_pick.index);
        };
        _token_store += _ui->on_add_mid_waypoint += [&]()
        {
            auto type = _context_pick.type == PickResult::Type::Entity ? IWaypoint::Type::Entity : _context_pick.type == PickResult::Type::Trigger ? IWaypoint::Type::Trigger : IWaypoint::Type::Position;

            if (_context_pick.type == PickResult::Type::Room)
            {
                _context_pick.position = _context_pick.centroid;
            }
            else if (_context_pick.type == PickResult::Type::Entity)
            {
                if (const auto level = _level.lock())
                {
                    if (const auto item = level->item(_context_pick.index).lock())
                    {
                        _context_pick.position = item->position();
                    }
                }
            }
            else if (_context_pick.type == PickResult::Type::Trigger)
            {
                if (const auto level = _level.lock())
                {
                    if (const auto trigger = level->trigger(_context_pick.index).lock())
                    {
                        _context_pick.position = trigger->position();
                    }
                }
            }

            // Filter out non-wall normals - ceiling and floor normals should be vertical.
            if (_context_pick.triangle.normal.y != 0)
            {
                _context_pick.triangle.normal.x = 0;
                _context_pick.triangle.normal.z = 0;
                _context_pick.triangle.normal.Normalize();
            }

            on_waypoint_added(_context_pick.position, _context_pick.triangle.normal, room_from_pick(_context_pick), type, _context_pick.index);
        };
        _token_store += _ui->on_remove_waypoint += [&]() { on_waypoint_removed(_context_pick.index); };
        _token_store += _ui->on_hide += [&]()
        {
            if (auto level = _level.lock())
            {
                if (_context_pick.type == PickResult::Type::Entity)
                {
                    on_item_visibility(level->item(_context_pick.index), false);
                }
                else if (_context_pick.type == PickResult::Type::Trigger)
                {
                    on_trigger_visibility(level->trigger(_context_pick.index), false);
                }
                else if (_context_pick.type == PickResult::Type::Light)
                {
                    on_light_visibility(level->light(_context_pick.index), false);
                }
                else if (_context_pick.type == PickResult::Type::Room)
                {
                    on_room_visibility(level->room(_context_pick.index), false);
                }
                else if (_context_pick.type == PickResult::Type::CameraSink)
                {
                    on_camera_sink_visibility(level->camera_sink(_context_pick.index), false);
                }
            }
        };
        _token_store += _ui->on_orbit += [&]()
        {
            bool was_alternate_select = _was_alternate_select;
            on_room_selected(room_from_pick(_context_pick));
            if (!was_alternate_select)
            {
                set_camera_mode(CameraMode::Orbit);
            }

            _target = _context_pick.position;

            auto stored_pick = _context_pick;
            stored_pick.override_centre = true;
            stored_pick.type = PickResult::Type::Room;
            add_recent_orbit(stored_pick);
        };
        _ui->on_settings += on_settings;
        _token_store += _ui->on_tool_selected += [&](auto tool) { _active_tool = tool; _measure->reset(); };
        _token_store += _ui->on_camera_position += [&](const auto& position)
        {
            if (_camera_mode == CameraMode::Orbit)
            {
                set_camera_mode(CameraMode::Free);
            }
            _free_camera.set_position(position);
        };
        _token_store += _ui->on_camera_rotation += [&](auto yaw, auto pitch)
        {
            current_camera().set_rotation_yaw(yaw);
            current_camera().set_rotation_pitch(pitch);
        };
        _token_store += _ui->on_copy += [&](auto type)
        {
            switch (type)
            {
                case IContextMenu::CopyType::Position:
                {
                    const auto pos = _context_pick.position * trlevel::Scale;
                    _clipboard->write(to_utf16(std::format("{:.0f}, {:.0f}, {:.0f}", pos.x, pos.y, pos.z)));
                    break;
                }
                case IContextMenu::CopyType::Number:
                {
                    _clipboard->write(std::to_wstring(_context_pick.index));
                    break;
                }
            }
        };
        _ui->on_select_trigger += on_trigger_selected;

        _ui->set_settings(_settings);
        _ui->set_camera_mode(CameraMode::Orbit);

        _token_store += _measure->on_visible += [&](bool show) { _ui->set_show_measure(show); };
        _token_store += _measure->on_position += [&](auto pos) { _ui->set_measure_position(pos); };
        _token_store += _measure->on_distance += [&](float distance) { _ui->set_measure_distance(distance); };

        _token_store += _picking->pick_sources += [&](PickInfo, PickResult& result) 
        {
            result.stop = !should_pick();
        };
        _token_store += _picking->pick_sources += [&](PickInfo info, PickResult& result)
        {
            if (result.stop || _active_tool != Tool::None)
            {
                _compass_axis.reset();
                return;
            }

            Compass::Axis axis;
            if (_compass->pick(info.screen_position, info.screen_size, axis))
            {
                result.hit = true;
                result.stop = true;
                result.position = info.position + info.direction;
                result.type = PickResult::Type::Compass;
                result.index = static_cast<uint32_t>(axis);
                result.distance = 1.0f;
                result.text = axis_name(axis);
                _compass_axis = axis;
            }
            else
            {
                _compass_axis.reset();
            }
        };
        _token_store += _picking->pick_sources += [&](PickInfo info, PickResult& result)
        {
            const auto level = _level.lock();
            if (result.stop || !level)
            {
                return;
            }
            result = nearest_result(result, level->pick(current_camera(), info.position, info.direction));
        };
        _token_store += _picking->pick_sources += [&](PickInfo info, PickResult& result)
        {
            if (result.stop)
            {
                return;
            }
            result = nearest_result(result, _route->pick(info.position, info.direction));
        };
        _token_store += _picking->pick_sources += [&](PickInfo, PickResult& result)
        {
            if (_active_tool == Tool::Measure && result.hit && !result.stop)
            {
                _measure->set(result.position);
                _scene_changed = true;
                if (_measure->measuring())
                {
                    result.text = _measure->distance();
                }
                else
                {
                    result.text = "|....|";
                }
            }
        };

        _token_store += _picking->on_pick += [&](PickInfo, PickResult result)
        {
            _current_pick = result;

            const auto level = _level.lock();
            if (level && _route)
            {
                result.text = generate_pick_message(result, *level, *_route);
            }
            _ui->set_pick(result);

            if (result.stop)
            {
                return;
            }

            // Highlight sectors in the minimap.
            if (level)
            {
                std::optional<RoomInfo> info;
                if (result.hit)
                {
                    if (_current_pick.type == PickResult::Type::Room &&
                        _current_pick.index == level->selected_room())
                    {
                        const auto room = level->room(_current_pick.index).lock();
                        if (room)
                        {
                            info = room->info();
                        }
                    }
                    else if (_current_pick.type == PickResult::Type::Trigger)
                    {
                        const auto trigger = level->trigger(_current_pick.index).lock();
                        if (trigger && trigger_room(trigger) == level->selected_room())
                        {
                            if (const auto room = trigger->room().lock())
                            {
                                info = room->info();
                            }
                        }
                    }
                }

                if (!info.has_value())
                {
                    _ui->clear_minimap_highlight();
                    return;
                }

                auto x = _current_pick.position.x - (info.value().x / trlevel::Scale_X);
                auto z = _current_pick.position.z - (info.value().z / trlevel::Scale_Z);
                _ui->set_minimap_highlight(static_cast<uint16_t>(x), static_cast<uint16_t>(z));
            }
        };

        _token_store += _menu_detector.on_menu_toggled += [&](bool)
        {
            _timer.reset();
            _camera_input.reset();
        };

        _ui->set_route(_route);
    }

    void Viewer::initialise_input()
    {
        _token_store += _keyboard.on_key_up += [&](auto key, bool, bool) 
        {
            if (!_ui->is_input_active())
            {
                _camera_input.key_up(key);
            }
        };

        auto add_shortcut = [&](bool control, uint16_t key, std::function<void ()> fn)
        {
            _token_store += _shortcuts->add_shortcut(control, key) += [&, fn]()
            {
                if (!_ui->is_input_active()) { fn(); }
            };
        };

        add_shortcut(false, 'P', [&]() { toggle_alternate_mode(); });
        add_shortcut(false, 'M', [&]()
        {
            _active_tool = Tool::Measure;
            _measure->reset();
            _scene_changed = true;
        });
        add_shortcut(false, 'I', [&]() { toggle_show_items(); });
        add_shortcut(false, 'T', [&]() { toggle_show_triggers(); });
        add_shortcut(false, 'G', [&]() { toggle_show_geometry(); });
        add_shortcut(false, VK_OEM_MINUS, [&]() { select_previous_orbit(); });
        add_shortcut(false, VK_OEM_PLUS, [&]() { select_next_orbit(); });
        add_shortcut(false, VK_F1, [&]() { _ui->toggle_settings_visibility(); });
        add_shortcut(false, 'H', [&]() { toggle_highlight(); });
        add_shortcut(false, VK_INSERT, [&]()
        {
            // Reset the camera to defaults.
            _camera.set_rotation_yaw(0.f);
            _camera.set_rotation_pitch(-0.78539f);
            _camera.set_zoom(8.f);
        });
        add_shortcut(false, 'L', [&]() { toggle_show_lights(); });
        add_shortcut(true, 'H', [&]() { toggle_show_lighting(); });
        add_shortcut(false, 'K', [&]() { toggle_show_camera_sinks(); });

        _token_store += _keyboard.on_key_down += [&](uint16_t key, bool control, bool)
        {
            if (!_ui->is_input_active() && !_ui->show_context_menu())
            {
                _camera_input.key_down(key, control);
            }
            else if (_ui->show_context_menu() && key == VK_ESCAPE)
            {
                _ui->set_show_context_menu(false);
            }
        };

        setup_camera_input();

        using namespace input;

        _token_store += _mouse->mouse_click += [&](IMouse::Button button)
        {
            if (button == IMouse::Button::Left)
            {
                auto io = ImGui::GetIO();
                if (!(_ui->is_cursor_over() || io.WantCaptureMouse))
                {
                    if (_ui->show_context_menu())
                    {
                        _ui->set_show_context_menu(false);
                        return;
                    }

                    _ui->set_show_context_menu(false);

                    if (_compass_axis.has_value())
                    {
                        align_camera_to_axis(current_camera(), _compass_axis.value());
                        _compass_axis.reset();
                    }
                    else if (_current_pick.hit)
                    {
                        if (_active_tool == Tool::Measure)
                        {
                            if (_measure->add(_current_pick.position))
                            {
                                _active_tool = Tool::None;
                            }
                        }
                        else
                        {
                            select_pick(_current_pick);
                            
                            if (_settings.auto_orbit)
                            {
                                set_camera_mode(CameraMode::Orbit);
                                auto stored_pick = _current_pick;
                                stored_pick.position = _target;
                                add_recent_orbit(stored_pick);
                            }
                        }
                    }
                }
                else if (std::shared_ptr<ISector> sector = _ui->current_minimap_sector())
                {
                    const auto level = _level.lock();
                    if (level && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
                    {
                        uint32_t room = room_number(sector->room());
                        // Select the trigger (if it is a trigger).
                        const auto triggers = level->triggers();
                        auto trigger = std::find_if(triggers.begin(), triggers.end(),
                            [&](auto t)
                            {
                                const auto t_ptr = t.lock();
                                return trigger_room(t_ptr) == room && t_ptr->sector_id() == sector->id();
                            });

                        if (trigger == triggers.end() || (GetAsyncKeyState(VK_CONTROL) & 0x8000))
                        {
                            if (has_flag(sector->flags(), SectorFlag::Portal))
                            {
                                on_room_selected(sector->portal());
                            }
                            else if (!_settings.invert_map_controls && has_flag(sector->flags(), SectorFlag::RoomBelow))
                            {
                                on_room_selected(sector->room_below());
                            }
                            else if (_settings.invert_map_controls && has_flag(sector->flags(), SectorFlag::RoomAbove))
                            {
                                on_room_selected(sector->room_above());
                            }
                        }
                        else
                        {
                            on_trigger_selected(*trigger);
                        }
                    }
                }
            }
            else if (button == IMouse::Button::Right)
            {
                _ui->set_show_context_menu(false);

                if (!ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
                {
                    if (auto sector = _ui->current_minimap_sector())
                    {
                        if (!_settings.invert_map_controls && has_flag(sector->flags(), SectorFlag::RoomAbove))
                        {
                            on_room_selected(sector->room_above());
                        }
                        else if (_settings.invert_map_controls && has_flag(sector->flags(), SectorFlag::RoomBelow))
                        {
                            on_room_selected(sector->room_below());
                        }
                    }
                }
            }
        };

        _token_store += _mouse->mouse_click += [&](auto button)
        {
            if (button == input::IMouse::Button::Right && _current_pick.hit && _current_pick.type != PickResult::Type::Compass && current_camera().idle_rotation())
            {
                _context_pick = _current_pick;
                _ui->set_show_context_menu(true);
                _camera_input.reset(true);
                _ui->set_remove_waypoint_enabled(_current_pick.type == PickResult::Type::Waypoint);
                _ui->set_hide_enabled(equals_any(_current_pick.type, PickResult::Type::Entity, PickResult::Type::Trigger, PickResult::Type::Light, PickResult::Type::Room, PickResult::Type::CameraSink));
                _ui->set_mid_waypoint_enabled(_current_pick.type == PickResult::Type::Room && _current_pick.triangle.normal.y < 0);

                const auto level = _level.lock();
                if (_current_pick.type == PickResult::Type::Entity && level)
                {
                    const auto item = level->item(_current_pick.index).lock();
                    _ui->set_triggered_by(item ? item->triggers() : std::vector<std::weak_ptr<ITrigger>>{});
                }
                else if (_current_pick.type == PickResult::Type::CameraSink && level)
                {
                    const auto camera_sink = level->camera_sink(_current_pick.index).lock();
                    _ui->set_triggered_by(camera_sink ? camera_sink->triggers() : std::vector<std::weak_ptr<ITrigger>>{});
                }
                else 
                {
                    _ui->set_triggered_by({});
                }
            }
        };
    }

    void Viewer::update_camera()
    {
        if (_camera_mode == CameraMode::Free || _camera_mode == CameraMode::Axis)
        {
            const float Speed = std::max(0.01f, _settings.camera_movement_speed) * _CAMERA_MOVEMENT_SPEED_MULTIPLIER;
            _free_camera.move(_camera_input.movement() * Speed, _timer.elapsed());

            if (auto level = _level.lock())
            {
                level->on_camera_moved();
            }
        }

        current_camera().update(_timer.elapsed());
    }

    void Viewer::open(const std::weak_ptr<ILevel>& level, ILevel::OpenMode open_mode)
    {
        auto old_level = _level.lock();
        auto new_level = level.lock();
        _level = level;

        _level_token_store.clear();
        if (old_level)
        {
            old_level->on_room_selected -= on_room_selected;
            old_level->on_item_selected -= on_item_selected;
            old_level->on_trigger_selected -= on_trigger_selected;
        }

        _level_token_store += new_level->on_alternate_mode_selected += [&](bool enabled) { set_alternate_mode(enabled); };
        _level_token_store += new_level->on_alternate_group_selected += [&](uint16_t group, bool enabled) { set_alternate_group(group, enabled); };
        _level_token_store += new_level->on_level_changed += [&]() { _scene_changed = true; };
        new_level->on_room_selected += on_room_selected;
        new_level->on_item_selected += on_item_selected;
        new_level->on_trigger_selected += on_trigger_selected;

        new_level->set_show_triggers(_ui->toggle(Options::triggers));
        new_level->set_show_geometry(_ui->toggle(Options::geometry));
        new_level->set_show_water(_ui->toggle(Options::water));
        new_level->set_show_wireframe(_ui->toggle(Options::wireframe));
        new_level->set_show_bounding_boxes(_ui->toggle(Options::show_bounding_boxes));
        new_level->set_show_lights(_ui->toggle(Options::lights));
        new_level->set_show_items(_ui->toggle(Options::items));
        new_level->set_show_rooms(_ui->toggle(Options::rooms));
        new_level->set_show_camera_sinks(_ui->toggle(Options::camera_sinks));
        new_level->set_show_lighting(_ui->toggle(Options::lighting));

        // Set up the views.
        auto rooms = new_level->rooms();

        if (open_mode == ILevel::OpenMode::Full || !old_level)
        {
            _camera.reset();
            _ui->set_toggle(Options::highlight, false);
            _ui->set_toggle(Options::flip, false);
            _ui->set_toggle(Options::depth_enabled, false);
            _ui->set_scalar(Options::depth, 1);
            _measure->reset();
            _recent_orbits.clear();
            _recent_orbit_index = 0u;

            std::weak_ptr<IItem> lara;
            if (_settings.go_to_lara && find_last_item_by_type_id(*new_level, 0u, lara))
            {
                on_item_selected(lara);
            }
            else
            {
                on_room_selected(0);
            }

            if (new_level->selected_room() < rooms.size())
            {
                _ui->set_selected_room(rooms[new_level->selected_room()].lock());
            }
            
            auto selected_item = new_level->selected_item();
            _ui->set_selected_item(selected_item.value_or(0));

            // Strip the last part of the path away.
            const auto filename = new_level->filename();
            auto last_index = std::min(filename.find_last_of('\\'), filename.find_last_of('/'));
            auto name = last_index == filename.npos ? filename : filename.substr(std::min(last_index + 1, filename.size()));
            _ui->set_level(name, new_level);
            window().set_title("trview - " + name);
        }
        else if (open_mode == ILevel::OpenMode::Reload && old_level)
        {
            new_level->set_alternate_mode(old_level->alternate_mode());
            new_level->set_neighbour_depth(old_level->neighbour_depth());
            new_level->set_highlight_mode(ILevel::RoomHighlightMode::Highlight, old_level->highlight_mode_enabled(ILevel::RoomHighlightMode::Highlight));
            new_level->set_highlight_mode(ILevel::RoomHighlightMode::Neighbours, old_level->highlight_mode_enabled(ILevel::RoomHighlightMode::Neighbours));

            for (const auto& group : new_level->alternate_groups())
            {
                new_level->set_alternate_group(group, old_level->alternate_group(group));
            }
        }

        // Reset UI buttons
        _ui->set_max_rooms(static_cast<uint32_t>(rooms.size()));
        _ui->set_use_alternate_groups(new_level->version() >= trlevel::LevelVersion::Tomb4);
        _ui->set_alternate_groups(new_level->alternate_groups());
        _ui->set_flip_enabled(new_level->any_alternates());

        _scene_changed = true;
    }

    void Viewer::render()
    {
        _timer.update();
        update_camera();

        const auto mouse_pos = client_cursor_position(window());
        if (mouse_pos != _previous_mouse_pos || (_camera_moved || _camera_input.movement().LengthSquared() > 0))
        {
            _picking->pick(current_camera());
        }
        _previous_mouse_pos = mouse_pos;
        _camera_moved = false;

        _device->begin();
        _main_window->begin();
        _main_window->clear(Colour(_settings.background_colour));

        if (_scene_changed)
        {
            _scene_target->clear(Colour::Transparent);

            graphics::RenderTargetStore rs_store(_device->context());
            graphics::ViewportStore vp_store(_device->context());
            _scene_target->apply();

            render_scene();
            _scene_changed = false;
        }

        _scene_sprite->render(_scene_target->texture(), 0, 0, window().size().width, window().size().height);
        _ui->set_camera_position(current_camera().position());
        _ui->set_camera_rotation(current_camera().rotation_yaw(), current_camera().rotation_pitch());
    }

    void Viewer::render_ui()
    {
        _ui->render();
    }

    void Viewer::present(bool vsync)
    {
        _main_window->present(vsync);
    }

    bool Viewer::should_pick() const
    {
        const auto window = this->window();
        const auto level = _level.lock();
        return level && window_under_cursor() == window && !window_is_minimised(window) && !_ui->is_cursor_over() && !cursor_outside_window(window);
    }

    void Viewer::render_scene()
    {
        if (auto level = _level.lock())
        {
            // Update the view matrix based on the room selected in the room window.
            if (level->number_of_rooms() > 0)
            {
                _camera.set_target(_target);
            }
            
            const auto& camera = current_camera();

            level->render(camera, _show_selection);
            auto texture_storage = level->texture_storage();

            _sector_highlight->render(camera, *texture_storage);
            _measure->render(camera, *texture_storage);

            if (_show_route)
            {
                _route->render(camera, *texture_storage, _show_selection);
            }

            level->render_transparency(camera);
            _compass->render(camera, *texture_storage);
        }
    }

    const ICamera& Viewer::current_camera() const
    {
        if (_camera_mode == CameraMode::Orbit)
        {
            return _camera;
        }
        return _free_camera;
    }

    ICamera& Viewer::current_camera()
    {
        if (_camera_mode == CameraMode::Orbit)
        {
            return _camera;
        }
        return _free_camera;
    }

    void Viewer::set_camera_mode(CameraMode camera_mode)
    {
        if (_camera_mode == camera_mode) 
        {
            return;
        }

        _camera_moved = true;
        if (camera_mode == CameraMode::Free || camera_mode == CameraMode::Axis)
        {
            _free_camera.set_alignment(camera_mode_to_alignment(camera_mode));
            if (_camera_mode == CameraMode::Orbit)
            {
                _free_camera.set_position(_camera.position());
                _free_camera.set_rotation_yaw(_camera.rotation_yaw());
                _free_camera.set_rotation_pitch(_camera.rotation_pitch());
            }
        }

        _camera_mode = camera_mode;
        _ui->set_camera_mode(camera_mode);
        _scene_changed = true;
    }

    void Viewer::set_camera_projection_mode(ProjectionMode projection_mode)
    {
        _free_camera.set_projection_mode(projection_mode);
        _camera.set_projection_mode(projection_mode);
        _ui->set_camera_projection_mode(projection_mode);
        _scene_changed = true;
    }

    void Viewer::toggle_highlight()
    {
        if (auto level = _level.lock())
        {
            bool new_value = !level->highlight_mode_enabled(Level::RoomHighlightMode::Highlight);
            level->set_highlight_mode(Level::RoomHighlightMode::Highlight, new_value);
            _ui->set_toggle(Options::highlight, new_value);
        }
    }

    void Viewer::select_room(uint32_t room_number)
    {
        const auto level = _level.lock();
        if (!level || room_number >= level->number_of_rooms())
        {
            return;
        }

        const auto room = level->room(room_number).lock();
        if (!room)
        {
            return;
        }

        _ui->set_selected_room(room);
        _was_alternate_select = false;
        _target = room->centre();
        _scene_changed = true;
        if (_settings.auto_orbit)
        {
            set_camera_mode(CameraMode::Orbit);
        }
    }

    void Viewer::select_item(const std::weak_ptr<IItem>& item)
    {
        if (auto item_ptr = item.lock())
        {
            _target = item_ptr->position();
            _ui->set_selected_item(item_ptr->number());
            if (_settings.auto_orbit)
            {
                set_camera_mode(CameraMode::Orbit);
            }
            _scene_changed = true;
        }
    }

    void Viewer::select_trigger(const std::weak_ptr<ITrigger>& trigger)
    {
        if (auto trigger_ptr = trigger.lock())
        {
            _target = trigger_ptr->position();
            if (_settings.auto_orbit)
            {
                set_camera_mode(CameraMode::Orbit);
            }
            _scene_changed = true;
        }
    }

    void Viewer::select_waypoint(const std::weak_ptr<IWaypoint>& waypoint)
    {
        if (auto waypoint_ptr = waypoint.lock())
        {
            _target = waypoint_ptr->position();
            if (_settings.auto_orbit)
            {
                set_camera_mode(CameraMode::Orbit);
            }
            _scene_changed = true;
        }
    }

    void Viewer::set_route(const std::shared_ptr<IRoute>& route)
    {
        _route = route;
        _ui->set_route(route);
        _scene_changed = true;
    }

    void Viewer::set_show_compass(bool value)
    {
        _compass->set_visible(value);
        _scene_changed = true;
    }

    void Viewer::set_show_minimap(bool value)
    {
        _ui->set_show_minimap(value);
    }

    void Viewer::set_show_route(bool value)
    {
        _show_route = value;
        _scene_changed = true;
    }

    void Viewer::set_show_selection(bool value)
    {
        _show_selection = value;
        _scene_changed = true;
    }

    void Viewer::set_show_tools(bool value)
    {
        _measure->set_visible(value);
        _scene_changed = true;
    }

    void Viewer::set_show_tooltip(bool value)
    {
        _ui->set_show_tooltip(value);
    }

    void Viewer::set_show_ui(bool value)
    {
        _ui->set_visible(value);
    }

    bool Viewer::ui_input_active() const
    {
        return _ui->is_input_active();
    }

    void Viewer::set_alternate_mode(bool enabled)
    {
        if (auto level = _level.lock())
        {
            _was_alternate_select = true;
            level->set_alternate_mode(enabled);
            _ui->set_toggle(Options::flip, enabled);
        }
    }

    void Viewer::toggle_alternate_mode()
    {
        const auto level = _level.lock();
        if (level && level->any_alternates())
        {
            set_alternate_mode(!level->alternate_mode());
        }
    }

    void Viewer::set_alternate_group(uint32_t group, bool enabled)
    {
        if (auto level = _level.lock())
        {
            _was_alternate_select = true;
            level->set_alternate_group(group, enabled);
            _ui->set_alternate_group(group, enabled);
        }
    }

    bool Viewer::alternate_group(uint32_t group) const
    {
        if (const auto level = _level.lock())
        {
            return level->alternate_group(group);
        }
        return false;
    }

    void Viewer::resize_elements()
    {
        const auto size = window().size();
        if (size == Size())
        {
            return;
        }

        // Inform elements that need to know that the device has been resized.
        _camera.set_view_size(size);
        _free_camera.set_view_size(size);
        _ui->set_host_size(size);
        _scene_target = _render_target_source(static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height), graphics::IRenderTarget::DepthStencilMode::Enabled);
        _scene_sprite->set_host_size(size);
        _scene_changed = true;
    }

    // Set up keyboard and mouse input for the camera.
    void Viewer::setup_camera_input()
    {
        using namespace input;

        _token_store += _mouse->mouse_down += [&](auto button) { _camera_input.mouse_down(button); };
        _token_store += _mouse->mouse_up += [&](auto button) { _camera_input.mouse_up(button); };
        _token_store += _mouse->mouse_move += [&](long x, long y) { _camera_input.mouse_move(x, y); };
        _token_store += _mouse->mouse_wheel += [&](int16_t scroll) 
        {
            if (window_under_cursor() == window())
            {
                if (ImGui::GetCurrentContext() != nullptr)
                {
                    auto& io = ImGui::GetIO();
                    if (!io.WantCaptureMouse)
                    {
                        _ui->set_show_context_menu(false);
                        _camera_input.mouse_scroll(scroll);
                    }
                }
            }
        };

        _token_store += _camera_input.on_rotate += [&](float x, float y)
        {
            if (_ui->is_cursor_over())
            {
                return;
            }

            _camera_moved = true;
            _ui->set_show_context_menu(false);

            ICamera& camera = current_camera();
            const float low_sensitivity = 200.0f;
            const float high_sensitivity = 25.0f;
            const float sensitivity = low_sensitivity + (high_sensitivity - low_sensitivity) * _settings.camera_sensitivity;
            camera.set_rotation_yaw(camera.rotation_yaw() + x / sensitivity);
            camera.set_rotation_pitch(camera.rotation_pitch() - y / sensitivity);
            if (auto level = _level.lock())
            {
                level->on_camera_moved();
            }
        };

        _token_store += _camera_input.on_zoom += [&](float zoom)
        {
            if (_ui->is_cursor_over())
            {
                return;
            }
            
            _camera_moved = true;
            if (_camera_mode == CameraMode::Orbit)
            {
                _camera.set_zoom(_camera.zoom() + zoom);
                if (auto level = _level.lock())
                {
                    level->on_camera_moved();
                }
            }
            else if (_free_camera.projection_mode() == ProjectionMode::Orthographic)
            {
                _free_camera.set_zoom(_free_camera.zoom() + zoom);
                if (auto level = _level.lock())
                {
                    level->on_camera_moved();
                }
            }
        };

        _token_store += _camera_input.on_pan += [&](bool vertical, float x, float y)
        {
            if (_ui->is_cursor_over() || _camera_mode != CameraMode::Orbit)
            {
                return;
            }

            _camera_moved = true;
            _ui->set_show_context_menu(false);

            ICamera& camera = current_camera();

            using namespace DirectX::SimpleMath;

            if (camera.projection_mode() == ProjectionMode::Perspective)
            {
                if (vertical)
                {
                    _target += 0.05f * Vector3::Up * y * (_settings.invert_vertical_pan ? -1.0f : 1.0f);
                }
                else
                {
                    // Rotate forward and right by the camera yaw...
                    const auto rotation = Matrix::CreateRotationY(camera.rotation_yaw());
                    const auto forward = Vector3::Transform(Vector3::Forward, rotation);
                    const auto right = Vector3::Transform(Vector3::Right, rotation);

                    // Add them on to the position.
                    const auto movement = 0.05f * (forward * -y + right * -x);
                    _target += movement;
                }
            }
            else
            {
                auto rotate = Matrix::CreateFromYawPitchRoll(camera.rotation_yaw(), camera.rotation_pitch(), 0);
                _target += 0.05f * Vector3::Transform(Vector3(-x, y * (_settings.invert_vertical_pan ? -1.0f : 1.0f), 0), rotate);
            }

            if (auto level = _level.lock())
            {
                level->on_camera_moved();
            }
            _scene_changed = true;
        };

        _token_store += _camera_input.on_mode_change += [&](CameraMode mode) { set_camera_mode(mode); };
    }

    void Viewer::set_show_triggers(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_triggers(show);
            _ui->set_toggle(Options::triggers, show);
        }
    }

    void Viewer::toggle_show_triggers()
    {
        if (auto level = _level.lock())
        {
            set_show_triggers(!level->show_triggers());
        }
    }

    void Viewer::set_show_items(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_items(show);
            _ui->set_toggle(Options::items, show);
        }
    }

    void Viewer::toggle_show_items()
    {
        if (auto level = _level.lock())
        {
            set_show_items(!level->show_items());
        }
    }

    void Viewer::set_show_geometry(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_geometry(show);
            _ui->set_toggle(Options::geometry, show);
        }
    }

    void Viewer::toggle_show_geometry()
    {
        if (auto level = _level.lock())
        {
            set_show_geometry(!level->show_geometry());
        }
    }

    void Viewer::toggle_show_lights()
    {
        if (auto level = _level.lock())
        {
            set_show_lights(!level->show_lights());
        }
    }

    void Viewer::set_show_water(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_water(show);
        }
    }

    void Viewer::set_show_wireframe(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_wireframe(show);
            _ui->set_toggle(Options::wireframe, show);
        }
    }

    void Viewer::set_show_bounding_boxes(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_bounding_boxes(show);
            _ui->set_toggle(Options::show_bounding_boxes, show);
        }
    }

    void Viewer::set_show_lights(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_lights(show);
            _ui->set_toggle(Options::lights, show);
        }
    }

    uint32_t Viewer::room_from_pick(const PickResult& pick) const
    {
        const auto level = _level.lock();
        switch (pick.type)
        {
        case PickResult::Type::Room:
            return pick.index;
        case PickResult::Type::Entity:
        {
            if (level)
            {
                if (auto item = level->item(pick.index).lock())
                {
                    return item_room(item);
                }
            }
            break;
        }
        case PickResult::Type::Trigger:
        {
            if (level)
            {
                if (const auto trigger = level->trigger(pick.index).lock())
                {
                    return trigger_room(trigger);
                }
            }
            break;
        }
        case PickResult::Type::Waypoint:
        {
            if (const auto waypoint = _route->waypoint(pick.index).lock())
            {
                return waypoint->room();
            }
            break;
        }
        }

        return level ? level->selected_room() : 0u;
    }

    void Viewer::add_recent_orbit(const PickResult& pick)
    {
        if (!_recent_orbits.empty())
        {
            _recent_orbits.resize(_recent_orbit_index + 1);
        }
        _recent_orbits.push_back(pick);
        _recent_orbit_index = _recent_orbits.size() - 1;
    }

    void Viewer::select_previous_orbit()
    {
        if (_recent_orbit_index > 0)
        {
            --_recent_orbit_index;
        }

        if (_recent_orbit_index < _recent_orbits.size())
        {
            const auto& pick = _recent_orbits[_recent_orbit_index];
            select_pick(pick);
        }
    }

    void Viewer::select_next_orbit()
    {
        if (_recent_orbits.size() && _recent_orbit_index < _recent_orbits.size() - 1)
        {
            ++_recent_orbit_index;
            const auto& pick = _recent_orbits[_recent_orbit_index];
            select_pick(pick);
        }
    }

    void Viewer::select_pick(const PickResult& pick)
    {
        const auto level = _level.lock();
        switch (pick.type)
        {
        case PickResult::Type::Room:
            on_room_selected(pick.index);
            if (pick.override_centre)
            {
                _target = pick.position;
            }
            break;
        case PickResult::Type::Entity:
        {
            if (level)
            {
                on_item_selected(level->item(pick.index));
            }
            break;
        }
        case PickResult::Type::Trigger:
        {
            if (level)
            {
                on_trigger_selected(level->trigger(pick.index));
            }
            break;
        }
        case PickResult::Type::Waypoint:
        {
            if (level)
            {
                on_waypoint_selected(pick.index);
            }
            break;
        }
        case PickResult::Type::Light:
        {
            if (level)
            {
                on_light_selected(level->light(pick.index));
            }
            break;
        }
        case PickResult::Type::CameraSink:
        {
            if (level)
            {
                on_camera_sink_selected(level->camera_sink(pick.index));
            }
            break;
        }
        }
    }

    void Viewer::apply_camera_settings()
    {
        _free_camera.set_acceleration_settings(_settings.camera_acceleration, _settings.camera_acceleration_rate);
        _free_camera.set_fov(_settings.fov);
        _camera.set_fov(_settings.fov);
    }

    void Viewer::set_settings(const UserSettings& settings)
    {
        _settings = settings;
        _ui->set_settings(_settings);
        apply_camera_settings();
        _scene_changed = true;
    }

    CameraMode Viewer::camera_mode() const
    {
        return _camera_mode;
    }

    void Viewer::select_light(const std::weak_ptr<ILight>& light)
    {
        auto light_ptr = light.lock();
        _target = light_ptr->position();
        if (_settings.auto_orbit)
        {
            set_camera_mode(CameraMode::Orbit);
        }
        _scene_changed = true;
    }

    std::optional<int> Viewer::process_message(UINT message, WPARAM, LPARAM)
    {
        if (message == WM_ACTIVATE)
        {
            _camera_input.reset_input();
        }
        return {};
    }

    DirectX::SimpleMath::Vector3 Viewer::target() const
    {
        return _target;
    }

    void Viewer::set_target(const DirectX::SimpleMath::Vector3& target)
    {
        _target = target;
    }

    void Viewer::set_show_rooms(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_rooms(show);
            _ui->set_toggle(Options::rooms, show);
        }
    }

    void Viewer::select_sector(const std::weak_ptr<ISector>& sector)
    {
        if (auto sector_ptr = sector.lock())
        {
            set_sector_highlight(sector_ptr);
            _ui->set_minimap_highlight(sector_ptr->x(), sector_ptr->z());
        }
    }

    void Viewer::set_sector_highlight(const std::shared_ptr<ISector>& sector)
    {
        const auto level = _level.lock();
        if (!level)
        {
            return;
        }

        const auto room_info = level->room_info(level->selected_room());
        _sector_highlight->set_sector(sector,
            Matrix::CreateTranslation(room_info.x / trlevel::Scale_X, 0, room_info.z / trlevel::Scale_Z));
        _scene_changed = true;
    }

    void Viewer::set_scene_changed()
    {
        _scene_changed = true;
    }

    void Viewer::select_camera_sink(const std::weak_ptr<ICameraSink>& camera_sink)
    {
        auto camera_sink_ptr = camera_sink.lock();
        _target = camera_sink_ptr->position();
        if (_settings.auto_orbit)
        {
            set_camera_mode(CameraMode::Orbit);
        }
        _scene_changed = true;
    }

    void Viewer::set_show_camera_sinks(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_camera_sinks(show);
            _ui->set_toggle(Options::camera_sinks, show);
        }
    }

    void Viewer::toggle_show_camera_sinks()
    {
        if (auto level = _level.lock())
        {
            set_show_camera_sinks(!level->show_camera_sinks());
        }
    }

    void Viewer::set_show_lighting(bool show)
    {
        if (auto level = _level.lock())
        {
            level->set_show_lighting(show);
            _ui->set_toggle(Options::lighting, show);
        }
    }

    void Viewer::toggle_show_lighting()
    {
        if (auto level = _level.lock())
        {
            set_show_lighting(!level->show_lighting());
        }
    }

    void Viewer::select_static_mesh(const std::weak_ptr<IStaticMesh>& static_mesh)
    {
        if (auto static_mesh_ptr = static_mesh.lock())
        {
            _target = static_mesh_ptr->position();
            if (_settings.auto_orbit)
            {
                set_camera_mode(CameraMode::Orbit);
            }
            _scene_changed = true;
        }
    }
}
