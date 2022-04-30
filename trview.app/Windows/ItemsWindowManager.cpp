#include "ItemsWindowManager.h"
#include <trview.app/Windows/WindowIDs.h>

namespace trview
{
    ItemsWindowManager::ItemsWindowManager(const Window& window, const std::shared_ptr<IShortcuts>& shortcuts, const IItemsWindow::Source& items_window_source)
        : _items_window_source(items_window_source), MessageHandler(window)
    {
        _token_store += shortcuts->add_shortcut(true, 'I') += [&]() { create_window(); };
    }

    std::optional<int> ItemsWindowManager::process_message(UINT message, WPARAM wParam, LPARAM)
    {
        if (message == WM_COMMAND && LOWORD(wParam) == ID_APP_WINDOWS_ITEMS)
        {
            create_window();
        }
        return {};
    }

    void ItemsWindowManager::render()
    {
        WindowManager::render();
    }

    std::weak_ptr<IItemsWindow> ItemsWindowManager::create_window()
    {
        auto items_window = _items_window_source();
        items_window->on_item_selected += on_item_selected;
        items_window->on_item_visibility += on_item_visibility;
        items_window->on_trigger_selected += on_trigger_selected;
        items_window->on_add_to_route += on_add_to_route;
        items_window->set_items(_items);
        items_window->set_triggers(_triggers);
        items_window->set_current_room(_current_room);
        if (_selected_item.has_value())
        {
            items_window->set_selected_item(_selected_item.value());
        }
        return add_window(items_window);
    }

    void ItemsWindowManager::set_items(const std::vector<Item>& items)
    {
        _items = items;
        for (auto& window : _windows)
        {
            window.second->clear_selected_item();
            window.second->set_items(items);
        }
    }

    void ItemsWindowManager::set_item_visible(const Item& item, bool visible)
    {
        auto found = std::find_if(_items.begin(), _items.end(), [&item](const auto& l) { return l.number() == item.number(); });
        if (found == _items.end())
        {
            return;
        }
        found->set_visible(visible);
        for (auto& window : _windows)
        {
            window.second->update_item(*found);
        }
    }

    void ItemsWindowManager::set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers)
    {
        _triggers = triggers;
        for (auto& window : _windows)
        {
            window.second->set_triggers(triggers);
        }
    }

    void ItemsWindowManager::set_room(uint32_t room)
    {
        _current_room = room;
        for (auto& window : _windows)
        {
            window.second->set_current_room(room);
        }
    }

    void ItemsWindowManager::set_selected_item(const Item& item)
    {
        _selected_item = item;
        for (auto& window : _windows)
        {
            window.second->set_selected_item(item);
        }
    }

    void ItemsWindowManager::update(float delta)
    {
        WindowManager::update(delta);
    }
}
