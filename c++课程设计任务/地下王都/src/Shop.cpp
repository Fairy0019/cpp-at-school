#include "Shop.h"

#include <stdexcept>

Shop::Shop() {
    items_.push_back(std::make_unique<Potion>("红药水", 8, Potion::Effect::RestoreHp, 15));
    items_.push_back(std::make_unique<Potion>("蓝药水", 10, Potion::Effect::RestoreMp, 10));
    items_.push_back(std::make_unique<Weapon>("短剑", 15, 2));
    items_.push_back(std::make_unique<Weapon>("铁剑", 35, 5));
    items_.push_back(std::make_unique<Armor>("皮甲", 20, 2));
    items_.push_back(std::make_unique<Armor>("铁甲", 40, 5));
    items_.push_back(std::make_unique<Accessory>("铜戒", 12, 1, 0));
    items_.push_back(std::make_unique<Accessory>("护符", 18, 0, 2));
}

int Shop::size() const {
    return static_cast<int>(items_.size());
}

Item& Shop::operator[](int index) {
    if (index < 0 || index >= size()) {
        throw std::out_of_range("商品编号越界");
    }
    return *items_[static_cast<std::size_t>(index)];
}

const Item& Shop::operator[](int index) const {
    if (index < 0 || index >= size()) {
        throw std::out_of_range("商品编号越界");
    }
    return *items_[static_cast<std::size_t>(index)];
}

Item& Shop::operator[](const std::string& name) {
    for (const std::unique_ptr<Item>& item : items_) {
        if (item->getName() == name) {
            return *item;
        }
    }
    throw std::out_of_range("没有这件商品：" + name);
}

const Item& Shop::operator[](const std::string& name) const {
    for (const std::unique_ptr<Item>& item : items_) {
        if (item->getName() == name) {
            return *item;
        }
    }
    throw std::out_of_range("没有这件商品：" + name);
}
