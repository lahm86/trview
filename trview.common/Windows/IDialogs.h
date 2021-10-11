#pragma once

#include <optional>
#include <string>
#include <vector>
#include "../Window.h"

namespace trview
{
    struct IDialogs
    {
        enum class Buttons
        {
            OK,
            OK_Cancel,
            Yes_No
        };

        struct SaveFileResult
        {
            std::string filename;
            int filter_index;
        };

        struct FileFilter
        {
            std::wstring name;
            std::vector<std::wstring> file_types;
        };

        virtual ~IDialogs() = 0;
        /// <summary>
        /// Show a modal message box.
        /// </summary>
        /// <param name="window">The window that owns the message box.</param>
        /// <param name="message">The message of the message box.</param>
        /// <param name="title">The title of the message box.</param>
        /// <param name="buttons">The buttons to show on the message box.</param>
        /// <returns>True if the positive result was chosen.</returns>
        virtual bool message_box(const Window& window, const std::wstring& message, const std::wstring& title, Buttons buttons) const = 0;
        /// <summary>
        /// Prompt the user to select a file to open.
        /// </summary>
        /// <param name="title">The title of the dialog box.</param>
        /// <param name="filters">The filters to use.</param>
        /// <param name="flags">The flags for the dialog.</param>
        /// <returns>The filename if one was selected or an empty optional.</returns>
        virtual std::optional<std::string> open_file(const std::wstring& title, const std::vector<FileFilter>& filters, uint32_t flags) const = 0;
        /// <summary>
        /// Prompt the user to select a file to save.
        /// </summary>
        /// <param name="title">The title of the dialog box.</param>
        /// <param name="filters">The file filters to apply.</param>
        /// <returns>The save file result if one was selected or an empty optional.</returns>
        virtual std::optional<SaveFileResult> save_file(const std::wstring& title, const std::vector<FileFilter>& filters) const = 0;
    };
}
