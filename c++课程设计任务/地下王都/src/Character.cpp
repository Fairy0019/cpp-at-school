#include "Character.h"

#include "Random.h"

#include <algorithm>
#include <utility>

Character::Character(std::string name, int maxHp, int maxMp, int attack, int defense)
    : name_(std::move(name)),
      hp_(maxHp),
      maxHp_(maxHp),
      mp_(maxMp),
      maxMp_(maxMp),
      attack_(attack),
      defense_(defense),
      defending_(false) {
}

const std::string& Character::getName() const {
    return name_;
}

int Character::getHp() const {
    return hp_;
}

int Character::getMaxHp() const {
    return maxHp_;
}

int Character::getMp() const {
    return mp_;
}

int Character::getMaxMp() const {
    return maxMp_;
}

int Character::getAttack() const {
    return attack_;
}

int Character::getDefense() const {
    return defense_;
}

bool Character::isAlive() const {
    return hp_ > 0;
}

bool Character::isDefending() const {
    return defending_;
}

void Character::setDefending(bool defending) {
    defending_ = defending;
}

AttackResult Character::attack(Character& target) {
    AttackResult result;
    result.reducedByDefense = target.isDefending();

    const int baseDamage = std::max(1, getAttack() - target.getDefense());
    result.critical = Random::chance(kCriticalChance);

    int damage = std::max(1, static_cast<int>(baseDamage * Random::real(0.85, 1.15)));
    if (result.critical) {
        damage = std::max(1, static_cast<int>(damage * kCriticalMultiplier));
    }

    result.damage = target.takeDamage(damage);
    return result;
}

int Character::takeDamage(int rawDamage) {
    if (rawDamage <= 0 || hp_ <= 0) {
        return 0;
    }

    int damage = rawDamage;
    if (defending_) {
        damage = std::max(1, damage / 2);
    }

    const int actualDamage = std::min(hp_, damage);
    hp_ -= actualDamage;
    return actualDamage;
}

int Character::heal(int amount) {
    if (amount <= 0) {
        return 0;
    }

    const int actualHeal = std::min(amount, maxHp_ - hp_);
    hp_ += actualHeal;
    return actualHeal;
}

int Character::restoreMp(int amount) {
    if (amount <= 0) {
        return 0;
    }

    const int actualRestore = std::min(amount, maxMp_ - mp_);
    mp_ += actualRestore;
    return actualRestore;
}

bool Character::spendMp(int amount) {
    if (amount < 0 || mp_ < amount) {
        return false;
    }
    mp_ -= amount;
    return true;
}

void Character::restoreAll() {
    hp_ = maxHp_;
    mp_ = maxMp_;
}

void Character::growStats(int maxHpUp, int maxMpUp, int attackUp, int defenseUp) {
    maxHp_ += maxHpUp;
    maxMp_ += maxMpUp;
    attack_ += attackUp;
    defense_ += defenseUp;
}

void Character::setVitals(int hp, int mp) {
    hp_ = std::max(0, std::min(hp, maxHp_));
    mp_ = std::max(0, std::min(mp, maxMp_));
}

std::string Character::buildAttackMessage(const Character& target,
                                          const AttackResult& result,
                                          const std::string& verb) const {
    std::string message = name_ + " " + verb + " " + target.getName() +
                          "，造成 " + std::to_string(result.damage) + " 点伤害";
    if (result.critical) {
        message += "（暴击！）";
    }
    if (result.reducedByDefense) {
        message += "（防御减伤）";
    }
    message += "。";
    return message;
}
