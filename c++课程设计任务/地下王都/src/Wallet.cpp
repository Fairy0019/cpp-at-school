#include "Wallet.h"

#include <stdexcept>

Wallet::Wallet(int gold)
    : gold_(gold < 0 ? 0 : gold) {
}

int Wallet::get() const {
    return gold_;
}

bool Wallet::canAfford(int amount) const {
    return amount >= 0 && gold_ >= amount;
}

void Wallet::add(int amount) {
    if (amount < 0) {
        throw std::invalid_argument("不能增加负数金币");
    }
    gold_ += amount;
}

Wallet& Wallet::operator+=(int amount) {
    add(amount);
    return *this;
}

Wallet& Wallet::operator-=(int amount) {
    if (amount < 0) {
        throw std::invalid_argument("不能扣除负数金币");
    }
    if (amount > gold_) {
        throw std::runtime_error("金币不足");
    }
    gold_ -= amount;
    return *this;
}

Wallet Wallet::operator+(int amount) const {
    Wallet result(*this);
    result += amount;
    return result;
}

Wallet Wallet::operator-(int amount) const {
    Wallet result(*this);
    result -= amount;
    return result;
}
