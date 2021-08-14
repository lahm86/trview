#pragma once

#include <trview.app/Routing/IWaypoint.h>

namespace trview
{
    namespace mocks
    {
        struct MockWaypoint final : public IWaypoint
        {
            virtual ~MockWaypoint() = default;
            MOCK_METHOD(DirectX::BoundingBox, bounding_box, (), (const, override));
            MOCK_METHOD(DirectX::SimpleMath::Vector3, position, (), (const, override));
            MOCK_METHOD(Type, type, (), (const, override));
            MOCK_METHOD(bool, has_save, (), (const, override));
            MOCK_METHOD(uint32_t, index, (), (const, override));
            MOCK_METHOD(uint32_t, room, (), (const, override));
            MOCK_METHOD(std::wstring, notes, (), (const, override));
            MOCK_METHOD(std::vector<uint8_t>, save_file, (), (const, override));
            MOCK_METHOD(void, set_notes, (const std::wstring&), (override));
            MOCK_METHOD(void, set_route_colour, (const Colour&), (override));
            MOCK_METHOD(void, set_save_file, (const std::vector<uint8_t>&), (override));
            MOCK_METHOD(void, render, (const ICamera&, const ILevelTextureStorage&, const DirectX::SimpleMath::Color&), (override));
            MOCK_METHOD(void, render_join, (const IWaypoint&, const ICamera&, const ILevelTextureStorage&, const DirectX::SimpleMath::Color&), (override));
            MOCK_METHOD(void, get_transparent_triangles, (ITransparencyBuffer&, const ICamera&, const DirectX::SimpleMath::Color&), (override));
            MOCK_METHOD(bool, visible, (), (const, override));
            MOCK_METHOD(void, set_visible, (bool), (override));
            /// <summary>
            /// Index used for testing ordering.
            /// </summary>
            uint32_t test_index;
        };
    }
}
