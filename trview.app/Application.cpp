#include "Application.h"
#include <trlevel/LevelEncryptedException.h>

#include "Resources/resource.h"

using namespace DirectX::SimpleMath;

namespace trview
{
    namespace
    {
        INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
        {
            UNREFERENCED_PARAMETER(lParam);
            switch (message)
            {
                case WM_INITDIALOG:
                    return (INT_PTR)TRUE;
                case WM_COMMAND:
                    if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
                    {
                        EndDialog(hDlg, LOWORD(wParam));
                        return (INT_PTR)TRUE;
                    }
                    break;
            }
            return (INT_PTR)FALSE;
        }
    }

    IApplication::~IApplication()
    {
    }

    Application::Application(const Window& application_window,
        std::unique_ptr<IUpdateChecker> update_checker,
        std::shared_ptr<ISettingsLoader> settings_loader,
        const trlevel::ILevel::Source& trlevel_source,
        std::unique_ptr<IFileMenu> file_menu,
        std::unique_ptr<IViewer> viewer,
        const IRoute::Source& route_source,
        std::shared_ptr<IShortcuts> shortcuts,
        std::unique_ptr<IItemsWindowManager> items_window_manager,
        std::unique_ptr<ITriggersWindowManager> triggers_window_manager,
        std::unique_ptr<IRouteWindowManager> route_window_manager,
        std::unique_ptr<IRoomsWindowManager> rooms_window_manager,
        const ILevel::Source& level_source,
        std::shared_ptr<IStartupOptions> startup_options,
        std::shared_ptr<IDialogs> dialogs,
        std::shared_ptr<IFiles> files,
        std::unique_ptr<IImGuiBackend> imgui_backend,
        std::unique_ptr<ILightsWindowManager> lights_window_manager,
        std::unique_ptr<ILogWindowManager> log_window_manager,
        std::unique_ptr<ITexturesWindowManager> textures_window_manager)
        : MessageHandler(application_window), _instance(GetModuleHandle(nullptr)),
        _file_menu(std::move(file_menu)), _update_checker(std::move(update_checker)), _view_menu(window()), _settings_loader(settings_loader), _trlevel_source(trlevel_source),
        _viewer(std::move(viewer)), _route_source(route_source), _route(route_source()), _shortcuts(shortcuts), _items_windows(std::move(items_window_manager)),
        _triggers_windows(std::move(triggers_window_manager)), _route_window(std::move(route_window_manager)), _rooms_windows(std::move(rooms_window_manager)), _level_source(level_source),
        _dialogs(dialogs), _files(files), _timer(default_time_source()), _imgui_backend(std::move(imgui_backend)), _lights_windows(std::move(lights_window_manager)), _log_windows(std::move(log_window_manager)),
        _textures_windows(std::move(textures_window_manager))
    {
        SetWindowLongPtr(window(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(_imgui_backend.get()));

        _update_checker->check_for_updates();
        _settings = _settings_loader->load_user_settings();

        _file_menu->set_recent_files(_settings.recent_files);
        _token_store += _file_menu->on_file_open += [=](const auto& file) { open(file, ILevel::OpenMode::Full); };
        _token_store += _file_menu->on_reload += [=]() { reload(); };

        setup_shortcuts();
        setup_view_menu();
        setup_items_windows();
        setup_triggers_windows();
        setup_rooms_windows();
        setup_route_window();
        setup_lights_windows();
        setup_viewer(*startup_options);

        register_lua();
        lua_init(&lua_registry);
    }

    Application::~Application()
    {
        SetWindowLongPtr(window(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
        _settings_loader->save_user_settings(_settings);
        if (_imgui_setup)
        {
            _imgui_backend->shutdown();
            ImGui::DestroyContext();
        }
    }

    void Application::open(const std::string& filename, ILevel::OpenMode open_mode)
    {
        if (open_mode == ILevel::OpenMode::Full && !should_discard_changes())
        {
            return;
        }

        std::unique_ptr<trlevel::ILevel> new_level;
        try
        {
            new_level = _trlevel_source(filename);
        }
        catch (trlevel::LevelEncryptedException&)
        {
            _dialogs->message_box(L"Level is encrypted and cannot be loaded", L"Error", IDialogs::Buttons::OK);
            return;
        }
        catch (...)
        {
            _dialogs->message_box(L"Failed to load level", L"Error", IDialogs::Buttons::OK);
            return;
        }

        _file_menu->open_file(filename);
        _settings.add_recent_file(filename);
        _file_menu->set_recent_files(_settings.recent_files);
        _settings_loader->save_user_settings(_settings);
        _viewer->set_settings(_settings);

        auto old_level = std::move(_level);
        _level = _level_source(std::move(new_level));
        _level->set_filename(filename);

        _level->set_map_colours(_settings.map_colours);

        _items_windows->set_items(_level->items());
        _items_windows->set_triggers(_level->triggers());
        _items_windows->set_level_version(_level->version());
        _items_windows->set_model_checker([&](uint32_t id) { return _level->has_model(id); });
        _triggers_windows->set_items(_level->items());
        _triggers_windows->set_triggers(_level->triggers());
        _rooms_windows->set_level_version(_level->version());
        _rooms_windows->set_items(_level->items());
        _rooms_windows->set_triggers(_level->triggers());
        _rooms_windows->set_rooms(_level->rooms());
        _route_window->set_items(_level->items());
        _route_window->set_triggers(_level->triggers());
        _route_window->set_rooms(_level->rooms());
        _lights_windows->set_level_version(_level->version());
        _lights_windows->set_lights(_level->lights());
        if (open_mode == ILevel::OpenMode::Full)
        {
            _route->clear();
            _route->set_colour(_settings.route_colour);
            _route->set_waypoint_colour(_settings.waypoint_colour);
            _route->set_unsaved(false);
            _route_window->set_route(_route.get());
        }
        _textures_windows->set_texture_storage(_level->texture_storage());

        _viewer->open(_level.get(), open_mode);
        _viewer->set_route(_route);

        if (old_level && open_mode == ILevel::OpenMode::Reload)
        {
            const Vector3 old_target = _viewer->target();
            const bool old_auto_orbit = _settings.auto_orbit;
            _settings.auto_orbit = false;
            _viewer->set_settings(_settings);

            auto selected_item = old_level->selected_item();
            if (selected_item.has_value())
            {
                const auto new_selected_item = _level->item(selected_item.value());
                if (new_selected_item.has_value())
                {
                    select_item(new_selected_item.value());
                }
            }

            auto selected_trigger = old_level->selected_trigger();
            if (selected_trigger.has_value())
            {
                select_trigger(_level->trigger(selected_trigger.value()));
            }

            auto selected_light = old_level->selected_light();
            if (selected_light.has_value())
            {
                select_light(_level->light(selected_light.value()));
            }

            select_room(old_level->selected_room());

            _viewer->set_target(old_target);
            _settings.auto_orbit = old_auto_orbit;
            _viewer->set_settings(_settings);
        }
        else
        {
            _recent_route_prompted = false;
            open_recent_route();
        }
    }

    std::optional<int> Application::process_message(UINT message, WPARAM wParam, LPARAM)
    {
        switch (message)
        {
            case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);

                // Parse the menu selections:
                switch (wmId)
                {
                    case IDM_ABOUT:
                        DialogBox(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_ABOUTBOX), window(), About);
                        break;
                    case ID_HELP_GITHUB:
                    {
                        ShellExecute(0, 0, L"https://github.com/chreden/trview", 0, 0, SW_SHOW);
                        break;
                    }
                    case ID_HELP_DISCORD:
                    {
                        ShellExecute(0, 0, L"https://discord.gg/Zy7kYge", 0, 0, SW_SHOW);
                        break;
                    }
                    case IDM_EXIT:
                    {
                        if (should_discard_changes())
                        {
                            on_closing();
                            DestroyWindow(window());
                        }
                        break;
                    }
                    case ID_WINDOWS_RESET_LAYOUT:
                    {
                        auto& io = ImGui::GetIO();
                        _files->delete_file(io.IniFilename);
                        io.IniFilename = nullptr;
                        _imgui_setup = false;
                        _imgui_backend->shutdown();
                        break;
                    }
                }
                break;
            }
            case WM_CLOSE:
            {
                if (should_discard_changes())
                {
                    on_closing();
                    DestroyWindow(window());
                }
                return 0;
            }
            case WM_DESTROY:
            {
                PostQuitMessage(0);
                break;
            }
        }
        return {};
    }

