#pragma once

#include <trview.common/Event.h>

namespace trview
{
    class Console final
    {
    public:
        struct Names
        {
            static const inline std::string log = "##Log";
            static const inline std::string input = "##input";
        };

        void render();
        bool visible() const;
        void print(const std::wstring& text);
        void set_visible(bool value);
        Event<std::wstring> on_command;
    private:
        std::string _text;
        bool _visible{ false };
    };
}
