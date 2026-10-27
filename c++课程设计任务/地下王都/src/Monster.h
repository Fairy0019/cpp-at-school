#pragma once

#include "Character.h"
#include "Item.h"

#include <memory>
#include <string>
#include <vector>

enum class MonsterColor {
    Red,
    Green,
    Blue,
    Magenta,
    Yellow,
    Cyan
};

// 怪物基类：不同怪物通过虚函数提供文字形象、元素颜色、遭遇文本和掉落物。
class Monster : public Character {
public:
    Monster(std::string name, int maxHp, int attack, int defense,
            int expReward, int goldReward);

    std::string takeTurn(Character& target) override;

    int getExpReward() const;
    int getGoldReward() const;

    virtual const std::vector<std::string>& getArt() const = 0;
    virtual std::string getEncounterText() const = 0;
    virtual MonsterColor getColor() const = 0;
    virtual std::string getBattleTitle() const;

    // 守护者战使用不同的战斗标题，并允许剧情层区分首领。
    virtual bool isBoss() const;

    // 默认不掉落；具体怪物重写。
    virtual std::unique_ptr<Item> rollDrop() const;

private:
    int expReward_;
    int goldReward_;
};
