#include "Monster.h"

#include <utility>

Monster::Monster(std::string name, int maxHp, int attack, int defense,
                 int expReward, int goldReward)
    : Character(std::move(name), maxHp, 0, attack, defense),
      expReward_(expReward),
      goldReward_(goldReward) {
}

std::string Monster::takeTurn(Character& target) {
    const AttackResult result = attack(target);
    return buildAttackMessage(target, result, "扑向");
}

int Monster::getExpReward() const {
    return expReward_;
}

int Monster::getGoldReward() const {
    return goldReward_;
}

std::string Monster::getBattleTitle() const {
    return "地下遗迹 | 遭遇战";
}

bool Monster::isBoss() const {
    return false;
}

std::unique_ptr<Item> Monster::rollDrop() const {
    return nullptr;
}
