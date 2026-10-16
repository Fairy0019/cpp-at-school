#include <iostream>
#include <string>
#include <stdexcept>
#include <cctype>
using namespace std;

// 将字符串转换为整数，如果不是合法整数则抛出异常
int parseInt(const string &s)
{
    if (s.empty())
    {
        throw invalid_argument("输入的不是整数");
    }

    size_t start = 0;
    if (s[0] == '+' || s[0] == '-')
    {
        if (s.size() == 1)
        {
            throw invalid_argument("输入的不是整数");
        }
        start = 1;
    }

    for (size_t i = start; i < s.size(); ++i)
    {
        if (!isdigit(static_cast<unsigned char>(s[i])))
        {
            throw invalid_argument("输入的不是整数");
        }
    }

    try
    {
        return stoi(s);
    }
    catch (const out_of_range &)
    {
        throw invalid_argument("整数超出范围");
    }
}

int main()
{
    try
    {
        string s1, s2;

        cout << "请输入两个整数: ";
        if (!(cin >> s1 >> s2))
        {
            throw runtime_error("输入失败");
        }

        int a = parseInt(s1);
        int b = parseInt(s2);

        char op;
        cout << "请选择运算 (+,-,*,/): ";
        cin >> op;

        int result = 0;
        switch (op)
        {
        case '+':
            result = a + b;
            break;
        case '-':
            result = a - b;
            break;
        case '*':
            result = a * b;
            break;
        case '/':
            if (b == 0)
            {
                throw runtime_error("除数不能为零");
            }
            result = a / b;
            break;
        default:
            throw runtime_error("无效的运算符");
        }

        cout << "结果: " << result << endl;
    }
    catch (const exception &e)
    {
        cerr << "错误: " << e.what() << endl;
        return 1;
    }

    return 0;
}