#include "Goblin.h"

#include "Random.h"
#include "Sprites.h"

Goblin::Goblin()
    : Monster("哥布林", 32, 9, 2, 20, 18) {
    // 40% 概率带着抢来的装备。
    if (Random::chance(0.4)) {
        if (Random::chance(0.5)) {
            equipment_ = std::make_unique<Weapon>("锈短剑", 8, 1);
        } else {
            equipment_ = std::make_unique<Armor>("破皮甲", 8, 1);
        }
    }
}

std::string Goblin::takeTurn(Character& target) {
    const AttackResult result = attack(target);
    return buildAttackMessage(target, result, equipment_ ? "挥砍" : "扑向");
}

const std::vector<std::string>& Goblin::getArt() const {
    return Sprites::goblin();
}

std::string Goblin::getEncounterText() const {
    if (equipment_) {
        return "一个哥布林从碎石后钻出，手里攥着" + equipment_->getName() + "。";
    }
    return "一个哥布林从碎石后探出头，咧嘴笑着。";
}

MonsterColor Goblin::getColor() const {
    return MonsterColor::Green;
}

std::unique_ptr<Item> Goblin::rollDrop() const {
    // 只有带装备且命中 30% 掉落判定时才掉，整体概率约 12%。
    if (equipment_ && Random::chance(0.3)) {
        return equipment_->clone();
    }
    return nullptr;
}

int Goblin::getAttack() const {
    const int bonus = equipment_ ? equipment_->getAttackBonus() : 0;
    return Character::getAttack() + bonus;
}

int Goblin::getDefense() const {
    const int bonus = equipment_ ? equipment_->getDefenseBonus() : 0;
    return Character::getDefense() + bonus;
}
