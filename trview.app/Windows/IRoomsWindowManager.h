#pragma once

#include "RoomsWindow.h"

namespace trview
{
    struct IRoomsWindowManager
    {
        virtual ~IRoomsWindowManager() = 0;

        /// Event raised when the user has selected a room in the room window.
        Event<uint32_t> on_room_selected;

        /// Event raised when the user has selected an item in the room window.
        Event<Item> on_item_selected;

        /// Event raised when the user has selected a trigger in the room window.
        Event<std::weak_ptr<ITrigger>> on_trigger_selected;

        /// Render all of the rooms windows.
        /// @param vsync Whether to use vsync.
        virtual void render(bool vsync) = 0;

        /// Set the items in the current level.
        virtual void set_items(const std::vector<Item>& items) = 0;
        /// <summary>
        /// Set the level version for the current level. This will control which information is displayed.
        /// </summary>
        /// <param name="version">The level version.</param>
        virtual void set_level_version(trlevel::LevelVersion version) = 0;
        /// Set the current room that the viewer is focusing on.
        /// @param room The current room.
        virtual void set_room(uint32_t room) = 0;

        /// Set the rooms to display in the window.
        /// @param rooms The rooms to show.
        virtual void set_rooms(const std::vector<std::weak_ptr<IRoom>>& items) = 0;

        /// Set the item currently selected in the viewer.
        /// @param item The item currently selected.
        virtual void set_selected_item(const Item& item) = 0;

        /// Set the trigger currently selected in the viewer.
        /// @param trigger The trigger currently selected.
        virtual void set_selected_trigger(const std::weak_ptr<ITrigger>& trigger) = 0;

        /// Set the triggers in the level.
        /// @param triggers The triggers in the level.
        virtual void set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers) = 0;

        /// Create a new rooms window.
        virtual std::weak_ptr<IRoomsWindow> create_window() = 0;
        /// <summary>
        /// Update the windows.
        /// </summary>
        /// <param name="delta">Elapsed time since previous update.</param>
        virtual void update(float delta) = 0;
    };
}
