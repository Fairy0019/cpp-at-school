#include "RuinsRat.h"

#include "Sprites.h"

RuinsRat::RuinsRat()
    : Monster("遗迹鼠", 24, 7, 1, 12, 10) {
}

const std::vector<std::string>& RuinsRat::getArt() const {
    return Sprites::rat();
}

std::string RuinsRat::getEncounterText() const {
    return "一只遗迹鼠从碎石堆里窜出，挡住了去路。";
}

MonsterColor RuinsRat::getColor() const {
    return MonsterColor::Red;
}
