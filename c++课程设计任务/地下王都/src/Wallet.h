#pragma once

// 钱包：把金币操作封装成自定义类型，便于重载 += / -= / + / -。
class Wallet {
public:
    explicit Wallet(int gold = 0);

    int get() const;
    bool canAfford(int amount) const;

    void add(int amount);

    Wallet& operator+=(int amount);
    Wallet& operator-=(int amount);

    Wallet operator+(int amount) const;
    Wallet operator-(int amount) const;

private:
    int gold_;
};
