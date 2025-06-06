#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <trview.app/Routing/IWaypoint.h>
#include <trview.app/Geometry/PickResult.h>
#include <trview.common/Event.h>
#include <trview.common/IFiles.h>

namespace trview
{
    struct UserSettings;

    /// <summary>
    /// A route is a series of waypoints with notes.
    /// </summary>
    struct IRoute
    {
        struct FileData final
        {
            std::vector<uint8_t> data;
        };

        using Source = std::function<std::shared_ptr<IRoute>(std::optional<FileData>)>;

        Event<> on_changed;
        Event<std::weak_ptr<IWaypoint>> on_waypoint_selected;

        virtual ~IRoute() = 0;
        /// <summary>
        /// Add a new waypoint to the end of the route.
        /// </summary>
        /// <param name="position">The new waypoint.</param>
        /// <param name="normal">The normal to align the waypoint to.</param>
        /// <param name="room">The room the waypoint is in.</param>
        virtual std::shared_ptr<IWaypoint> add(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, uint32_t room) = 0;
        /// <summary>
        /// Add a new waypoint to the end of the route.
        /// </summary>
        /// <param name="position">The position of the waypoint in the world.</param>
        /// <param name="normal">The normal to align the waypoint to.</param>
        /// <param name="room">The room that waypoint is in.</param>
        /// <param name="type">The type of the waypoint.</param>
        /// <param name="type_index">The index of the referred to entity or trigger.</param>
        virtual std::shared_ptr<IWaypoint> add(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, uint32_t room, IWaypoint::Type type, uint32_t type_index) = 0;

