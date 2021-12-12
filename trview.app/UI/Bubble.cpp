#include "Bubble.h"

using namespace trview::ui;
using namespace trview::graphics;

namespace trview
{
    const float Bubble::FadeTime{ 0.66666666666666666666666666666667f };
    const std::string Bubble::Names::Bubble{ "Bubble" };

    Bubble::Bubble(Control& control)
    {
        _label = control.add_child(std::make_unique<Label>(Size(40, 20), Colour::Grey, L"Copied", 8, TextAlignment::Centre, ParagraphAlignment::Centre));
        _label->set_name(Names::Bubble);
        _label->set_visible(false);
        _label->set_z(-1);

        _token_store += control.on_update += [&](float delta)
        {
            if (_label->visible())
            {
                const float speed = 1.0f / FadeTime;

                auto bg = _label->background_colour();
                bg.a = std::max(bg.a - delta * speed, 0.0f);
                auto fg = _label->text_colour();
                fg.a = std::max(fg.a - delta * speed, 0.0f);

                _label->set_text_colour(fg);
                _label->set_background_colour(bg);

                if (bg.a == 0.0f || fg.a == 0.0f)
                {
                    _label->set_visible(false);
                }
            }
        };
    }

    void Bubble::show(const Point& position)
    {
        _label->set_position(position - Point(_label->size().width * 0.5f, 0));
        _label->set_background_colour(Colour::Grey);
        _label->set_text_colour(Colour::White);
        _label->set_visible(true);
    }
}
