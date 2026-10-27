#pragma once

#include "Monster.h"

class RuinsRat : public Monster {
public:
    RuinsRat();

    const std::vector<std::string>& getArt() const override;
    std::string getEncounterText() const override;
    MonsterColor getColor() const override;
};
