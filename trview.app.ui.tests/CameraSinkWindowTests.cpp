#include "pch.h"
#include "CameraSinkWindowTests.h"
#include <trview.app/Mocks/Elements/ICameraSink.h>
#include <trview.app/Mocks/Elements/IRoom.h>
#include <trview.app/Mocks/Elements/ITrigger.h>
#include <trview.app/Windows/CameraSink/CameraSinkWindow.h>
#include <trview.common/Mocks/Windows/IClipboard.h>
#include <trview.tests.common/Mocks.h>

using namespace testing;
using namespace trview;
using namespace trview::mocks;
using namespace trview::tests;

namespace
{
    namespace
    {
        auto register_test_module()
        {
            struct test_module
            {
                std::shared_ptr<IClipboard> clipboard{ mock_shared<MockClipboard>() };

                std::unique_ptr<CameraSinkWindow> build()
                {
                    return std::make_unique<CameraSinkWindow>(clipboard);
                }
            };

            return test_module{};
        }
    }

    struct CameraSinkWindowContext final
    {
        std::unique_ptr<CameraSinkWindow> ptr;
        std::vector<std::shared_ptr<MockCameraSink>> camera_sinks;
        std::vector<std::shared_ptr<MockTrigger>> triggers;

        void render()
        {
            if (ptr)
            {
                ptr->render();
            }
        }
    };
}

