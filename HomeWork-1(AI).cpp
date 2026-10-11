#include <iostream>
#include <string>

// 使用命名空间 School 封装所有与学生相关的类型和函数
namespace School {

    // 定义学生结构体
    struct Student {
        std::string name; // 姓名
        int id;           // 学号
        float score;      // 成绩
    };

    // 输出学生信息，参数为常量指针
    void printStudent(const Student* stu) {
        if (stu == nullptr) {
            std::cout << "学生信息为空！" << std::endl;
            return;
        }

        std::cout << "学号:" << stu->id
                  << ", 姓名:" << stu->name
                  << ", 成绩:" << stu->score << std::endl;
    }

    // 更新学生成绩，要求新成绩在 0~100 之间
    void updateScore(Student* stu, float newScore) {
        if (stu == nullptr) {
            std::cout << "学生信息为空，无法更新！" << std::endl;
            return;
        }

        if (newScore < 0.0f || newScore > 100.0f) {
            std::cout << "错误：新成绩必须在 0~100 之间，更新失败！" << std::endl;
            return;
        }

        stu->score = newScore;
    }

} // namespace School

int main() {
    const int SIZE = 3;                 // 常量定义数组大小
    School::Student students[SIZE];     // 定义学生数组

    // 输入 3 名学生信息
    for (int i = 0; i < SIZE; ++i) {
        std::cout << "请输入第 " << i + 1 << " 名学生的姓名、学号、成绩：";
        std::cin >> students[i].name >> students[i].id >> students[i].score;
    }

    // 输出所有学生信息
    std::cout << "\n所有学生信息如下：" << std::endl;
    for (int i = 0; i < SIZE; ++i) {
        School::printStudent(&students[i]); // 传递常量指针
    }

    // 查找学生
    int searchId;
    std::cout << "\n请输入要查找的学号：";
    std::cin >> searchId;

    bool found = false;

    for (int i = 0; i < SIZE; ++i) {
        if (students[i].id == searchId) {
            found = true;

            float newScore;
            std::cout << "找到该学生，请输入新成绩：";
            std::cin >> newScore;

            // 更新成绩
            School::updateScore(&students[i], newScore);

            // 重新输出该学生信息
            std::cout << "更新后的学生信息：" << std::endl;
            School::printStudent(&students[i]);
            break;
        }
    }

    if (!found) {
        std::cout << "未找到该学号" << std::endl;
    }

    return 0;
}