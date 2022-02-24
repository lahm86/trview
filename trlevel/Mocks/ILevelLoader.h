#pragma once

#include "../ILevelLoader.h"

namespace trlevel
{
    namespace mocks
    {
        struct MockLevelLoader : public ILevelLoader
        {
            virtual ~MockLevelLoader() = default;
            MOCK_METHOD(std::unique_ptr<ILevel>, load_level, (const std::string&), (const, override));
        };
    }
}
