#pragma once

#include <SimpleMath.h>
#include <trview.common/Colour.h>
#include <trview.app/Geometry/IRenderable.h>

namespace trview
{
    /// <summary>
    /// A waypoint in a route.
    /// </summary>
    struct IWaypoint : public IRenderable
    {
        /// <summary>
        /// Defines the possible types of waypoint.
        /// </summary>
        enum class Type
        {
            /// <summary>
            /// The waypoint is a position in the world.
            /// </summary>
            Position,
            /// <summary>
            /// The entity is an entity in the world.
            /// </summary>
            Entity,
            /// <summary>
            /// The waypoint is a trigger in the world.
            /// </summary>
            Trigger,
            /// <summary>
            /// The waypoint is a Randomizer location. This unlocks extra properties and changes the serialisation
            /// behaviour of the route to only export Randomiser Locations in a compatible format.
            /// </summary>
            RandoLocation
        };

        /// <summary>
        /// Create a waypoint.
        /// </summary>
        using Source = std::function<std::unique_ptr<IWaypoint>(const DirectX::SimpleMath::Vector3&, const DirectX::SimpleMath::Vector3&, uint32_t, Type, uint32_t, const Colour&)>;
        /// <summary>
        /// Destructor for IWaypoint.
        /// </summary>
        virtual ~IWaypoint() = 0;
        /// <summary>
        /// Get the bounding box for the waypoint.
        /// </summary>
        /// <returns>The bounding box.</returns>
        virtual DirectX::BoundingBox bounding_box() const = 0;
        /// <summary>
        /// Get the Randomizer 'Difficulty' value.
        /// </summary>
        virtual std::string difficulty() const = 0;
        /// <summary>
        /// Get the position of the waypoint in the 3D view,
        /// </summary>
        /// <returns></returns>
        virtual DirectX::SimpleMath::Vector3 position() const = 0;
        /// <summary>
        /// Get the type of the waypoint.
        /// </summary>
        virtual Type type() const = 0;
        /// <summary>
        /// Get whether the waypoint has an attached save file.
        /// </summary>
        virtual bool has_save() const = 0;
        /// <summary>
        /// Gets the index of the entity or trigger that the waypoint refers to.
        /// </summary>
        virtual uint32_t index() const = 0;
        /// <summary>
        /// Whether the Randomizer 'is item' flag is set.
        /// </summary>
        virtual bool is_item() const = 0;
        /// <summary>
        /// Get any notes associated with the waypoint.
        /// </summary>
        virtual std::wstring notes() const = 0;
        /// <summary>
        /// Whether the Randomizer 'requires glitch' flag is set.
        /// </summary>
        virtual bool requires_glitch() const = 0;
        /// <summary>
        /// Get the room number that the waypoint is in.
        /// </summary>
        virtual uint32_t room() const = 0;
        /// <summary>
        /// Render the join between this waypoint and another.
        /// </summary>
        /// <param name="next_waypoint">The waypoint to join to.</param>
        /// <param name="camera">The camera to use to render.</param>
        /// <param name="texture_storage">The texture storage to use to render.</param>
        /// <param name="colour">The colour for the join.</param>
        virtual void render_join(const IWaypoint& next_waypoint, const ICamera& camera, const ILevelTextureStorage& texture_storage, const DirectX::SimpleMath::Color& colour) = 0;
        /// <summary>
        /// Get the contents of the attached save file.
        /// </summary>
        virtual std::vector<uint8_t> save_file() const = 0;
        /// <summary>
        /// Set the Randomizer 'difficulty' value.
        /// </summary>
        /// <param name="value">The new difficulty value.</param>
        virtual void set_difficulty(const std::string& value) = 0;
        /// <summary>
        /// Set the Randomizer 'is item' flag.
        /// </summary>
        /// <param name="value">The new flag value.</param>
        virtual void set_is_item(bool value) = 0;
        /// Set the notes associated with the waypoint.
        /// @param notes The notes to save.
        virtual void set_notes(const std::wstring& notes) = 0;
        /// <summary>
        /// Set the Randomizer 'requires glitch' flag.
        /// </summary>
        /// <param name="value">The new flag value.</param>
        virtual void set_requires_glitch(bool value) = 0;
        /// Set the route colour for the waypoint blob.
        /// @param colour The colour of the route.
        virtual void set_route_colour(const Colour& colour) = 0;
        /// Set the contents of the attached save file.
        virtual void set_save_file(const std::vector<uint8_t>& data) = 0;
        /// <summary>
        /// Set the Randomizer 'vehicle required' flag.
        /// </summary>
        /// <param name="value">The new flag value.</param>
        virtual void set_vehicle_required(bool value) = 0;
        /// <summary>
        /// Get the Randomizer 'vehicle required' flag.
        /// </summary>
        virtual bool vehicle_required() const = 0;
        /// <summary>
        /// Get the position of the blob on top of the waypoint pole for rendering.
        /// </summary>
        virtual DirectX::SimpleMath::Vector3 blob_position() const = 0;
        /// <summary>
        /// Get the normal to which the waypoint is aligned.
        /// </summary>
        virtual DirectX::SimpleMath::Vector3 normal() const = 0;
    };

    /// <summary>
    /// Convert a string value into an IWaypoint::Type.
    /// </summary>
    /// <param name="value">The string to convert.</param>
    /// <returns>The converted type.</returns>
    IWaypoint::Type waypoint_type_from_string(const std::string& value);
    /// <summary>
    /// Convert an IWaypoint::Type to a string.
    /// </summary>
    /// <param name="type">The type to convert.</param>
    /// <returns>The string value.</returns>
    std::wstring waypoint_type_to_string(IWaypoint::Type type);
}
