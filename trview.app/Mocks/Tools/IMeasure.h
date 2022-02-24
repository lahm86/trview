#pragma once

#include "../../Tools/IMeasure.h"

namespace trview
{
    namespace mocks
    {
        struct MockMeasure : public IMeasure
        {
            virtual ~MockMeasure() = default;
            MOCK_METHOD(void, reset, (), (override));
            MOCK_METHOD(bool, add, (const DirectX::SimpleMath::Vector3&), (override));
            MOCK_METHOD(void, set, (const DirectX::SimpleMath::Vector3&), (override));
            MOCK_METHOD(void, render, (const ICamera&, const ILevelTextureStorage&), (override));
            MOCK_METHOD(std::wstring, distance, (), (const, override));
            MOCK_METHOD(bool, measuring, (), (const, override));
            MOCK_METHOD(void, set_visible, (bool), (override));
        };
    }
}