void register_camera_sink_window_tests(ImGuiTestEngine* engine)
{
    test<CameraSinkWindowContext>(engine, "Camera/Sink Window", "List Filtered When Room Set and Track Room Enabled",
        [](ImGuiTestContext* ctx) { ctx->GetVars<CameraSinkWindowContext>().render(); },
        [](ImGuiTestContext* ctx)
        {
            auto& context = ctx->GetVars<CameraSinkWindowContext>();
            context.ptr = register_test_module().build();

            auto room_78 = mock_shared<MockRoom>()->with_number(78);
            auto camera_sink1 = mock_shared<MockCameraSink>()->with_number(0)->with_room(mock_shared<MockRoom>()->with_number(56));
            auto camera_sink2 = mock_shared<MockCameraSink>()->with_number(1)->with_room(room_78);
            context.ptr->set_camera_sinks({ camera_sink1, camera_sink2 });
            context.ptr->set_current_room(room_78);
            context.camera_sinks = { camera_sink1, camera_sink2 };

            ctx->ItemClick("/**/Track##track");
            ctx->ItemCheck("/**/Room");

            IM_CHECK_EQ(ctx->ItemExists("/**/0##0"), false);;
            IM_CHECK_EQ(ctx->ItemExists("/**/1##1"), true);;
        });

    test<CameraSinkWindowContext>(engine, "Camera/Sink Window", "Selected Not Raised When Sync Off",
        [](ImGuiTestContext* ctx) { ctx->GetVars<CameraSinkWindowContext>().render(); },
        [](ImGuiTestContext* ctx)
        {
            auto& context = ctx->GetVars<CameraSinkWindowContext>();
            context.ptr = register_test_module().build();

            std::shared_ptr<ICameraSink> raised;
            auto token = context.ptr->on_camera_sink_selected += [&raised](const auto& camera_sink) { raised = camera_sink.lock(); };

            auto camera_sink1 = mock_shared<MockCameraSink>()->with_number(0);
            auto camera_sink2 = mock_shared<MockCameraSink>()->with_number(1);
            context.camera_sinks = { camera_sink1, camera_sink2 };
            context.ptr->set_camera_sinks({ camera_sink1, camera_sink2 });

            ctx->ItemUncheck("/**/Sync");
            ctx->ItemClick("/**/1##1");

            IM_CHECK_EQ(raised, nullptr);
        });

    test<CameraSinkWindowContext>(engine, "Camera/Sink Window", "Selected Raised",
        [](ImGuiTestContext* ctx) { ctx->GetVars<CameraSinkWindowContext>().render(); },
        [](ImGuiTestContext* ctx)
        {
            auto& context = ctx->GetVars<CameraSinkWindowContext>();
            context.ptr = register_test_module().build();

            std::shared_ptr<ICameraSink> raised;
            auto token = context.ptr->on_camera_sink_selected += [&raised](const auto& camera_sink) { raised = camera_sink.lock(); };

            auto camera_sink1 = mock_shared<MockCameraSink>()->with_number(0);
            auto camera_sink2 = mock_shared<MockCameraSink>()->with_number(1);
            context.camera_sinks = { camera_sink1, camera_sink2 };
            context.ptr->set_camera_sinks({ camera_sink1, camera_sink2 });

            ctx->ItemClick("/**/1##1");

            IM_CHECK_EQ(raised, camera_sink2);
        });

    test<CameraSinkWindowContext>(engine, "Camera/Sink Window", "Trigger Selected Raised",
        [](ImGuiTestContext* ctx) { ctx->GetVars<CameraSinkWindowContext>().render(); },
        [](ImGuiTestContext* ctx)
        {
            auto& context = ctx->GetVars<CameraSinkWindowContext>();
            context.ptr = register_test_module().build();

            std::shared_ptr<ITrigger> raised;
            auto token = context.ptr->on_trigger_selected += [&raised](const auto& trigger) { raised = trigger.lock(); };
            auto trigger1 = mock_shared<MockTrigger>();
            auto trigger2 = mock_shared<MockTrigger>();
            ON_CALL(*trigger2, number).WillByDefault(testing::Return(1));
            context.triggers = { trigger1, trigger2 };
            auto camera_sink = mock_shared<MockCameraSink>() ->with_number(0)->with_triggers({ trigger1, trigger2 });
            context.camera_sinks = { camera_sink };
            context.ptr->set_camera_sinks({ camera_sink });
            context.ptr->set_selected_camera_sink(camera_sink);

            ctx->ItemClick("/**/1");
            IM_CHECK_EQ(raised, trigger2);
        });

    test<CameraSinkWindowContext>(engine, "Camera/Sink Window", "Type Changed Raised",
        [](ImGuiTestContext* ctx) { ctx->GetVars<CameraSinkWindowContext>().render(); },
        [](ImGuiTestContext* ctx)
        {
            auto& context = ctx->GetVars<CameraSinkWindowContext>();
            context.ptr = register_test_module().build();

            bool raised = false;
            auto token = context.ptr->on_camera_sink_type_changed += [&raised]() { raised = true; };
            auto camera_sink = mock_shared<MockCameraSink>()->with_number(0)->with_type(ICameraSink::Type::Camera);
            context.camera_sinks = { camera_sink };
            context.ptr->set_camera_sinks({ camera_sink });
            context.ptr->set_selected_camera_sink(camera_sink);

            ctx->SetRef("/Camera\\/Sink 0\\/Details_8FE7677A");
            ctx->ComboClick("Type/Sink##type");

            IM_CHECK_EQ(raised, true);
        });

    test<CameraSinkWindowContext>(engine, "Camera/Sink Window", "Visibility Raised",
        [](ImGuiTestContext* ctx) { ctx->GetVars<CameraSinkWindowContext>().render(); },
        [](ImGuiTestContext* ctx)
        {
            auto& context = ctx->GetVars<CameraSinkWindowContext>();
            context.ptr = register_test_module().build();

            std::optional<std::tuple<std::shared_ptr<ICameraSink>, bool>> raised;
            auto token = context.ptr->on_camera_sink_visibility += [&raised](const auto& camera_sink, bool visible)
            { 
                auto cs = camera_sink.lock();
                ON_CALL(*std::static_pointer_cast<MockCameraSink>(cs), visible).WillByDefault(Return(visible));
                raised = { cs, visible }; 
            };

            auto camera_sink1 = mock_shared<MockCameraSink>()->with_number(0);
            auto camera_sink2 = mock_shared<MockCameraSink>()->with_number(1);
            ON_CALL(*camera_sink2, visible).WillByDefault(testing::Return(true));
            context.camera_sinks = { camera_sink1, camera_sink2 };
            context.ptr->set_camera_sinks({ camera_sink1, camera_sink2 });

            ctx->ItemClick("/**/##hide-1");

            IM_CHECK_EQ(std::get<0>(raised.value()), camera_sink2);
            IM_CHECK_EQ(std::get<1>(raised.value()), false);
        });
}
