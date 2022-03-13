#include <trview.app/UI/Console.h>
#include "TestImgui.h"

using namespace trview;
using namespace trview::tests;

TEST(Console, CommandEventRaised)
{
    Console console;
    console.set_visible(true);

    std::optional<std::wstring> raised;
    auto token = console.on_command += [&](auto value)
    {
        raised = value;
    };

    TestImgui imgui([&]() { console.render(); });
    imgui.click_element(imgui.id("Console").id(Console::Names::input));
    imgui.enter_text("Test command");
    imgui.press_key(ImGuiKey_Enter);
    imgui.reset();
    imgui.render();

    ASSERT_TRUE(raised.has_value());
    ASSERT_EQ(raised.value(), L"Test command");
    ASSERT_EQ(imgui.item_text(imgui.id("Console").id(Console::Names::input)), "");
}

TEST(Console, PrintAddsLine)
{
    Console console;
    console.set_visible(true);

    TestImgui imgui([&]() { console.render(); });
    ASSERT_EQ(imgui.item_text(imgui.id("Console").id(Console::Names::log)), "");

    console.print(L"Test log entry");
    imgui.render();
    ASSERT_EQ(imgui.item_text(imgui.id("Console").id(Console::Names::log)), "Test log entry");
}