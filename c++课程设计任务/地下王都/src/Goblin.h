#pragma once

#include "Monster.h"

// 哥布林：有概率带着一件抢来的装备，击杀后有较低概率掉落该装备。
class Goblin : public Monster {
public:
    Goblin();

    std::string takeTurn(Character& target) override;
    const std::vector<std::string>& getArt() const override;
    std::string getEncounterText() const override;
    MonsterColor getColor() const override;
    std::unique_ptr<Item> rollDrop() const override;

    int getAttack() const override;
    int getDefense() const override;

private:
    std::unique_ptr<Equipment> equipment_;
};
