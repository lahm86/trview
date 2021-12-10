#pragma once

#include <external/boost/di.hpp>
#include "ViewerUI.h"
#include "SettingsWindow.h"
#include "ViewOptions.h"
#include "Bubble.h"
#include "ContextMenu.h"
#include "CameraControls.h"
#include <trview.common/Resources.h>
#include <trview.ui/ILoader.h>

namespace trview
{
    auto register_app_ui_module() noexcept
    {
        using namespace boost;
        return di::make_injector(
            di::bind<ISettingsWindow::Source>.to(
                [](const auto& injector) -> ISettingsWindow::Source
                {
                    return [&](ui::Control& parent)
                    {
                        return std::make_unique<SettingsWindow>(parent, injector.create<std::shared_ptr<ui::ILoader>>());
                    };
                }),
            di::bind<ICameraControls::Source>.to(
                [](const auto& injector) -> ICameraControls::Source
                {
                    return [&](ui::Control& parent)
                    {
                        return std::make_unique<CameraControls>(parent, injector.create<std::shared_ptr<ui::ILoader>>());
                    };
                }),
            di::bind<IViewOptions::Source>.to(
                [](const auto& injector) -> IViewOptions::Source
                {
                    return [&](auto&& parent)
                    {
                        return std::make_unique<ViewOptions>(parent, injector.create< std::shared_ptr<ui::ILoader>>());
                    };
                }),
            di::bind<IBubble::Source>.to(
                [](const auto&) -> IBubble::Source
                {
                    return [&](ui::Control& parent)
                    {
                        return std::make_unique<Bubble>(parent);
                    };
                }),
            di::bind<IContextMenu::Source>.to(
                [](const auto& injector) -> IContextMenu::Source
                {
                    return [&](ui::Control& parent)
                    {
                        return std::make_unique<ContextMenu>(parent, injector.create<std::shared_ptr<ui::ILoader>>());
                    };
                }),
            di::bind<IViewerUI>.to(
                [](const auto& injector) -> std::unique_ptr<IViewerUI>
                {
                    return std::make_unique<ViewerUI>(
                        injector.create<Window>(),
                        injector.create<std::shared_ptr<ITextureStorage>>(),
                        injector.create<std::shared_ptr<IShortcuts>>(),
                        injector.create<ui::IInput::Source>(),
                        injector.create<ui::render::IRenderer::Source>(),
                        injector.create<ui::render::IMapRenderer::Source>(),
                        injector.create<ISettingsWindow::Source>(),
                        injector.create<IViewOptions::Source>(),
                        injector.create<IContextMenu::Source>(),
                        injector.create<ICameraControls::Source>(),
                        injector.create<std::shared_ptr<ui::ILoader>>());
                })
        );
    }
}
