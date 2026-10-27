#pragma once

#include "Item.h"

#include <memory>
#include <string>
#include <vector>

// 商店：保存商品原型，购买时通过 clone() 复制一份给玩家。
// operator[] 支持按编号或名称取商品，是课程设计要求的重载。
class Shop {
public:
    Shop();
    explicit Shop(int itemCount, double priceScale = 1.0);

    int size() const;

    Item& operator[](int index);
    const Item& operator[](int index) const;

    Item& operator[](const std::string& name);
    const Item& operator[](const std::string& name) const;

private:
    std::vector<std::unique_ptr<Item>> items_;
};
