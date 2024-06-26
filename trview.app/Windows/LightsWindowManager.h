#pragma once

#include <trview.common/Windows/IShortcuts.h>
#include <trview.common/MessageHandler.h>
#include "ILightsWindow.h"
#include "ILightsWindowManager.h"
#include "WindowManager.h"

namespace trview
{
    class LightsWindowManager final : public ILightsWindowManager, WindowManager<ILightsWindow>, public MessageHandler
    {
    public:
        LightsWindowManager(const Window& window, const std::shared_ptr<IShortcuts>& shortcuts, const ILightsWindow::Source& lights_window_source);
        virtual ~LightsWindowManager() = default;
        virtual void set_lights(const std::vector<std::weak_ptr<ILight>>& lights) override;
        virtual void set_level_version(trlevel::LevelVersion version) override;
        virtual void render() override;
        virtual void update(float delta) override;
        virtual void set_selected_light(const std::weak_ptr<ILight>& light) override;
        virtual std::optional<int> process_message(UINT message, WPARAM wParam, LPARAM lParam) override;
        virtual std::weak_ptr<ILightsWindow> create_window() override;
        void set_room(const std::weak_ptr<IRoom>& room) override;
    private:
        std::vector<std::weak_ptr<ILight>> _lights;
        ILightsWindow::Source _lights_window_source;
        std::weak_ptr<ILight> _selected_light;
        trlevel::LevelVersion _level_version{ trlevel::LevelVersion::Tomb1 };
        std::weak_ptr<IRoom> _current_room;
    };
}
