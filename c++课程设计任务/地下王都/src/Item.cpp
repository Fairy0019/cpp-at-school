#include "Item.h"

#include "Character.h"

#include <utility>

Item::Item(std::string name, int price)
    : name_(std::move(name)),
      price_(price) {
}

const std::string& Item::getName() const {
    return name_;
}

int Item::getPrice() const {
    return price_;
}

int Item::getSellPrice() const {
    return price_ / 2;
}

Potion::Potion(std::string name, int price, Effect effect, int amount)
    : Item(std::move(name), price),
      effect_(effect),
      amount_(amount) {
}

ItemKind Potion::getKind() const {
    return ItemKind::Consumable;
}

std::string Potion::getDescription() const {
    if (effect_ == Effect::RestoreHp) {
        return "回复 " + std::to_string(amount_) + " 点 HP";
    }
    return "回复 " + std::to_string(amount_) + " 点 MP";
}

bool Potion::use(Character& user, std::string& message) {
    if (effect_ == Effect::RestoreHp) {
        if (user.getHp() >= user.getMaxHp()) {
            message = "HP 已满，无需使用" + getName() + "。";
            return false;
        }
        const int healed = user.heal(amount_);
        message = "你使用" + getName() + "，回复了 " + std::to_string(healed) + " 点 HP。";
        return true;
    }

    if (user.getMp() >= user.getMaxMp()) {
        message = "MP 已满，无需使用" + getName() + "。";
        return false;
    }
    const int restored = user.restoreMp(amount_);
    message = "你使用" + getName() + "，回复了 " + std::to_string(restored) + " 点 MP。";
    return true;
}

std::unique_ptr<Item> Potion::clone() const {
    return std::make_unique<Potion>(getName(), getPrice(), effect_, amount_);
}

Potion::Effect Potion::getEffect() const {
    return effect_;
}

int Potion::getAmount() const {
    return amount_;
}

Equipment::Equipment(std::string name, int price, EquipmentSlot slot, int attackBonus, int defenseBonus)
    : Item(std::move(name), price),
      slot_(slot),
      attackBonus_(attackBonus),
      defenseBonus_(defenseBonus) {
}

ItemKind Equipment::getKind() const {
    switch (slot_) {
        case EquipmentSlot::Weapon:
            return ItemKind::Weapon;
        case EquipmentSlot::Armor:
            return ItemKind::Armor;
        case EquipmentSlot::Accessory:
            return ItemKind::Accessory;
    }
    return ItemKind::Weapon;
}

std::string Equipment::getDescription() const {
    std::string result;
    if (attackBonus_ != 0) {
        result = "攻击 +" + std::to_string(attackBonus_);
    }
    if (defenseBonus_ != 0) {
        if (!result.empty()) {
            result += " ";
        }
        result += "防御 +" + std::to_string(defenseBonus_);
    }
    if (result.empty()) {
        result = "无属性加成";
    }
    return result;
}

bool Equipment::use(Character&, std::string& message) {
    message = "请在背包中穿戴" + getName() + "。";
    return false;
}

EquipmentSlot Equipment::getSlot() const {
    return slot_;
}

int Equipment::getAttackBonus() const {
    return attackBonus_;
}

int Equipment::getDefenseBonus() const {
    return defenseBonus_;
}

Weapon::Weapon(std::string name, int price, int attackBonus)
    : Equipment(std::move(name), price, EquipmentSlot::Weapon, attackBonus, 0) {
}

std::unique_ptr<Item> Weapon::clone() const {
    return std::make_unique<Weapon>(getName(), getPrice(), getAttackBonus());
}

Armor::Armor(std::string name, int price, int defenseBonus)
    : Equipment(std::move(name), price, EquipmentSlot::Armor, 0, defenseBonus) {
}

std::unique_ptr<Item> Armor::clone() const {
    return std::make_unique<Armor>(getName(), getPrice(), getDefenseBonus());
}

Accessory::Accessory(std::string name, int price, int attackBonus, int defenseBonus)
    : Equipment(std::move(name), price, EquipmentSlot::Accessory, attackBonus, defenseBonus) {
}

std::unique_ptr<Item> Accessory::clone() const {
    return std::make_unique<Accessory>(getName(), getPrice(),
                                       getAttackBonus(), getDefenseBonus());
}

Material::Material(std::string name, int price)
    : Item(std::move(name), price) {
}

ItemKind Material::getKind() const {
    return ItemKind::Material;
}

std::string Material::getDescription() const {
    return "可以在商店出售的材料。";
}

bool Material::use(Character&, std::string& message) {
    message = "材料不能直接使用，可以卖给商人。";
    return false;
}

std::unique_ptr<Item> Material::clone() const {
    return std::make_unique<Material>(getName(), getPrice());
}

bool Inventory::canAdd() const {
    return static_cast<int>(items_.size()) < kCapacity;
}

bool Inventory::add(std::unique_ptr<Item> item) {
    if (!item || !canAdd()) {
        return false;
    }
    items_.push_back(std::move(item));
    return true;
}

Inventory& Inventory::operator+(std::unique_ptr<Item> item) {
    add(std::move(item));
    return *this;
}

int Inventory::size() const {
    return static_cast<int>(items_.size());
}

int Inventory::capacity() const {
    return kCapacity;
}

bool Inventory::empty() const {
    return items_.empty();
}

bool Inventory::full() const {
    return !canAdd();
}

const std::vector<std::unique_ptr<Item>>& Inventory::getItems() const {
    return items_;
}

Item* Inventory::getItem(int index) {
    if (index < 0 || index >= size()) {
        return nullptr;
    }
    return items_[static_cast<std::size_t>(index)].get();
}

const Item* Inventory::getItem(int index) const {
    if (index < 0 || index >= size()) {
        return nullptr;
    }
    return items_[static_cast<std::size_t>(index)].get();
}

std::unique_ptr<Item> Inventory::takeItem(int index) {
    if (index < 0 || index >= size()) {
        return nullptr;
    }
    std::unique_ptr<Item> item = std::move(items_[static_cast<std::size_t>(index)]);
    items_.erase(items_.begin() + index);
    return item;
}

bool Inventory::useItem(int index, Character& user, std::string& message) {
    Item* item = getItem(index);
    if (item == nullptr) {
        message = "物品不存在。";
        return false;
    }

    std::string useMessage;
    if (!item->use(user, useMessage)) {
        message = useMessage;
        return false;
    }

    message = useMessage;
    items_.erase(items_.begin() + index);
    return true;
}

int Inventory::countPotions() const {
    int count = 0;
    for (const std::unique_ptr<Item>& item : items_) {
        if (item->getKind() == ItemKind::Consumable) {
            ++count;
        }
    }
    return count;
}
