/// @file A window with two panels where the right panel can be collapsed.

#pragma once

#include <memory>
#include <string>
#include <trview.common/MessageHandler.h>
#include <trview.common/TokenStore.h>
#include <trview.ui.render/IRenderer.h>
#include <trview.graphics/IDeviceWindow.h>
#include <trview.ui/IInput.h>
#include <trview.ui/Window.h>
#include <trview.app/Windows/WindowResizer.h>

namespace trview
{
    namespace ui
    {
        class Control;
        class Button;
    }

    /// A window with two panels where the right panel can be collapsed.
    class CollapsiblePanel : public MessageHandler
    {
    public:
        struct Names
        {
            static const std::string left_panel;
            static const std::string right_panel;
        };

        /// <summary>
        /// Create a collapsible panel window as a child of the specified window.
        /// </summary>
        /// <param name="device_window_source">The device window source function.</param>
        /// <param name="ui_renderer">The renderer to use.</param>
        /// <param name="parent">The parent window.</param>
        /// <param name="window_class">Window class name</param>
        /// <param name="title">Window title</param>
        /// <param name="input_source">Function to call to create an IInput.</param>
        /// <param name="size">Window size</param>
        /// <param name="ui">UI for the window.</param>
        CollapsiblePanel(const graphics::IDeviceWindow::Source& device_window_source, std::unique_ptr<ui::render::IRenderer> ui_renderer, const Window& parent,
            const std::wstring& window_class, const std::wstring& title, const ui::IInput::Source& input_source, const Size& size, std::unique_ptr<ui::Control> ui);

        virtual ~CollapsiblePanel() = default;

        /// Handles a window message.
        /// @param message The message that was received.
        /// @param wParam The WPARAM for the message.
        /// @param lParam The LPARAM for the message.
        virtual std::optional<int> process_message(UINT message, WPARAM wParam, LPARAM lParam) override;

        /// Render the window.
        /// @param device The device to render with.
        /// @param vsync Whether to use vsync or not.
        void render(bool vsync);

        /// Get the root control for the window.
        /// @returns The root control.
        ui::Control* root_control() const;

        /// Event raised when the window is closed.
        Event<> on_window_closed;

        Event<Size> on_size_changed;
    protected:
        /// <summary>
        /// Set the expander button. This should be called by derived classes when they have the button to bind.
        /// </summary>
        /// <param name="button">The button to use as the expander button.</param>
        void set_expander(ui::Button* button);

        /// Set whether the window can be made taller.
        /// @param Whether the window can be made taller.
        void set_allow_increase_height(bool value);

        void set_minimum_height(uint32_t height);

        TokenStore   _token_store;
        ui::Control* _left_panel;
        ui::Control* _right_panel;
        std::unique_ptr<ui::Control> _ui;
        std::unique_ptr<ui::IInput> _input;
        std::unique_ptr<graphics::IDeviceWindow> _device_window;
    private:
        void toggle_expand();
        void register_change_detection(ui::Control* control);

        Window _parent;
        std::unique_ptr<ui::render::IRenderer>   _ui_renderer;
        WindowResizer   _window_resizer;
        ui::Button* _expander;
        Size        _initial_size;
        bool        _allow_increase_height{ true };
        bool        _expanded{ true };
        bool        _ui_changed{ true };
    };
}