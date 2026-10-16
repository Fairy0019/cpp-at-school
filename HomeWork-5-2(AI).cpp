#include <iostream>
#include <cstdlib>
using namespace std;

class Point
{
private:
    int x, y;

public:
    Point(int x = 0, int y = 0) : x(x), y(y) {}

    friend class Rectangle;                // Rectangle 是 Point 的友元类
    friend void showPoint(const Point &p); // 友元函数
};

// 友元函数：可以访问 Point 的私有成员 x 和 y
void showPoint(const Point &p)
{
    cout << "Point(" << p.x << ", " << p.y << ")";
}

class Circle
{
private:
    Point center; // 圆心
    int radius;   // 半径

public:
    Circle(int x, int y, int r) : center(x, y), radius(r) {}

    void show() const
    {
        cout << "Circle: center = ";
        showPoint(center); // 通过友元函数访问 Point 私有成员
        cout << ", radius = " << radius << endl;
    }
};

class Rectangle
{
private:
    Point topLeft;     // 左上角
    Point bottomRight; // 右下角
    int width;
    int height;

public:
    Rectangle(int x1, int y1, int x2, int y2)
        : topLeft(x1, y1), bottomRight(x2, y2),
          width(abs(x2 - x1)), height(abs(y2 - y1)) {}

    void show() const
    {
        // Rectangle 是 Point 的友元类，可以直接访问 Point 的私有成员
        cout << "Rectangle: topLeft = Point("
             << topLeft.x << ", " << topLeft.y << ")";
        cout << ", bottomRight = Point("
             << bottomRight.x << ", " << bottomRight.y << ")";
        cout << ", width = " << width
             << ", height = " << height << endl;
    }
};

int main()
{
    Point p(1, 2);
    showPoint(p);
    cout << endl;

    Circle c(3, 4, 5);
    c.show();

    Rectangle r(0, 0, 10, 5);
    r.show();

    return 0;
}