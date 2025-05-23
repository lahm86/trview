#pragma once

#include "../../Windows/IDialogs.h"

namespace trview
{
    namespace mocks
    {
        struct MockDialogs : public IDialogs
        {
            MockDialogs();
            virtual ~MockDialogs();
            MOCK_METHOD(bool, message_box, (const std::wstring&, const std::wstring&, Buttons), (const, override));
            MOCK_METHOD(std::optional<FileResult>, open_file, (const std::wstring&, const std::vector<FileFilter>&, uint32_t, std::optional<std::string>), (const, override));
            MOCK_METHOD(std::optional<std::string>, open_folder, (), (const, override));
            MOCK_METHOD(std::optional<FileResult>, save_file, (const std::wstring&, const std::vector<FileFilter>&, uint32_t), (const, override));
        };
    }
}
