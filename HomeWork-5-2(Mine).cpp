#include <iostream>

using namespace std;

/*ruct Point
{
    int x;
    int y;
    //相应的构造函数？
};

struct Circle
{
    int radius;
    Point center;   //显然，圆心
};

struct Rectangle
{
    int width;
    int height;
    Point topLeft;
    Point bottomRight;
};
*/

//Rectangle 类是Point 类的友元类，怎么申明？
/*
    尚未搞清楚这题到底要干嘛，

    使用友元函数可以访问 Point 类的私有成员？Circle?

    Rectangle 类访问 Point 类的私有成员？

    我是不是写成C中的结构体了？
    
    注：C++类的区别是默认访问权限不同，结构体默认public，类默认private。
*/

class Point
{
private:
    int x;
    int y;

public:
    Point(int x = 0, int y = 0) : x(x), y(y) {}
    friend class Rectangle;                // Rectangle 是 Point 的友元类
    friend void showPoint(const Point &p); // 友元函数
};

void showPoint(const Point &p)
{
    cout << "Point (" << p.x << "," << p.y << ")";
}

class Circle
{
private:
    Point center;    // 圆心
    int radius;     // 半径
public:
    Circle(const Point &c, int r) : center(c), radius(r) {}
    friend void showCircle(const Circle &c);      // 友元函数
};

void showCircle(const Circle &c)
{
    cout << "Circle: center = ";
    showPoint(c.center);    // 通过友元函数访问 Point 私有成员
    cout << ", radius = " << c.radius << endl;
}

class Rectangle
{
private:
    Point topLeft;          // 左上角
    Point bottomRight;      // 右下角
    int width;
    int height;
public:
    Rectangle(const Point &topLeft, const Point &bottomRight) : topLeft(topLeft), bottomRight(bottomRight) {
        width = bottomRight.x - topLeft.x;
        height = bottomRight.y - topLeft.y;
    }
    friend void showRectangle(const Rectangle &r);      // 友元函数
};

void showRectangle(const Rectangle &r)
{
    cout << "Rectangle: topLeft = ";
    showPoint(r.topLeft);        // 通过友元函数访问 Point 私有成员
    cout << ", bottomRight = ";
    showPoint(r.bottomRight);    // 通过友元函数访问 Point 私有成员
    cout << ", width = " << r.width << ", height = " << r.height << endl;
}

//然后呢？主函数没给任何要求，很难想象，一个程序的编写是不要

int main()
{
    Point p1(1, 2);
    Point p2(3, 4);
    Circle c(p1, 5);
    Rectangle r(p1, p2);

    showPoint(p1);
    cout << endl;
    showCircle(c);
    showRectangle(r);

    return 0;
}