        virtual std::shared_ptr<IWaypoint> add(const std::shared_ptr<IWaypoint>& waypoint) = 0;
        /// <summary>
        /// Remove all of the waypoints from the route.
        /// </summary>
        virtual void clear() = 0;
        /// <summary>
        /// Get the colour of the route.
        /// </summary>
        /// <returns>The colour of the route.</returns>
        virtual Colour colour() const = 0;
        virtual std::optional<std::string> filename() const = 0;
        /// <summary>
        /// Insert the new waypoint into the route.
        /// </summary>
        /// <param name="position">The new waypoint.</param>
        /// <param name="normal">The normal to align the waypoint to.</param>
        /// <param name="room">The room that the waypoint is in.</param>
        /// <param name="index">The index in the route list to put the waypoint.</param>
        virtual void insert(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, uint32_t room, uint32_t index) = 0;
        /// <summary>
        /// Insert the new waypoint into the route based on the currently selected waypoint.
        /// </summary>
        /// <param name="position">The new waypoint.</param>
        /// <param name="normal">The normal to align the waypoint to.</param>
        /// <param name="room">The room that the waypoint is in.</param>
        /// <returns>The index of the new waypoint.</returns>
        virtual uint32_t insert(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, uint32_t room) = 0;
        /// <summary>
        /// Insert a new non-positional waypoint.
        /// </summary>
        /// <param name="position">The position of the waypoint in the world.</param>
        /// <param name="normal">The normal to align the waypoint to.</param>
        /// <param name="room">The room that the waypoint is in.</param>
        /// <param name="index">The index in the route list to put the waypoint.</param>
        /// <param name="type">The type of waypoint.</param>
        /// <param name="type_index">The index of the trigger or entity to reference.</param>
        virtual void insert(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, uint32_t room, uint32_t index, IWaypoint::Type type, uint32_t type_index) = 0;
        /// <summary>
        /// Insert a new non-positional waypoint based on the currently selected waypoint.
        /// </summary>
        /// <param name="position">The position of the waypoint in the world.</param>
        /// <param name="normal">The normal to align the waypoint to.</param>
        /// <param name="room">The room that the waypoint is in.</param>
        /// <param name="type">The type of waypoint.</param>
        /// <param name="type_index">The index of the trigger or entity to reference.</param>
        /// <returns>The index of the new waypoint.</returns>
        virtual uint32_t insert(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& normal, uint32_t room, IWaypoint::Type type, uint32_t type_index) = 0;
        /// <summary>
        /// Determines whether the route has any unsaved changes.
        /// </summary>
        /// <returns>True if there are unsaved changes.</returns>
        virtual bool is_unsaved() const = 0;
        virtual std::weak_ptr<ILevel> level() const = 0;
        /// <summary>
        /// Move a waypoint from one index to another.
        /// </summary>
        /// <param name="from">The source index.</param>
        /// <param name="to">Destination index.</param>
        virtual void move(int32_t from, int32_t to) = 0;
        /// <summary>
        /// Pick against the waypoints in the route.
        /// </summary>
        /// <param name="position">The position of the camera.</param>
        /// <param name="direction">The direction of the ray.</param>
        /// <returns>The pcik result.</returns>
        virtual PickResult pick(const DirectX::SimpleMath::Vector3& position, const DirectX::SimpleMath::Vector3& direction) const = 0;
        virtual void reload(const std::shared_ptr<IFiles>& files, const UserSettings& settings) = 0;
        /// <summary>
        /// Remove the waypoint at the specified index.
        /// </summary>
        /// <param name="index">The index of the waypoint to remove.</param>
        virtual void remove(uint32_t index) = 0;
        virtual void remove(const std::shared_ptr<IWaypoint>& waypoint) = 0;
        /// <summary>
        /// Render the route.
        /// </summary>
        /// <param name="camera">The camera to use to render.</param>
        /// <param name="texture_storage">Texture storage for the mesh.</param>
        /// <param name="show_selection">Whether to show the selection outline.</param>
        virtual void render(const ICamera& camera, bool show_selection) = 0;
        virtual void save(const std::shared_ptr<IFiles>& files, const UserSettings& settings) = 0;
        virtual void save_as(const std::shared_ptr<IFiles>& files, const std::string& filename, const UserSettings& settings) = 0;
        /// <summary>
        /// Get the index of the currently selected waypoint.
        /// </summary>
        /// <returns>The index of the currently selected waypoint.</returns>
        virtual uint32_t selected_waypoint() const = 0;
        virtual void select_waypoint(const std::weak_ptr<IWaypoint>& waypoint) = 0;
        /// <summary>
        /// Set the colour for the route.
        /// </summary>
        /// <param name="colour">The colour to use.</param>
        virtual void set_colour(const Colour& colour) = 0;
        virtual void set_filename(const std::string& filename) = 0;
        virtual void set_show_route_line(bool show) = 0;
        virtual void set_level(const std::weak_ptr<ILevel>& level) = 0;
        /// <summary>
        /// Set the colour for the stick.
        /// </summary>
        /// <param name="colour">The colour to use.</param>
        virtual void set_waypoint_colour(const Colour& colour) = 0;
        /// <summary>
        /// Set whether the route has unsaved changes.
        /// </summary>
        /// <param name="value">Whether the route has unsaved changes.</param>
        virtual void set_unsaved(bool value) = 0;
        virtual bool show_route_line() const = 0;
        /// <summary>
        /// Get the colour to use for the stick.
        /// </summary>
        virtual Colour waypoint_colour() const = 0;
        /// <summary>
        /// Get the waypoint at the specified index.
        /// </summary>
        /// <param name="index">The index to get.</param>
        /// <returns>The waypoint.</returns>
        virtual std::weak_ptr<IWaypoint> waypoint(uint32_t index) const = 0;
        /// <summary>
        /// Get the number of waypoints in the route.
        /// </summary>
        /// <returns>The number of waypoints in the route.</returns>
        virtual uint32_t waypoints() const = 0;
    };

    std::shared_ptr<IRoute> import_route(const IRoute::Source& route_source, const std::shared_ptr<IFiles>& files, const std::string& route_filename);
}