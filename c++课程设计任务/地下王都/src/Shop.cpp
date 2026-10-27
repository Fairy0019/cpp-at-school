#include "Shop.h"

#include "Random.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

int scaledPrice(int basePrice, double scale) {
    return std::max(1, static_cast<int>(basePrice * scale + 0.5));
}

std::vector<std::unique_ptr<Item>> makeCatalog(double priceScale) {
    std::vector<std::unique_ptr<Item>> catalog;
    catalog.push_back(std::make_unique<Potion>(
        "红药水", scaledPrice(8, priceScale), Potion::Effect::RestoreHp, 15));
    catalog.push_back(std::make_unique<Potion>(
        "蓝药水", scaledPrice(10, priceScale), Potion::Effect::RestoreMp, 10));
    catalog.push_back(std::make_unique<Weapon>("短剑", scaledPrice(15, priceScale), 2));
    catalog.push_back(std::make_unique<Weapon>("铁剑", scaledPrice(35, priceScale), 5));
    catalog.push_back(std::make_unique<Armor>("皮甲", scaledPrice(20, priceScale), 2));
    catalog.push_back(std::make_unique<Armor>("铁甲", scaledPrice(40, priceScale), 5));
    catalog.push_back(std::make_unique<Accessory>("铜戒", scaledPrice(12, priceScale), 1, 0));
    catalog.push_back(std::make_unique<Accessory>("护符", scaledPrice(18, priceScale), 0, 2));
    return catalog;
}

}  // namespace

Shop::Shop()
    : Shop(8, 1.0) {
}

Shop::Shop(int itemCount, double priceScale) {
    std::vector<std::unique_ptr<Item>> catalog = makeCatalog(priceScale);
    const int wanted = std::max(1, std::min(itemCount, static_cast<int>(catalog.size())));

    if (wanted == static_cast<int>(catalog.size())) {
        items_ = std::move(catalog);
        return;
    }

    for (int index = 0; index < wanted; ++index) {
        const int pick = Random::integer(0, static_cast<int>(catalog.size()) - 1);
        items_.push_back(std::move(catalog[static_cast<std::size_t>(pick)]));
        catalog.erase(catalog.begin() + pick);
    }
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