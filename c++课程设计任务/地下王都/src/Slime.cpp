#include "Slime.h"

#include "Random.h"
#include "Sprites.h"

Slime::Slime(SlimeElement element)
    : Monster(element == SlimeElement::Water ? "水史莱姆" : "火史莱姆",
              element == SlimeElement::Water ? 30 : 24,
              element == SlimeElement::Water ? 6 : 9,
              element == SlimeElement::Water ? 4 : 1,
              element == SlimeElement::Water ? 14 : 16,
              element == SlimeElement::Water ? 8 : 12),
      element_(element) {
}

std::string Slime::takeTurn(Character& target) {
    const AttackResult result = attack(target);
    return buildAttackMessage(
        target, result,
        element_ == SlimeElement::Water ? "涌动撞击" : "燃烧着撞向");
}

const std::vector<std::string>& Slime::getArt() const {
    return element_ == SlimeElement::Water ? Sprites::waterSlime() : Sprites::fireSlime();
}

std::string Slime::getEncounterText() const {
    if (element_ == SlimeElement::Water) {
        return "一团水蓝色的黏液从地面鼓起，缓缓涌了过来。";
    }
    return "一团灼热的黏液在地面翻滚，空气被烤得扭曲。";
}

MonsterColor Slime::getColor() const {
    return element_ == SlimeElement::Water ? MonsterColor::Blue : MonsterColor::Red;
}

std::unique_ptr<Item> Slime::rollDrop() const {
    if (Random::chance(0.5)) {
        return std::make_unique<Material>("史莱姆粘液", 8);
    }
    return nullptr;
}

SlimeElement Slime::getElement() const {
    return element_;
}
