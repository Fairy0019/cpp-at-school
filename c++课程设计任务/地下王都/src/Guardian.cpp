#include "Guardian.h"

#include "Item.h"
#include "Sprites.h"

#include <algorithm>
#include <string>

Guardian::Guardian(int playerLevel)
    : Monster("封存骑士",
              70 + std::max(1, playerLevel) * 8,
              8 + std::max(1, playerLevel),
              3 + std::max(1, playerLevel) / 3,
              120,
              80),
      turnCount_(0) {
}

std::string Guardian::takeTurn(Character& target) {
    ++turnCount_;

    // 每三次行动，骑士会放弃普通挥砍，改用一次穿透护甲的封存冲击。
    if (turnCount_ % 3 == 0) {
        const bool reduced = target.isDefending();
        const int rawDamage = std::max(1, getAttack() - target.getDefense() / 2 + 2);
        const int damage = target.takeDamage(rawDamage);

        std::string message = "封存骑士抬起手臂，封存冲击落在 " + target.getName() +
                              " 身上，造成 " + std::to_string(damage) + " 点伤害";
        if (reduced) {
            message += "（防御减伤）";
        }
        message += "。";
        return message;
    }

    const AttackResult result = attack(target);
    return buildAttackMessage(target, result, "挥下旧剑");
}

const std::vector<std::string>& Guardian::getArt() const {
    return Sprites::guardian();
}

std::string Guardian::getEncounterText() const {
    return "第四骑士·封存从回廊尽头驶出。它没有马，只有一副仍在执行旧命令的甲胄。";
}

MonsterColor Guardian::getColor() const {
    return MonsterColor::Magenta;
}

std::string Guardian::getBattleTitle() const {
    return "核心回廊 | 守护者战";
}

bool Guardian::isBoss() const {
    return true;
}

std::unique_ptr<Item> Guardian::rollDrop() const {
    // 守护者核心：旧王朝封存单元的残片，也是它仍然站在这里的理由。
    return std::make_unique<Accessory>("封存核心", 120, 2, 3);
}
