#include "FileMenu.h"
#include "../Resources/resource.h"
#include <ranges>

namespace trview
{
    namespace
    {
        constexpr int ID_RECENT_FILE_BASE = 5000;
        constexpr int ID_SWITCHFILE_BASE = 10000;

        void update_menu(const Window& window, const std::vector<std::string>& recent_files)
        {
            // Set up the recently used files menu.
            HMENU menu = GetMenu(window);
            HMENU popup = CreatePopupMenu();

            for (auto i = 0u; i < recent_files.size(); ++i)
            {
                AppendMenu(popup, MF_STRING, ID_RECENT_FILE_BASE + i, to_utf16(recent_files[i]).c_str());
            }

            MENUITEMINFO info;
            memset(&info, 0, sizeof(info));
            info.cbSize = sizeof(info);
            info.fMask = MIIM_SUBMENU;
            info.hSubMenu = popup;
            SetMenuItemInfo(menu, ID_FILE_OPENRECENT, FALSE, &info);
            SetMenu(window, menu);
        }

        /// Reset the menu to have no items in it.
        /// @param window The window that the menu bar belongs to.
        /// @param menu The menu to clear.
        void reset_menu(const Window& window, HMENU menu)
        {
            int count = GetMenuItemCount(menu);

            while ((count = GetMenuItemCount(menu)) > 0)
                RemoveMenu(menu, 0, MF_BYPOSITION);

            DrawMenuBar(window);
        }

        /// Create the directory listing menu as a child of the menu in the specified window.
        /// @param window The window that owns the menu.
        /// @returns The new menu.
        HMENU create_directory_listing_menu(const Window& window)
        {
            HMENU menu = GetMenu(window);
            HMENU directory_listing_menu = CreatePopupMenu();

            MENUITEMINFO info;
            memset(&info, 0, sizeof(info));

            info.cbSize = sizeof(info);
            info.fMask = MIIM_SUBMENU;
            info.hSubMenu = directory_listing_menu;

            // Set up the popup menu and grey it out initially - only when it's populated do we enable it
            SetMenuItemInfo(menu, ID_FILE_SWITCHLEVEL, FALSE, &info);
            EnableMenuItem(menu, ID_FILE_SWITCHLEVEL, MF_GRAYED);

            SetMenu(window, menu);

            DrawMenuBar(window);
            return directory_listing_menu;
        }

        std::string trimmed_level_name(const std::string& input)
        {
            return input.substr(input.find_last_of("/\\") + 1);
        }
    }

    IFileMenu::~IFileMenu()
    {
    }

    FileMenu::FileMenu(const Window& window, const std::shared_ptr<IShortcuts>& shortcuts, const std::shared_ptr<IDialogs>& dialogs, const std::shared_ptr<IFiles>& files)
        : MessageHandler(window), _dialogs(dialogs), _directory_listing_menu(create_directory_listing_menu(window)), _files(files)
    {
        DragAcceptFiles(window, TRUE);

        HMENU menu = GetMenu(window);
        EnableMenuItem(menu, ID_FILE_RELOAD, MF_GRAYED);

        _token_store += shortcuts->add_shortcut(false, VK_F5) += [&]() { on_reload(); };
        _token_store += shortcuts->add_shortcut(true, 'O') += [&]() { choose_file(); };
        _token_store += shortcuts->add_shortcut(false, VK_F6) += [&]() { previous_directory_file(); };
        _token_store += shortcuts->add_shortcut(false, VK_F7) += [&]() { next_directory_file();  };
    }

