#pragma once

#include <memory>
#include <string>
#include <vector>

class Character;

enum class ItemKind {
    Consumable,
    Weapon,
    Armor,
    Accessory,
    Material,
    Relic
};

enum class EquipmentSlot {
    Weapon,
    Armor,
    Accessory
};

// 物品抽象基类：消耗品、武器、防具都从这里派生。
class Item {
public:
    Item(std::string name, int price);
    virtual ~Item() = default;

    const std::string& getName() const;
    int getPrice() const;
    int getSellPrice() const;

    virtual ItemKind getKind() const = 0;
    virtual std::string getDescription() const = 0;
    virtual bool use(Character& user, std::string& message) = 0;

    // 商店购买时用来复制一份新物品，避免把商品原型移走。
    virtual std::unique_ptr<Item> clone() const = 0;

private:
    std::string name_;
    int price_;
};

// 药水：可以恢复 HP 或 MP，是战斗中唯一可以直接使用的物品类型。
class Potion : public Item {
public:
    enum class Effect {
        RestoreHp,
        RestoreMp
    };

    Potion(std::string name, int price, Effect effect, int amount);

    ItemKind getKind() const override;
    std::string getDescription() const override;
    bool use(Character& user, std::string& message) override;
    std::unique_ptr<Item> clone() const override;

    Effect getEffect() const;
    int getAmount() const;

private:
    Effect effect_;
    int amount_;
};

// 装备基类：记录部位和属性加成。
class Equipment : public Item {
public:
    Equipment(std::string name, int price, EquipmentSlot slot, int attackBonus, int defenseBonus);

    ItemKind getKind() const override;
    std::string getDescription() const override;
    bool use(Character& user, std::string& message) override;

    EquipmentSlot getSlot() const;
    int getAttackBonus() const;
    int getDefenseBonus() const;

private:
    EquipmentSlot slot_;
    int attackBonus_;
    int defenseBonus_;
};

class Weapon : public Equipment {
public:
    Weapon(std::string name, int price, int attackBonus);
    std::unique_ptr<Item> clone() const override;
};

class Armor : public Equipment {
public:
    Armor(std::string name, int price, int defenseBonus);
    std::unique_ptr<Item> clone() const override;
};

class Accessory : public Equipment {
public:
    Accessory(std::string name, int price, int attackBonus, int defenseBonus);
    std::unique_ptr<Item> clone() const override;
};

class Material : public Item {
public:
    Material(std::string name, int price);

    ItemKind getKind() const override;
    std::string getDescription() const override;
    bool use(Character& user, std::string& message) override;
    std::unique_ptr<Item> clone() const override;
};

// 古遗物：地下的关键道具，普通拾荒者不收，地面商店老板会解读。
class Relic : public Item {
public:
    Relic(std::string name, int price);

    ItemKind getKind() const override;
    std::string getDescription() const override;
    bool use(Character& user, std::string& message) override;
    std::unique_ptr<Item> clone() const override;
};

// 背包：独占持有物品，容量有限。
class Inventory {
public:
    static constexpr int kCapacity = 8;

    bool canAdd() const;
    bool add(std::unique_ptr<Item> item);

    // 任务书要求的 + 重载：把物品加入背包，返回自身以支持链式调用。
    Inventory& operator+(std::unique_ptr<Item> item);

    int size() const;
    int capacity() const;
    bool empty() const;
    bool full() const;

    const std::vector<std::unique_ptr<Item>>& getItems() const;
    Item* getItem(int index);
    const Item* getItem(int index) const;

    std::unique_ptr<Item> takeItem(int index);
    bool useItem(int index, Character& user, std::string& message);
    int countPotions() const;

private:
    std::vector<std::unique_ptr<Item>> items_;
};
