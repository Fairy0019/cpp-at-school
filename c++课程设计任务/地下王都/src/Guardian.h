#pragma once

#include "Monster.h"

// 封存骑士：旧王朝四守护单元之一，守在核心门前。
// 它不主动制造灾难，只执行数百年前留下的封存命令。
class Guardian : public Monster {
public:
    explicit Guardian(int playerLevel);

    std::string takeTurn(Character& target) override;
    const std::vector<std::string>& getArt() const override;
    std::string getEncounterText() const override;
    MonsterColor getColor() const override;
    std::string getBattleTitle() const override;
    bool isBoss() const override;
    std::unique_ptr<Item> rollDrop() const override;

private:
    int turnCount_;
};
