#include "Mimic.h"

#include "Item.h"
#include "Sprites.h"

#include <algorithm>
#include <string>

Mimic::Mimic(int playerLevel)
    : Monster("宝箱怪",
              35 + std::max(1, playerLevel) * 5,
              7 + std::max(1, playerLevel),
              2 + std::max(1, playerLevel) / 3,
              35,
              25),
      turnCount_(0) {
}

std::string Mimic::takeTurn(Character& target) {
    ++turnCount_;

    // 每三次行动，箱盖会猛地合上，造成一次更重的伤害。
    if (turnCount_ % 3 == 0) {
        const bool reduced = target.isDefending();
        const int rawDamage = std::max(1, getAttack() + 2 - target.getDefense() / 2);
        const int damage = target.takeDamage(rawDamage);

        std::string message = "宝箱怪猛地合上箱盖，咬住 " + target.getName() +
                              "，造成 " + std::to_string(damage) + " 点伤害";
        if (reduced) {
            message += "（防御减伤）";
        }
        message += "。";
        return message;
    }

    const AttackResult result = attack(target);
    return buildAttackMessage(target, result, "扑向");
}

const std::vector<std::string>& Mimic::getArt() const {
    return Sprites::mimic();
}

std::string Mimic::getEncounterText() const {
    return "宝箱的盖子突然掀开，里面没有金币，只有一排牙。";
}

MonsterColor Mimic::getColor() const {
    return MonsterColor::Yellow;
}

std::string Mimic::getBattleTitle() const {
    return "奖励房间 | 宝箱怪";
}

std::unique_ptr<Item> Mimic::rollDrop() const {
    return std::make_unique<Material>("宝箱碎片", 12);
}
