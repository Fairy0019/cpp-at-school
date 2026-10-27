#include "DisasterVisual.h"

namespace DisasterVisual {

UI::Color tint(DisasterType type) {
    switch (type) {
        case DisasterType::Snow:
            return UI::Color::BrightCyan;
        case DisasterType::Earthquake:
            return UI::Color::BrightYellow;
        case DisasterType::Fire:
            return UI::Color::BrightRed;
    }
    return UI::Color::BrightYellow;
}

}  // namespace DisasterVisual
