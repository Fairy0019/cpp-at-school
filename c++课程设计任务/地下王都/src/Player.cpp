#include "Player.h"

#include "Random.h"

#include <algorithm>
#include <utility>

namespace {

constexpr int kBaseExperienceToLevel = 30;  // 1 级升 2 级所需经验
constexpr int kExperienceGrowth = 20;       // 每升一级，阈值增加 20

}  // namespace

Player::Player(std::string name)
    : Character(std::move(name), 30, 10, 8, 3),
      level_(1),
      experience_(0),
      experienceToNextLevel_(kBaseExperienceToLevel),
      ultimateCharge_(0),
      gender_("不明"),
      species_("人类"),
      wallet_(20) {
}

std::string Player::takeTurn(Character& target) {
    const AttackResult result = attack(target);
    return buildAttackMessage(target, result, "挥剑攻向");
}

int Player::getAttack() const {
    int bonus = weapon_ ? weapon_->getAttackBonus() : 0;
    bonus += accessory_ ? accessory_->getAttackBonus() : 0;
    return Character::getAttack() + bonus;
}

int Player::getDefense() const {
    int bonus = armor_ ? armor_->getDefenseBonus() : 0;
    bonus += accessory_ ? accessory_->getDefenseBonus() : 0;
    return Character::getDefense() + bonus;
}

int Player::gainExperience(int amount) {
    if (amount <= 0) {
        return 0;
    }

    experience_ += amount;
    int levelsGained = 0;
    while (experience_ >= experienceToNextLevel_) {
        experience_ -= experienceToNextLevel_;
        levelUp();
        experienceToNextLevel_ = kBaseExperienceToLevel + (level_ - 1) * kExperienceGrowth;
        ++levelsGained;
    }
    return levelsGained;
}

void Player::gainGold(int amount) {
    if (amount > 0) {
        wallet_ += amount;
    }
}

bool Player::spendGold(int amount) {
    if (amount < 0 || !wallet_.canAfford(amount)) {
        return false;
    }
    wallet_ -= amount;
    return true;
}

int Player::getLevel() const {
    return level_;
}

int Player::getExperience() const {
    return experience_;
}

int Player::getExperienceToNextLevel() const {
    return experienceToNextLevel_;
}

int Player::getGold() const {
    return wallet_.get();
}

const std::string& Player::getGender() const {
    return gender_;
}

const std::string& Player::getSpecies() const {
    return species_;
}

void Player::setIdentity(std::string gender, std::string species) {
    gender_ = gender.empty() ? "不明" : std::move(gender);
    species_ = species.empty() ? "人类" : std::move(species);
}

Inventory& Player::getInventory() {
    return inventory_;
}

const Inventory& Player::getInventory() const {
    return inventory_;
}

Player& Player::operator+(std::unique_ptr<Item> item) {
    inventory_ + std::move(item);
    return *this;
}

Wallet& Player::getWallet() {
    return wallet_;
}

const Wallet& Player::getWallet() const {
    return wallet_;
}

bool Player::castMagic(Character& target, std::string& message) {
    constexpr int kMpCost = 4;
    if (getMp() < kMpCost) {
        message = "MP 不足，魔法需要 4 点 MP。";
        return false;
    }

    spendMp(kMpCost);

    const int baseDamage = std::max(1, getAttack() + 2);
    int damage = std::max(1, static_cast<int>(baseDamage * Random::real(0.9, 1.1)));
    const bool critical = Random::chance(Character::kCriticalChance);
    if (critical) {
        damage = std::max(1, static_cast<int>(damage * Character::kCriticalMultiplier));
    }

    const int actualDamage = target.takeDamage(damage);
    message = "你施放「余烬咒」，" + target.getName() + " 受到 " +
              std::to_string(actualDamage) + " 点魔法伤害";
    if (critical) {
        message += "（暴击！）";
    }
    message += "。";

    if (ultimateCharge_ < kUltimateMax) {
        ++ultimateCharge_;
        message += "充能 +1。";
    } else {
        message += "充能已满。";
    }
    return true;
}

