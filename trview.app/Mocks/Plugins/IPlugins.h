#pragma once

#include "../../Plugins/IPlugins.h"

namespace trview
{
    namespace mocks
    {
        struct MockPlugins : public IPlugins
        {
            MockPlugins();
            virtual ~MockPlugins();
            MOCK_METHOD(std::vector<std::weak_ptr<IPlugin>>, plugins, (), (const, override));
            MOCK_METHOD(void, initialise, (IApplication*), (override));
            MOCK_METHOD(void, render_ui, (), (override));
            MOCK_METHOD(void, reload, (), (override));
            MOCK_METHOD(void, set_settings, (const UserSettings&), (override));
        };
    }
}
