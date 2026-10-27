#pragma once

#include "Monster.h"

// 宝箱怪：看起来像奖励房间里的宝箱，打开后有概率跳出来。
class Mimic : public Monster {
public:
    explicit Mimic(int playerLevel);

    std::string takeTurn(Character& target) override;
    const std::vector<std::string>& getArt() const override;
    std::string getEncounterText() const override;
    MonsterColor getColor() const override;
    std::string getBattleTitle() const override;
    std::unique_ptr<Item> rollDrop() const override;

private:
    int turnCount_;
};
