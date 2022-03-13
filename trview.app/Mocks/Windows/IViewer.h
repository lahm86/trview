#pragma once

#include "../../Windows/IViewer.h"

namespace trview
{
    namespace mocks
    {
        struct MockViewer : public IViewer
        {
            virtual ~MockViewer() = default;
            MOCK_METHOD(CameraMode, camera_mode, (), (const, override));
            MOCK_METHOD(void, render, (), (override));
            MOCK_METHOD(void, open, (ILevel*), (override));
            MOCK_METHOD(void, set_settings, (const UserSettings&), (override));
            MOCK_METHOD(void, select_item, (const Item&), (override));
            MOCK_METHOD(void, select_room, (uint32_t), (override));
            MOCK_METHOD(void, select_light, (const std::weak_ptr<ILight>&), (override));
            MOCK_METHOD(void, select_trigger, (const std::weak_ptr<ITrigger>&), (override));
            MOCK_METHOD(void, select_waypoint, (const IWaypoint&), (override));
            MOCK_METHOD(void, set_camera_mode, (CameraMode), (override));
            MOCK_METHOD(void, set_route, (const std::shared_ptr<IRoute>&), (override));
            MOCK_METHOD(void, set_show_compass, (bool), (override));
            MOCK_METHOD(void, set_show_minimap, (bool), (override));
            MOCK_METHOD(void, set_show_route, (bool), (override));
            MOCK_METHOD(void, set_show_selection, (bool), (override));
            MOCK_METHOD(void, set_show_tools, (bool), (override));
            MOCK_METHOD(void, set_show_tooltip, (bool), (override));
            MOCK_METHOD(void, set_show_ui, (bool), (override));
            MOCK_METHOD(bool, ui_input_active, (), (const, override));
            MOCK_METHOD(void, present, (bool), (override));
            MOCK_METHOD(void, render_ui, (), (override));
        };
    }
}