    int Application::run()
    {
        HACCEL hAccelTable = LoadAccelerators(_instance, MAKEINTRESOURCE(IDC_TRVIEW));

        MSG msg;
        memset(&msg, 0, sizeof(msg));

        while (msg.message != WM_QUIT)
        {
            while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    return (int)msg.wParam;
                }

                if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }

            save_window_placement();
            render();
            Sleep(1);
        }

        return (int)msg.wParam;
    }
        
    void Application::setup_view_menu()
    {
        _token_store += _view_menu.on_show_minimap += [&](bool show) { _viewer->set_show_minimap(show); };
        _token_store += _view_menu.on_show_tooltip += [&](bool show) { _viewer->set_show_tooltip(show); };
        _token_store += _view_menu.on_show_ui += [&](bool show) { _viewer->set_show_ui(show); };
        _token_store += _view_menu.on_show_compass += [&](bool show) { _viewer->set_show_compass(show); };
        _token_store += _view_menu.on_show_selection += [&](bool show) { _viewer->set_show_selection(show); };
        _token_store += _view_menu.on_show_route += [&](bool show) { _viewer->set_show_route(show); };
        _token_store += _view_menu.on_show_tools += [&](bool show) { _viewer->set_show_tools(show); };
        _token_store += _view_menu.on_unhide_all += [&]()
        {
            if (!_level)
            {
                return;
            }

            for (const auto& item : _level->items()) { if (!item.visible()) { set_item_visibility(item, true); } }
            for (const auto& trigger : _level->triggers()) { set_trigger_visibility(trigger, true); }
            for (const auto& light : _level->lights()) { set_light_visibility(light, true); }
            for (const auto& room : _level->rooms()) { set_room_visibility(room, true); }
        };
    }

    void Application::setup_viewer(const IStartupOptions& startup_options)
    {
        _token_store += _viewer->on_item_visibility += [this](const auto& item, bool value) { set_item_visibility(item, value); };
        _token_store += _viewer->on_item_selected += [this](const auto& item) { select_item(item); };
        _token_store += _viewer->on_room_selected += [this](uint32_t room) { select_room(room); };
        _token_store += _viewer->on_trigger_selected += [this](const auto& trigger) { select_trigger(trigger); };
        _token_store += _viewer->on_trigger_visibility += [this](const auto& trigger, bool value) { set_trigger_visibility(trigger, value); };
        _token_store += _viewer->on_light_selected += [this](const auto& light) { select_light(light); };
        _token_store += _viewer->on_light_visibility += [this](const auto& light, bool value) { set_light_visibility(light, value); };
        _token_store += _viewer->on_room_visibility += [this](const auto& room, bool value) { set_room_visibility(room, value); };
        _token_store += _viewer->on_waypoint_added += [this](const auto& position, const auto& normal, auto room, auto type, auto index) { add_waypoint(position, normal, room, type, index); };
        _token_store += _viewer->on_waypoint_selected += [this](auto index) { select_waypoint(index); };
        _token_store += _viewer->on_waypoint_removed += [this](auto index) { remove_waypoint(index); };
        _token_store += _viewer->on_settings += [this](auto&& settings)
        {
            _settings = settings;
            _viewer->set_settings(_settings);
            _route_window->set_randomizer_enabled(settings.randomizer_tools);
            _route->set_randomizer_enabled(settings.randomizer_tools);
            _rooms_windows->set_map_colours(settings.map_colours);
            if (_level)
            {
                _level->set_map_colours(settings.map_colours);
            }
        };
        _viewer->set_settings(_settings);

        auto filename = startup_options.filename();
        if (!filename.empty())
        {
            open(filename, ILevel::OpenMode::Full);
        }
    }

    void Application::setup_items_windows()
    {
        if (_settings.items_startup)
        {
            _items_windows->create_window();
        }

        _token_store += _items_windows->on_item_selected += [this](const auto& item) { select_item(item); };
        _token_store += _items_windows->on_item_visibility += [this](const auto& item, bool state) { set_item_visibility(item, state); };
        _token_store += _items_windows->on_trigger_selected += [this](const auto& trigger) { select_trigger(trigger); };
        _token_store += _items_windows->on_add_to_route += [this](const auto& item)
        {
            add_waypoint(item.position(), Vector3::Down, item.room(), IWaypoint::Type::Entity, item.number());
        };
    }

    void Application::setup_triggers_windows()
    {
        if (_settings.triggers_startup)
        {
            _triggers_windows->create_window();
        }
        _token_store += _triggers_windows->on_item_selected += [this](const auto& item) { select_item(item); };
        _token_store += _triggers_windows->on_trigger_selected += [this](const auto& trigger) { select_trigger(trigger); };
        _token_store += _triggers_windows->on_trigger_visibility += [this](const auto& trigger, bool state) { set_trigger_visibility(trigger, state); };
        _token_store += _triggers_windows->on_add_to_route += [this](const auto& trigger)
        {
            if (auto trigger_ptr = trigger.lock())
            {
                add_waypoint(trigger_ptr->position(), Vector3::Down, trigger_ptr->room(), IWaypoint::Type::Trigger, trigger_ptr->number());
            }
        };
    }

    void Application::setup_rooms_windows()
    {
        _rooms_windows->set_map_colours(_settings.map_colours);

        if (_settings.rooms_startup)
        {
            _rooms_windows->create_window();
        }

        _token_store += _rooms_windows->on_room_selected += [this](const auto& room) { select_room(room); };
        _token_store += _rooms_windows->on_item_selected += [this](const auto& item) { select_item(item); };
        _token_store += _rooms_windows->on_trigger_selected += [this](const auto& trigger) { select_trigger(trigger); };
        _token_store += _rooms_windows->on_room_visibility += [this](const auto& room, bool value) { set_room_visibility(room, value); };
    }

    void Application::setup_route_window()
    {
        _token_store += _route_window->on_waypoint_selected += [&](auto index) { select_waypoint(index); };
        _token_store += _route_window->on_item_selected += [&](const auto& item) { select_item(item); };
        _token_store += _route_window->on_trigger_selected += [&](const auto& trigger) { select_trigger(trigger); };
        _token_store += _route_window->on_route_import += [&](const std::string& path, bool is_rando) { this->import_route(path, is_rando); };
        _token_store += _route_window->on_route_export += [&](const std::string& path, bool is_rando)
        {
            export_route(*_route, _files, path, _level ? _level->filename() : "", _settings.randomizer, is_rando);
            _route->set_unsaved(false);
            if (_level)
            {
                _settings.recent_routes[_level->filename()] = { path, is_rando };
                _viewer->set_settings(_settings);
            }
        };
        _token_store += _route_window->on_waypoint_deleted += [&](auto index) { remove_waypoint(index); };
        _token_store += _route_window->on_colour_changed += [&](const Colour& colour)
        {
            _route->set_colour(colour);
            _viewer->set_route(_route);
        };
        _token_store += _route_window->on_waypoint_colour_changed += [&](const Colour& colour)
        {
            _route->set_waypoint_colour(colour);
            _viewer->set_route(_route);
        };
        _token_store += _route_window->on_waypoint_reordered += [&](int32_t from, int32_t to)
        {
            _route->move(from, to);
            _viewer->set_route(_route);
        };
        _route_window->set_randomizer_enabled(_settings.randomizer_tools);
        _route->set_randomizer_enabled(_settings.randomizer_tools);
        _route_window->set_randomizer_settings(_settings.randomizer);
        _token_store += _route_window->on_window_created += [&]() { open_recent_route(); };

        if (_settings.route_startup)
        {
            _route_window->create_window();
        }
    }

    void Application::setup_lights_windows()
    {
        _token_store += _lights_windows->on_light_selected += [this](const auto& light) { select_light(light); };
        _token_store += _lights_windows->on_light_visibility += [this](const auto& light, bool value) { set_light_visibility(light, value); };
    }

    void Application::setup_shortcuts()
    {
        auto add_shortcut = [&](bool control, uint16_t key, std::function<void()> fn)
        {
            _token_store += _shortcuts->add_shortcut(control, key) += [&, fn]()
            {
                if (!_viewer->ui_input_active()) { fn(); }
            };
        };

        add_shortcut(false, VK_LEFT, [&]() { select_previous_waypoint(); });
        add_shortcut(false, VK_RIGHT, [&]() { select_next_waypoint(); });
        add_shortcut(false, VK_DELETE, [&]() { remove_waypoint(_route->selected_waypoint()); });
    }

    void Application::add_waypoint(const Vector3& position, const Vector3& normal, uint32_t room, IWaypoint::Type type, uint32_t index)
    {
        uint32_t new_index = _route->insert(position, normal, room, type, index);
        _route_window->set_route(_route.get());
        select_waypoint(new_index);
    }

    void Application::remove_waypoint(uint32_t index)
    {
        _route->remove(index);
        _route_window->set_route(_route.get());
        _viewer->set_route(_route);

        if (_route->waypoints() > 0)
        {
            select_waypoint(_route->selected_waypoint());
        }
    }

    void Application::select_item(const Item& item)
    {
        if (!_level || item.number() >= _level->items().size())
        {
            return;
        }

        select_room(item.room());
        _level->set_selected_item(item.number());
        _viewer->select_item(item);
        _items_windows->set_selected_item(item);
        _rooms_windows->set_selected_item(item);
    }

    void Application::select_room(uint32_t room)
    {
        if (!_level || room >= _level->number_of_rooms())
        {
            return;
        }

        _level->set_selected_room(static_cast<uint16_t>(room));
        _viewer->select_room(room);
        _items_windows->set_room(room);
        _rooms_windows->set_room(room);
        _triggers_windows->set_room(room);
        _lights_windows->set_room(room);
    }

    void Application::select_trigger(const std::weak_ptr<ITrigger>& trigger)
    {
        if (!_level)
        {
            return;
        }

        auto trigger_ptr = trigger.lock();
        if (!trigger_ptr)
        {
            return;
        }

        select_room(trigger_ptr->room());
        _level->set_selected_trigger(trigger_ptr->number());
        _viewer->select_trigger(trigger);
        _triggers_windows->set_selected_trigger(trigger);
        _rooms_windows->set_selected_trigger(trigger);
    }

    void Application::select_waypoint(uint32_t index)
    {
        const auto& waypoint = _route->waypoint(index);
        select_room(_route->waypoint(index).room());
        _route->select_waypoint(index);
        _viewer->select_waypoint(waypoint);
        _route_window->select_waypoint(index);
    }

    void Application::select_next_waypoint()
    {
        if (_route->selected_waypoint() + 1 < _route->waypoints())
        {
            select_waypoint(_route->selected_waypoint() + 1);
        }
    }

    void Application::select_previous_waypoint()
    {
        if (_route->selected_waypoint() > 0)
        {
            select_waypoint(_route->selected_waypoint() - 1);
        }
    }

    void Application::select_light(const std::weak_ptr<ILight>& light)
    {
        if (!_level)
        {
            return;
        }

        auto light_ptr = light.lock();
        if (!light_ptr)
        {
            return;
        }

        select_room(light_ptr->room());
        _level->set_selected_light(light_ptr->number());
        _viewer->select_light(light);
        _lights_windows->set_selected_light(light);
    }

    void Application::set_item_visibility(const Item& item, bool visible)
    {
        if (!_level)
        {
            return;
        }

        _level->set_item_visibility(item.number(), visible);
        _items_windows->set_item_visible(item, visible);
    }

    void Application::set_trigger_visibility(const std::weak_ptr<ITrigger>& trigger, bool visible)
    {
        if (!_level)
        {
            return;
        }

        if (const auto trigger_ptr = trigger.lock())
        {
            if (trigger_ptr->visible() != visible)
            {
                _level->set_trigger_visibility(trigger_ptr->number(), visible);
            }
        }
    }

    void Application::set_light_visibility(const std::weak_ptr<ILight>& light, bool visible)
    {
        if (!_level)
        {
            return;
        }

        if (const auto light_ptr = light.lock())
        {
            if (light_ptr->visible() != visible)
            {
                _level->set_light_visibility(light_ptr->number(), visible);
            }
        }
    }

    void Application::set_room_visibility(const std::weak_ptr<IRoom>& room, bool visible)
    {
        if (!_level)
        {
            return;
        }

        if (const auto room_ptr = room.lock())
        {
            if (room_ptr->visible() != visible)
            {
                _level->set_room_visibility(room_ptr->number(), visible);
            }
        }
    }

    void Application::render()
    {
        // If minimised, don't render like crazy. Sleep so we don't hammer the CPU either.
        if (window_is_minimised(window()))
        {
            Sleep(1);
            return;
        }

        if (!_imgui_setup)
        {
            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            auto existing_context = ImGui::GetCurrentContext();
            if (existing_context)
            {
                ImGui::DestroyContext(existing_context);
            }
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

            _imgui_ini_filename = _files->appdata_directory() + "\\trview\\imgui.ini";
            io.IniFilename = _imgui_ini_filename.c_str();

            // Setup Dear ImGui style
            ImGui::StyleColorsDark();

            _font = io.Fonts->AddFontFromFileTTF((_files->fonts_directory() + "\\Arial.ttf").c_str(), 12.0f);

            // Setup Platform/Renderer backends
            _imgui_backend->initialise();
            _imgui_setup = true;
        }

        _timer.update();
        const auto elapsed = _timer.elapsed();
        _items_windows->update(elapsed);
        _triggers_windows->update(elapsed);
        _rooms_windows->update(elapsed);
        _route_window->update(elapsed);
        _lights_windows->update(elapsed);

        _viewer->render();

        _imgui_backend->new_frame();
        ImGui::NewFrame();

        ImGui::PushFont(_font);

        _viewer->render_ui();
        _items_windows->render();
        _triggers_windows->render();
        _rooms_windows->render();
        _route_window->render();
        _lights_windows->render();
        _log_windows->render();
        _textures_windows->render();

        ImGui::PopFont();
        ImGui::Render();
        _imgui_backend->render();

        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        _viewer->present(_settings.vsync);
    }

    bool Application::should_discard_changes()
    {
        if (_route->is_unsaved())
        {
            return _dialogs->message_box(L"Route has unsaved changes. Do you want to continue and lose changes?", L"Unsaved Route Changes", IDialogs::Buttons::Yes_No);
        }
        return true;
    }

    void Application::reload()
    {
        if (!_level)
        {
            return;
        }
        open(_level->filename(), ILevel::OpenMode::Reload);
    }

    void Application::import_route(const std::string& path, bool is_rando)
    {
        auto route = trview::import_route(_route_source, _files, path, _level.get(), _settings.randomizer, is_rando);
        if (route)
        {
            _route = route;
            _route_window->set_route(_route.get());
            _route->set_randomizer_enabled(_settings.randomizer_tools);
            _viewer->set_route(_route);
            if (_level)
            {
                _settings.recent_routes[_level->filename()] = { path, is_rando };
                _viewer->set_settings(_settings);
            }
        }
    }

    void Application::open_recent_route()
    {
        if (!_level || _recent_route_prompted || !_route_window->is_window_open())
        {
            return;
        }

        const auto recent_route = _settings.recent_routes.find(_level->filename());
        if (recent_route != _settings.recent_routes.end() &&
            _dialogs->message_box(L"Reopen last used route for this level?", L"Reopen route", IDialogs::Buttons::Yes_No))
        {
            this->import_route(recent_route->second.route_path, recent_route->second.is_rando);
        }
        else
        {
            _settings.recent_routes.erase(_level->filename());
            _viewer->set_settings(_settings);
        }
        _recent_route_prompted = true;
    }

    void Application::save_window_placement()
    {
        WINDOWPLACEMENT placement{};
        placement.length = sizeof(placement);
        if (GetWindowPlacement(window(), &placement))
        {
            _settings.window_placement =
            {
                placement.ptMinPosition.x, placement.ptMinPosition.y,
                placement.ptMaxPosition.x, placement.ptMaxPosition.y,
                placement.rcNormalPosition.left, placement.rcNormalPosition.top, placement.rcNormalPosition.right, placement.rcNormalPosition.bottom
            };
        }
    }
}