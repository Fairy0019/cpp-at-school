#include <iostream>
using namespace std;

int main() {
    int a = 0, b = 0;

    cout << "Enter two numbers: ";
    if (!(cin >> a >> b)) {
        cerr << "Please enter valid integers." << endl;
        return 1;
    }

    cout << "The numbers are: " << a << " and " << b << endl;
    if (a == b) {
        std::cout << "yes";
    } else {
        std::cerr << "no";
    }
    return 0;
}