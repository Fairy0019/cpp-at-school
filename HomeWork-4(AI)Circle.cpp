#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
constexpr double kPi = 3.14159265358979323846;

bool readRadius(const std::string& prompt, double& radius) {
    while (true) {
        std::cout << prompt;
        if (std::cin >> radius) {
            if (std::isfinite(radius) && radius >= 0.0) {
                return true;
            }
            std::cout << "Radius must be finite and non-negative.\n";
        } else {
            if (std::cin.eof()) {
                return false;
            }
            std::cout << "Invalid input; enter a number.\n";
            std::cin.clear();
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}
} // namespace

class Circle {
private:
    const double radius;

public:
    explicit Circle(double radius) : radius(radius) {
        if (!std::isfinite(radius) || radius < 0.0) {
            throw std::invalid_argument("Radius must be finite and non-negative.");
        }
    }

    double getArea() const {
        return kPi * radius * radius;
    }

    friend void compareArea(const Circle& c1, const Circle& c2);
};

void compareArea(const Circle& c1, const Circle& c2) {
    // The friend function reads private radii and compares the resulting areas.
    const double area1 = kPi * c1.radius * c1.radius;
    const double area2 = kPi * c2.radius * c2.radius;

    std::cout << std::fixed << std::setprecision(2)
              << "Circle 1 area: " << area1 << '\n'
              << "Circle 2 area: " << area2 << '\n';

    if (area1 == area2) {
        std::cout << "The two circles have equal areas.\n";
    } else if (area1 > area2) {
        std::cout << "Circle 1 has the larger area.\n";
    } else {
        std::cout << "Circle 2 has the larger area.\n";
    }
}

int main() {
    double radius1 = 0.0;
    double radius2 = 0.0;
    if (!readRadius("Enter the radius of circle 1: ", radius1) ||
        !readRadius("Enter the radius of circle 2: ", radius2)) {
        std::cerr << "Input ended before both radii were entered.\n";
        return 1;
    }

    try {
        const Circle circle1(radius1);
        const Circle circle2(radius2);
        compareArea(circle1, circle2);
    } catch (const std::invalid_argument& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