bool Player::ultimateAttack(Character& target, std::string& message) {
    if (ultimateCharge_ < kUltimateMax) {
        message = "大招尚未充能完成（" + std::to_string(ultimateCharge_) + "/" +
                  std::to_string(kUltimateMax) + "）。只有魔法攻击能充能。";
        return false;
    }

    ultimateCharge_ = 0;

    const int baseDamage = std::max(1, getAttack() * 3);
    const int damage = std::max(1, static_cast<int>(baseDamage * Random::real(0.95, 1.05)));
    const int actualDamage = target.takeDamage(damage);

    message = "你引爆全部充能，对 " + target.getName() + " 造成 " +
              std::to_string(actualDamage) + " 点伤害！";
    return true;
}

int Player::getUltimateCharge() const {
    return ultimateCharge_;
}

bool Player::equipFromInventory(int inventoryIndex, std::string& message) {
    Item* rawItem = inventory_.getItem(inventoryIndex);
    if (rawItem == nullptr) {
        message = "物品不存在。";
        return false;
    }

    Equipment* equipment = dynamic_cast<Equipment*>(rawItem);
    if (equipment == nullptr) {
        message = "这不是装备，无法穿戴。";
        return false;
    }

    const EquipmentSlot slot = equipment->getSlot();
    const std::string name = equipment->getName();
    const std::string description = equipment->getDescription();

    std::unique_ptr<Item> taken = inventory_.takeItem(inventoryIndex);
    if (!taken) {
        message = "物品不存在。";
        return false;
    }
    std::unique_ptr<Equipment> newEquipment(static_cast<Equipment*>(taken.release()));

    std::unique_ptr<Equipment>& targetSlot = equipmentSlot(slot);
    const bool swapped = (targetSlot != nullptr);
    if (swapped) {
        // 刚拿走一件装备，背包至少空出一格，旧装备一定能放回去。
        inventory_.add(std::move(targetSlot));
    }

    targetSlot = std::move(newEquipment);
    message = "装备了 " + name + "（" + description + "）。";
    if (swapped) {
        message += "旧装备已放回背包。";
    }
    return true;
}

bool Player::unequip(EquipmentSlot slot, std::string& message) {
    std::unique_ptr<Equipment>& targetSlot = equipmentSlot(slot);

    if (!targetSlot) {
        message = "该部位没有装备。";
        return false;
    }
    if (!inventory_.canAdd()) {
        message = "背包已满，无法卸下装备。";
        return false;
    }

    const std::string name = targetSlot->getName();
    inventory_.add(std::move(targetSlot));
    message = "卸下了 " + name + "。";
    return true;
}

const Equipment* Player::getWeapon() const {
    return weapon_.get();
}

const Equipment* Player::getArmor() const {
    return armor_.get();
}

const Equipment* Player::getAccessory() const {
    return accessory_.get();
}

std::unique_ptr<Equipment>& Player::equipmentSlot(EquipmentSlot slot) {
    switch (slot) {
        case EquipmentSlot::Weapon:
            return weapon_;
        case EquipmentSlot::Armor:
            return armor_;
        case EquipmentSlot::Accessory:
            return accessory_;
    }
    return accessory_;
}

void Player::restoreProgress(int level, int experience, int hp, int mp, int gold,
                             int ultimateCharge) {
    if (level < 1) {
        level = 1;
    }

    // 新建的 Player 处于 1 级基础属性，按等级逐级成长到目标等级。
    level_ = 1;
    while (level_ < level) {
        ++level_;
        growStats(6, 2, 2, 1);
    }

    experienceToNextLevel_ = kBaseExperienceToLevel + (level_ - 1) * kExperienceGrowth;
    experience_ = std::max(0, std::min(experience, experienceToNextLevel_ - 1));
    ultimateCharge_ = std::max(0, std::min(ultimateCharge, kUltimateMax));
    wallet_ = Wallet(std::max(0, gold));
    setVitals(std::max(1, hp), std::max(0, mp));
}

void Player::setEquipment(std::unique_ptr<Equipment> weapon,
                          std::unique_ptr<Equipment> armor,
                          std::unique_ptr<Equipment> accessory) {
    weapon_ = std::move(weapon);
    armor_ = std::move(armor);
    accessory_ = std::move(accessory);
}

void Player::levelUp() {
    ++level_;
    growStats(6, 2, 2, 1);
    restoreAll();
}