    std::optional<int> FileMenu::process_message(UINT message, WPARAM wParam, LPARAM)
    {
        if (message == WM_COMMAND)
        {
            int wmId = LOWORD(wParam);
            if (wmId >= ID_RECENT_FILE_BASE && wmId <= ID_RECENT_FILE_BASE + static_cast<int>(_recent_files.size()))
            {
                int index = wmId - ID_RECENT_FILE_BASE;
                if (index >= 0 && index < static_cast<int>(_recent_files.size()))
                {
                    on_file_open(_recent_files[index]);
                }
            }
            else if (wmId >= ID_SWITCHFILE_BASE && wmId <= (ID_SWITCHFILE_BASE + GetMenuItemCount(_directory_listing_menu)))
            {
                const auto& f = _file_switcher_list.at(wmId - ID_SWITCHFILE_BASE);
                on_file_open(f.path);
            }
            else if (wmId == ID_FILE_RELOAD)
            {
                on_reload();
            }
            else if (wmId == ID_FILE_OPEN)
            {
                choose_file();
            }
        }
        else if (message == WM_DROPFILES)
        {
            wchar_t filename[MAX_PATH];
            memset(&filename, 0, sizeof(filename));
            DragQueryFile((HDROP)wParam, 0, filename, MAX_PATH);
            on_file_open(to_utf8(filename));
        }

        return std::nullopt;
    }

    void FileMenu::open_file(const std::string& filename, const std::weak_ptr<trlevel::IPack>& pack)
    {
        _opened_file = filename;

        if (const auto pack_ptr = pack.lock())
        {
            _file_switcher_list = valid_pack_levels(*pack_ptr);
        }
        else
        {
            _file_switcher_list = _files->get_files(path_for_filename(filename), default_file_pattern);
        }

        // Enable menu when populating in case it's not enabled
        EnableMenuItem(GetMenu(window()), ID_FILE_SWITCHLEVEL, MF_ENABLED);

        // Clear all items from menu and repopulate
        reset_menu(window(), _directory_listing_menu);
        for (auto i = 0u; i < _file_switcher_list.size(); ++i)
        {
            AppendMenu(_directory_listing_menu, MF_STRING, ID_SWITCHFILE_BASE + static_cast<int>(i), to_utf16(_file_switcher_list[i].friendly_name).c_str());
        }

        EnableMenuItem(GetMenu(window()), ID_FILE_RELOAD, MF_ENABLED);
        DrawMenuBar(window());
    }

    void FileMenu::set_recent_files(const std::list<std::string>& files)
    {
        _recent_files.assign(files.begin(), files.end());
        update_menu(window(), _recent_files);
    }

    void FileMenu::choose_file()
    {
        const auto filename = _dialogs->open_file(L"Open level", { { L"All Tomb Raider Files", { L"*.tr2", L"*.tr4", L"*.trc", L"*.phd", L"*.psx", L"*.obj", L"*.tom", L"*.sat" } } }, OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, _initial_directory);
        if (filename.has_value())
        {
            _initial_directory = filename->directory;
            on_file_open(filename.value().filename);
        }
    }

    void FileMenu::previous_directory_file()
    {
        auto iter = std::find_if(_file_switcher_list.begin(), _file_switcher_list.end(),
            [&](const auto& f) { return f.path == _opened_file; });
        if (iter == _file_switcher_list.end() || iter == _file_switcher_list.begin())
        {
            return;
        }
        --iter;
        on_file_open(iter->path);
    }

    void FileMenu::next_directory_file()
    {
        auto iter = std::find_if(_file_switcher_list.begin(), _file_switcher_list.end(),
            [&](const auto& f) { return f.path == _opened_file; });
        if (iter == _file_switcher_list.end())
        {
            return;
        }
        if (++iter != _file_switcher_list.end())
        {
            on_file_open(iter->path);
        }
    }

    std::vector<std::string> FileMenu::local_levels() const
    {
        std::vector<std::string> files;
        for (const auto& f : _file_switcher_list)
        {
            files.push_back(f.path);
        }
        return files;
    }

    void FileMenu::switch_to(const std::string& filename)
    {
        const auto found = std::ranges::find_if(
            _file_switcher_list,
            [&](const auto& x) -> bool
            {
                return trimmed_level_name(x.friendly_name) == trimmed_level_name(filename);
            });
        if (found != _file_switcher_list.end())
        {
            on_file_open(found->path);
        }
    }

    void FileMenu::render()
    {
    }
}
