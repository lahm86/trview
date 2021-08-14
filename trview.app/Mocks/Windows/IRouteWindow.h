#pragma once

#include <trview.app/Windows/IRouteWindow.h>

namespace trview
{
    namespace mocks
    {
        struct MockRouteWindow : public IRouteWindow
        {
            virtual ~MockRouteWindow() = default;
            MOCK_METHOD(void, render, (bool), (override));
            MOCK_METHOD(void, set_route, (IRoute*), (override));
            MOCK_METHOD(void, select_waypoint, (uint32_t), (override));
            MOCK_METHOD(void, set_items, (const std::vector<Item>&), (override));
            MOCK_METHOD(void, set_rooms, (const std::vector<std::weak_ptr<IRoom>>&), (override));
            MOCK_METHOD(void, set_triggers, (const std::vector<std::weak_ptr<ITrigger>>&), (override));
            MOCK_METHOD(void, focus, (), (override));
            MOCK_METHOD(void, update, (float), (override));
        };
    }
}