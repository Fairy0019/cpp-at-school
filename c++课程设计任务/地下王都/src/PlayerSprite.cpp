#include "PlayerSprite.h"

#include "Item.h"
#include "Player.h"
#include "UI.h"

namespace {

constexpr int kSpriteWidth = 14;

UI::Color armorColor(const Equipment* armor) {
    if (armor == nullptr) {
        return UI::Color::BrightCyan;
    }
    // 皮甲一档偏黄，铁甲一档偏白。
    return armor->getDefenseBonus() >= 3 ? UI::Color::White : UI::Color::Yellow;
}

UI::Color weaponColor(const Equipment* weapon) {
    if (weapon == nullptr) {
        return UI::Color::Gray;
    }
    // 短剑偏绿，铁剑偏紫。
    return weapon->getAttackBonus() >= 3 ? UI::Color::BrightMagenta : UI::Color::BrightGreen;
}

std::string finishLine(const std::string& line) {
    return UI::fit(line, kSpriteWidth);
}

}  // namespace

namespace PlayerSprite {

std::vector<std::string> lines(const Equipment* weapon, const Equipment* armor) {
    const UI::Color armorTint = armorColor(armor);
    const UI::Color weaponTint = weaponColor(weapon);
    const UI::Color limbTint = UI::Color::Gray;

    //  头：人
    //  身体：旅（两侧手臂固定灰色）
    //  右手：没武器写「拳头」，装备后写武器名
    //  腿：斜线组成的双腿
    const std::string headLine =
        "   " + UI::paint("人", armorTint);

    std::string weaponLabel = weapon ? weapon->getName() : "拳头";
    if (UI::displayWidth(weaponLabel) > 4) {
        weaponLabel = UI::fit(weaponLabel, 4);
    }
    const std::string weaponField =
        UI::padRight(UI::paint(weaponLabel, weaponTint), 4);

    const std::string bodyLine =
        "  " + UI::paint("/", limbTint) +
        UI::paint("旅", armorTint) +
        UI::paint("\\", limbTint) +
        "    " + weaponField;

    const std::string coreLine =
        "   " + UI::paint("|", limbTint);

    const std::string legLine =
        "  " + UI::paint("/ \\", limbTint);

    return {
        finishLine(headLine),
        finishLine(bodyLine),
        finishLine(coreLine),
        finishLine(legLine)
    };
}

std::vector<std::string> lines(const Player& player) {
    return lines(player.getWeapon(), player.getArmor());
}

std::vector<std::string> defaultLines() {
    return lines(nullptr, nullptr);
}

}  // namespace PlayerSprite
