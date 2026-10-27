#include "MonsterVisual.h"

namespace MonsterVisual {

UI::Color tint(MonsterColor color) {
    switch (color) {
        case MonsterColor::Red:
            return UI::Color::BrightRed;
        case MonsterColor::Green:
            return UI::Color::BrightGreen;
        case MonsterColor::Blue:
            return UI::Color::BrightBlue;
        case MonsterColor::Magenta:
            return UI::Color::BrightMagenta;
        case MonsterColor::Yellow:
            return UI::Color::BrightYellow;
        case MonsterColor::Cyan:
            return UI::Color::BrightCyan;
    }
    return UI::Color::BrightRed;
}

}  // namespace MonsterVisual
