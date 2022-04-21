
#pragma once

#include <trview.common/Event.h>
#include <trview.app/Routing/IWaypoint.h>
#include <trview.app/Elements/Item.h>
#include <trview.app/Elements/Room.h>
#include <trview.common/Windows/IClipboard.h>
#include <trview.common/Windows/IDialogs.h>
#include <trview.common/IFiles.h>
#include "IRouteWindow.h"
#include <trview.common/Windows/IShell.h>

namespace trview
{
    struct IRoute;

    class RouteWindow final : public IRouteWindow
    {
    public:
        struct Names
        {
            static const std::string colour;
            static const inline std::string delete_waypoint = "Delete Waypoint";
            static const inline std::string attach_save = "Attach Save";
            static const inline std::string export_button = "Export";
            static const inline std::string import_button = "Import";
            static const inline std::string clear_save = "X";
            static const inline std::string notes = "Notes##notes";
            static const inline std::string waypoint_stats = "##waypointstats";
            static const inline std::string randomizer_flags = "Randomizer Flags";
            static const inline std::string waypoint_list_panel = "Waypoint List";
            static const inline std::string waypoint_details_panel = "Waypoint Details";
        };

        explicit RouteWindow(const std::shared_ptr<IClipboard>& clipboard, const std::shared_ptr<IDialogs>& dialogs,
            const std::shared_ptr<IFiles>& files);
        virtual ~RouteWindow() = default;
        virtual void render() override;
        virtual void set_route(IRoute* route) override;
        virtual void select_waypoint(uint32_t index) override;
        virtual void set_items(const std::vector<Item>& items) override;
        virtual void set_rooms(const std::vector<std::weak_ptr<IRoom>>& rooms) override;
        virtual void set_triggers(const std::vector<std::weak_ptr<ITrigger>>& triggers) override;
        virtual void focus() override;
        virtual void update(float delta) override;
        virtual void set_randomizer_enabled(bool value) override;
        virtual void set_randomizer_settings(const RandomizerSettings& settings) override;
    private:
        void load_randomiser_settings(IWaypoint& waypoint);
        void render_waypoint_list();
        void render_waypoint_details();
        bool render_route_window();
        std::string waypoint_text(const IWaypoint& waypoint) const;

        IRoute* _route{ nullptr };
        std::vector<Item> _all_items;
        std::vector<std::weak_ptr<IRoom>> _all_rooms;
        std::vector<std::weak_ptr<ITrigger>> _all_triggers;
        IWaypoint::Type _selected_type{ IWaypoint::Type::Position };
        uint32_t       _selected_index{ 0u };
        std::shared_ptr<IClipboard> _clipboard;
        std::shared_ptr<IDialogs> _dialogs;
        std::shared_ptr<IFiles> _files;
        bool _randomizer_enabled{ false };
        RandomizerSettings _randomizer_settings;
        bool _scroll_to_waypoint{ false };
        std::optional<float> _tooltip_timer;
        bool _need_focus{ false };
    };
}
