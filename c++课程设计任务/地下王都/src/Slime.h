#pragma once

#include "Monster.h"

enum class SlimeElement {
    Water,
    Fire
};

// 史莱姆：水 / 火两种元素，属性、颜色和文字形象不同。
class Slime : public Monster {
public:
    explicit Slime(SlimeElement element);

    std::string takeTurn(Character& target) override;
    const std::vector<std::string>& getArt() const override;
    std::string getEncounterText() const override;
    MonsterColor getColor() const override;
    std::unique_ptr<Item> rollDrop() const override;

    SlimeElement getElement() const;

private:
    SlimeElement element_;
};
