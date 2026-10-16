#include <iostream>
#include <string>

using namespace std;

int main()
{
    cout << "input two numbers " << endl;
    
    string a, b;
    char op;

    cin >> a >> op >> b;

    try 
    {
        size_t pos1 = 0;
        size_t pos2 = 0;
        int num1 = stoi(a, &pos1);
        int num2 = stoi(b, &pos2);
        if (pos1 != a.size() || pos2 != b.size())
        {
            cout << " error! input integer plz " << endl;
        }else 
        {
            switch (op)
            {
                case '+':
                    cout << num1 + num2 << endl;
                    break;

                case '-':
                    cout << num1 - num2 << endl;
                    break;

                case '*':
                    cout << num1 * num2 << endl;
                    break;
                case '/':
                    if (num2 == 0)
                    {
                        cout << "error! divide by zero" << endl;
                    }
                    else
                    {
                        cout << num1 / num2 << endl;
                    }
                    break;
                default:
                    cout << "error! try again plz" << endl;
                    break;
            }
        }
        
    }
    catch (const invalid_argument&)
    {
        cout << "error! input integer plz" << endl;
    }
    catch (const out_of_range&)
    {
        cout << "error! number is too large" << endl;
    }

    return 0;

}