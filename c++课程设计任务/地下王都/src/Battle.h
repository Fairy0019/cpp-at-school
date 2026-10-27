#pragma once

#include "Disaster.h"
#include "Monster.h"
#include "Player.h"

#include <string>
#include <vector>

enum class BattleResult {
    Victory,
    Defeat,
    Escaped
};

// 战斗流程控制器：负责回合推进、画面刷新、日志和胜负判定。
class Battle {
public:
    Battle(Player& player, int storyStage);

    BattleResult fight(Monster& monster);

private:
    void render(const Monster& monster, bool battleEnded = false) const;
    void renderInventory(const std::string& message) const;
    bool useItemFromInventory(std::string& message);
    void addLog(const std::string& message);

    Player& player_;
    std::vector<std::string> log_;
    int turn_;
    int storyStage_;
    int disasterCountdown_;
    bool hasDisaster_;
    DisasterType disasterType_;
    std::string disasterName_;
};
