#include "PlayerSprite.h"

#include "Item.h"
#include "Player.h"
#include "UI.h"

namespace {

constexpr int kSpriteWidth = 10;

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

std::string firstUtf8Char(const std::string& text) {
    if (text.empty()) {
        return std::string();
    }
    const unsigned char first = static_cast<unsigned char>(text[0]);
    std::size_t length = 1;
    if ((first & 0xE0) == 0xC0) {
        length = 2;
    } else if ((first & 0xF0) == 0xE0) {
        length = 3;
    } else if ((first & 0xF8) == 0xF0) {
        length = 4;
    }
    if (length > text.size()) {
        length = text.size();
    }
    return text.substr(0, length);
}

std::string lastUtf8Char(const std::string& text) {
    if (text.empty()) {
        return std::string();
    }
    std::size_t index = text.size() - 1;
    while (index > 0 &&
           (static_cast<unsigned char>(text[index]) & 0xC0) == 0x80) {
        --index;
    }
    return text.substr(index);
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

    const std::string weaponName = weapon ? weapon->getName() : "拳头";
    std::string weaponTop;
    std::string weaponBottom;
    if (weapon == nullptr) {
        // 拳头按正常顺序读：拳在上，头在下。
        weaponTop = "拳";
        weaponBottom = "头";
    } else {
        // 武器名竖排：上面是「剑」，下面是「短 / 铁」这类修饰字。
        weaponTop = lastUtf8Char(weaponName);
        weaponBottom = firstUtf8Char(weaponName);
    }

    //  头：人
    //  身体：旅
    //  右侧武器名竖着拆成两行：上面是后一个字，下面是前一个字
    //  四肢：固定灰色
    const std::string headLine =
        "   " + UI::paint("人", armorTint);

    const std::string bodyLine =
        "  " + UI::paint("/", limbTint) +
        UI::paint("旅", armorTint) +
        UI::paint("\\", limbTint) +
        "  " + UI::paint(weaponTop, weaponTint);

    const std::string coreLine =
        "   " + UI::paint("|", limbTint) +
        "  " + UI::paint(weaponBottom, weaponTint);

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
