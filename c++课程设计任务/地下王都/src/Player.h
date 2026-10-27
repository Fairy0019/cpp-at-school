#pragma once

#include "Character.h"
#include "Item.h"
#include "Wallet.h"

#include <memory>
#include <string>

// 玩家角色：等级、经验、钱包、背包和装备。
class Player : public Character {
public:
    static constexpr int kUltimateMax = 3;

    explicit Player(std::string name);

    std::string takeTurn(Character& target) override;

    // 在基础属性上叠加武器 / 防具加成。
    int getAttack() const override;
    int getDefense() const override;

    // 获得经验并根据阈值连续升级，返回提升的等级数。
    int gainExperience(int amount);
    void gainGold(int amount);
    bool spendGold(int amount);

    int getLevel() const;
    int getExperience() const;
    int getExperienceToNextLevel() const;
    int getGold() const;

    const std::string& getGender() const;
    const std::string& getSpecies() const;
    void setIdentity(std::string gender, std::string species);

    Inventory& getInventory();
    const Inventory& getInventory() const;

    // 人物类的 + 重载：把物品放进背包，返回自身以支持链式调用。
    Player& operator+(std::unique_ptr<Item> item);

    Wallet& getWallet();
    const Wallet& getWallet() const;

    // 魔法攻击：消耗 4 MP，无视防御，成功施放后大招充能 +1。
    bool castMagic(Character& target, std::string& message);

    // 大招：需要 3 格充能，消耗全部充能，造成 3 倍攻击的无视防御伤害。
    bool ultimateAttack(Character& target, std::string& message);

    int getUltimateCharge() const;

    // 从背包第 index 件物品中穿装备；失败时返回 false 并写入原因。
    bool equipFromInventory(int inventoryIndex, std::string& message);
    bool unequip(EquipmentSlot slot, std::string& message);

    const Equipment* getWeapon() const;
    const Equipment* getArmor() const;
    const Equipment* getAccessory() const;

    // 读档时恢复进度；只应在新建的 Player 上调用一次。
    void restoreProgress(int level, int experience, int hp, int mp, int gold,
                         int ultimateCharge = 0);
    void setEquipment(std::unique_ptr<Equipment> weapon,
                      std::unique_ptr<Equipment> armor,
                      std::unique_ptr<Equipment> accessory);

private:
    void levelUp();
    std::unique_ptr<Equipment>& equipmentSlot(EquipmentSlot slot);

    int level_;
    int experience_;
    int experienceToNextLevel_;
    int ultimateCharge_;
    std::string gender_;
    std::string species_;
    Wallet wallet_;
    Inventory inventory_;
    std::unique_ptr<Equipment> weapon_;
    std::unique_ptr<Equipment> armor_;
    std::unique_ptr<Equipment> accessory_;
};
