#pragma once

#include <string>

// 一次普通攻击的结算结果，供界面层生成战斗日志。
struct AttackResult {
    int damage = 0;
    bool critical = false;
    bool reducedByDefense = false;
};

// 角色抽象基类：玩家与所有怪物的共同父类。
// takeTurn 为纯虚函数，派生类决定自己的回合行为并返回战斗日志。
class Character {
public:
    // 基础暴击率 30%，暴击伤害为普通伤害的 150%。
    static constexpr double kCriticalChance = 0.30;
    static constexpr double kCriticalMultiplier = 1.5;

    Character(std::string name, int maxHp, int maxMp, int attack, int defense);
    virtual ~Character() = default;

    const std::string& getName() const;
    int getHp() const;
    int getMaxHp() const;
    int getMp() const;
    int getMaxMp() const;
    virtual int getAttack() const;
    virtual int getDefense() const;

    bool isAlive() const;
    bool isDefending() const;
    void setDefending(bool defending);

    // 纯虚函数：玩家和不同怪物用各自的方式完成一个回合。
    virtual std::string takeTurn(Character& target) = 0;

    // 普通攻击：随机浮动 + 暴击 + 防御减伤。
    AttackResult attack(Character& target);

    // 受到伤害，返回实际扣血量；HP 最低为 0。
    int takeDamage(int rawDamage);

    // 回复 HP / MP，返回实际回复量。
    int heal(int amount);
    int restoreMp(int amount);

    // 消耗 MP；不足时不扣除并返回 false。
    bool spendMp(int amount);

    // 将 HP、MP 恢复到上限。
    void restoreAll();

protected:
    // 升级时提升属性上限（当前值由 restoreAll 补满）。
    void growStats(int maxHpUp, int maxMpUp, int attackUp, int defenseUp);

    // 读档时恢复当前 HP / MP（自动裁剪到合法范围）。
    void setVitals(int hp, int mp);

    // 根据攻击结果生成一行战斗日志。
    std::string buildAttackMessage(const Character& target,
                                   const AttackResult& result,
                                   const std::string& verb) const;

private:
    std::string name_;
    int hp_;
    int maxHp_;
    int mp_;
    int maxMp_;
    int attack_;
    int defense_;
    bool defending_;
};
