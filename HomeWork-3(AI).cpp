#include <iostream>
#include <string>

class Student {
private:
    std::string name;
    int id;
    float score;

public:
    Student() : name("未命名"), id(0), score(0.0f) {
        std::cout << "默认构造函数被调用" << std::endl;
    }

    Student(const std::string& name, int id, float score = 0.0f)
        : name(name), id(id), score(score) {
        std::cout << "带参构造函数被调用" << std::endl;
    }

    ~Student() {
        std::cout << "析构函数被调用" << std::endl;
    }

    void setInfo(const std::string& name, int id, float score) {
        this->name = name;
        this->id = id;
        this->score = score;
    }

    const std::string& getName() const {
        return name;
    }

    int getId() const {
        return id;
    }

    float getScore() const {
        return score;
    }

    void setScore(float score) {
        this->score = score;
    }
};

Student* createStudents(int count = 3) {
    if (count <= 0) {
        return nullptr;
    }
    return new Student[count];
}

void freeStudents(Student* students) {
    delete[] students;
}

void printStudent(const Student* stu, bool showScore = true) {
    if (stu == nullptr) {
        std::cout << "学生信息为空" << std::endl;
        return;
    }

    std::cout << "学号:" << stu->getId()
              << ", 姓名:" << stu->getName();
    if (showScore) {
        std::cout << ", 成绩:" << stu->getScore();
    }
    std::cout << std::endl;
}

void printStudent(const Student* students, int count) {
    if (students == nullptr || count <= 0) {
        std::cout << "没有可输出的学生信息" << std::endl;
        return;
    }

    for (int i = 0; i < count; ++i) {
        printStudent(&students[i]);
    }
}

void updateScore(Student* stu, float newScore = 60.0f) {
    if (stu == nullptr) {
        std::cout << "学生信息为空，无法更新成绩" << std::endl;
        return;
    }
    if (newScore < 0.0f || newScore > 100.0f) {
        std::cout << "错误：新成绩必须在 0~100 之间，更新失败！" << std::endl;
        return;
    }
    stu->setScore(newScore);
}

int main() {
    const int SIZE = 3;
    Student* students = createStudents(SIZE);
    if (students == nullptr) {
        std::cerr << "学生数组创建失败" << std::endl;
        return 1;
    }

    for (int i = 0; i < SIZE; ++i) {
        std::string name;
        int id;
        float score;
        std::cout << "请输入第 " << i + 1 << " 名学生的姓名、学号和成绩：";
        if (!(std::cin >> name >> id >> score)) {
            std::cerr << "输入格式错误" << std::endl;
            freeStudents(students);
            students = nullptr;
            return 1;
        }
        students[i].setInfo(name, id, score);
    }

    std::cout << "\n全部学生信息：" << std::endl;
    printStudent(students, SIZE);

    std::cout << "\n默认参数：" << std::endl;
    printStudent(&students[0]);
    printStudent(&students[0], false);

    freeStudents(students);
    students = nullptr;
    return 0;
}
