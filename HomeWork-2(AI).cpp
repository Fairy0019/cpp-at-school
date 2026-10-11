#include <iostream>
#include <string>

namespace School {
    struct Student {
        std::string name;
        int id;
        float score;
    };

    Student* createStudents(int count = 3) {
        return new Student[count];
    }

    void freeStudents(Student* students) {
        delete[] students;
    }

    void printStudent(const Student* stu, bool showScore = true) {
        if (stu == nullptr) return;
        std::cout << "学号:" << stu->id << ", 姓名:" << stu->name;
        if (showScore) {
            std::cout << ", 成绩:" << stu->score;
        }
        std::cout << std::endl;
    }

    void printStudent(const Student* students, int count) {
        for (int i = 0; i < count; ++i) {
            printStudent(&students[i]);
        }
    }

    void updateScore(Student* stu, float newScore = 60.0f) {
        if (stu == nullptr) return;
        if (newScore < 0.0f || newScore > 100.0f) {
            std::cout << "错误：新成绩必须在 0~100 之间，更新失败！" << std::endl;
            return;
        }
        stu->score = newScore;
    }

    int updateScore(Student* students, int count, int id, float newScore) {
        int updated = 0;
        for (int i = 0; i < count; ++i) {
            if (students[i].id == id) {
                if (newScore >= 0.0f && newScore <= 100.0f) {
                    students[i].score = newScore;
                    ++updated;
                } else {
                    std::cout << "错误：新成绩必须在 0~100 之间，更新失败！" << std::endl;
                }
            }
        }
        return updated;
    }

    float averageScore(const Student* students, int count) {
        if (count <= 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < count; ++i) {
            sum += students[i].score;
        }
        return sum / count;
    }

    float averageScore(const Student* students, int count, bool excludeFailing) {
        if (count <= 0) return 0.0f;
        float sum = 0.0f;
        int num = 0;
        for (int i = 0; i < count; ++i) {
            if (excludeFailing) {
                if (students[i].score >= 60.0f) {
                    sum += students[i].score;
                    ++num;
                }
            } else {
                sum += students[i].score;
                ++num;
            }
        }
        if (num == 0) return 0.0f;
        return sum / num;
    }
}

int main() {
    const int SIZE = 3;
    School::Student* students = School::createStudents(SIZE);

    for (int i = 0; i < SIZE; ++i) {
        std::cout << "请输入第 " << i + 1 << " 名学生的姓名、学号、成绩：";
        std::cin >> students[i].name >> students[i].id >> students[i].score;
    }

    std::cout << "\n所有学生信息如下：" << std::endl;
    School::printStudent(students, SIZE);

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
            int updated = School::updateScore(students, SIZE, searchId, newScore);
            if (updated > 0) {
                std::cout << "更新成功！" << std::endl;
            }
            std::cout << "更新后的全部学生信息：" << std::endl;
            School::printStudent(students, SIZE);
            break;
        }
    }
    if (!found) {
        std::cout << "未找到该学号" << std::endl;
    }

    std::cout << "\n全体平均分：" << School::averageScore(students, SIZE) << std::endl;
    std::cout << "及格学生平均分：" << School::averageScore(students, SIZE, true) << std::endl;

    std::cout << "\n演示默认参数：" << std::endl;
    School::printStudent(&students[0]);
    School::printStudent(&students[0], false);

    School::freeStudents(students);
    students = nullptr;

    return 0;
}