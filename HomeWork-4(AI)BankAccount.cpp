#include <cmath>
#include <iostream>
#include <stdexcept>

class BankAccount {
private:
    int accountNumber;
    double balance;
    static double interestRate;

public:
    BankAccount(int accountNumber, double balance)
        : accountNumber(accountNumber), balance(balance) {
        if (this->accountNumber <= 0) {
            throw std::invalid_argument("Account number must be positive.");
        }
        if (!std::isfinite(this->balance) || this->balance < 0.0) {
            throw std::invalid_argument("Initial balance must be finite and non-negative.");
        }
    }

    // The parameter intentionally shadows the member to demonstrate this->.
    bool deposit(double balance) {
        if (!std::isfinite(balance) || balance <= 0.0) {
            std::cout << "Deposit amount must be positive.\n";
            return false;
        }
        this->balance += balance;
        return true;
    }

    // Here the parameter is the requested withdrawal amount.
    bool withdraw(double balance) {
        if (!std::isfinite(balance) || balance <= 0.0) {
            std::cout << "Withdrawal amount must be positive.\n";
            return false;
        }
        if (balance > this->balance) {
            std::cout << "Insufficient balance; withdrawal was not made.\n";
            return false;
        }
        this->balance -= balance;
        return true;
    }

    void display() const {
        std::cout << "Account number: " << accountNumber
                  << ", balance: " << balance
                  << ", shared interest rate: " << interestRate << "%\n";
    }

    static bool setInterestRate(double rate) {
        if (!std::isfinite(rate) || rate < 0.0) {
            std::cout << "Interest rate must be finite and non-negative.\n";
            return false;
        }
        interestRate = rate;
        return true;
    }

    static double getInterestRate() {
        return interestRate;
    }
};

double BankAccount::interestRate = 0.0;

int main() {
    BankAccount::setInterestRate(3.5);

    BankAccount account1(1001, 5000.0);
    BankAccount account2(1002, 3000.0);

    account1.deposit(1500.0);
    account2.withdraw(500.0);

    account1.display();
    account2.display();
    std::cout << "Current shared interest rate: "
              << BankAccount::getInterestRate() << "%\n";
    return 0;
}
