#pragma once

#include "../../Settings/ISettingsLoader.h"

namespace trview
{
    namespace mocks
    {
        struct MockSettingsLoader : public ISettingsLoader
        {
            virtual ~MockSettingsLoader() = default;
            MOCK_METHOD(UserSettings, load_user_settings, (), (const, override));
            MOCK_METHOD(void, save_user_settings, (const UserSettings&), (override));
        };
    }
}
