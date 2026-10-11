#include <iostream>
#include <string>
#include <limits>

// using namespace std;

namespace School {

    
    struct Student {
        std::string name; 
        int id;           
        float score;      
    };

    void printStudent(const Student* stu) {
        if (stu == nullptr) {
            std::cout << "学生信息为空！" << std::endl;
            return;
        }

        std::cout << "学号:" << stu->id
                  << ", 姓名:" << stu->name
                  << ", 成绩:" << stu->score << std::endl;
    }

    void updateScore(Student* stu, float newScore) {
        if (stu == nullptr) {
            std::cout << "学生信息为空，无法更新！" << std::endl;
            return;
        }

        if (newScore > 0.0f && newScore < 1000.0f ) { 
            stu->score = newScore;
            return;
        }else{
            std::cout << "错误：新成绩必须是数字且在 0~1000 之间，更新失败！" << std::endl;
        }

        
    }

} // namespace School 

int main() {
    int SIZE;   
    std::cout << "请输入学生人数：" << std::endl ;
    std::cin >> SIZE ;
    School::Student students[SIZE];

    if (SIZE <= 0)
    {
        std::cout << "没有学生信息，程序结束。" << std::endl;
        return 0;
    }else{
        for (int i = 0; i < SIZE; ++i)
        {
            std::cout << "请输入第 " << i + 1 << " 名学生的姓名、学号、成绩" << "(一共有" << SIZE << "名学生):" << std::endl;
            std::cin >> students[i].name >> students[i].id >> students[i].score;
        }

        // 输出所有学生信息
    label:
        std::cout << "\n所有学生信息如下" << "共" << SIZE << "名学生：" << std::endl;
        for (int i = 0; i < SIZE; ++i)
        {
            School::printStudent(&students[i]); // 传递常量指针
        }

        std::cout << "\n现在可以更新学生成绩了" << std::endl;
        std::cin.ignore(); // 忽略前一个输入后的换行符

        int t=-1;
        std::cout << "请输入1继续更新成绩,输入其他退出程序：" << std::endl;
        std::cin >> t;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if(t==1){
            do
            {
                // 查找学生
                /*int searchId;
                std::cout << "\n请输入要查找的学号：";
                std::cin >> searchId;*/

                std::string input;
                std::cout << "请输入(例如id:1001 或 name:张三)：\n";
                std::cin >> std::ws; // 清掉残留空白
                std::getline(std::cin, input);

                int id = -1;
                std::string name = "";

                if (input.rfind("id:", 0) == 0)
                {
                    id = std::stoi(input.substr(3));
                    // 按学号查找
                }
                else if (input.rfind("name:", 0) == 0)
                {
                    name = input.substr(5);
                    // 按姓名查找
                }
                else
                {
                    std::cout << "格式错误，请输入 id:数字 或 name:姓名" << std::endl;
                    continue;
                }
                bool found = false;

                for (int i = 0; i < SIZE; ++i)
                {
                    if (students[i].id == id || students[i].name == name)
                    {
                        found = true;

                        float newScore;
                        std::cout << "找到该学生，请输入新成绩：";
                        std::cin >> newScore;

                        // 更新成绩
                        School::updateScore(&students[i], newScore);

                        // 重新输出该学生信息
                        std::cout << "更新后的学生信息：" << std::endl;
                        School::printStudent(&students[i]);
                        // break;
                        goto label;
                    }
                }

                if (!found)
                {
                    std::cout << "未找到该学号/姓名" << std::endl;
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');  
                }
            } while (true);
        }else{
            std::cout << "不进入更新成绩环节，退出成绩程序" << std::endl;
        }
    }

    return 0;
}

/*
    just a single procedure is too loser;

    let me get a better one ,how about a loops without counting and data checking?


*/