#include <iostream>
#include <string>
#include <stdexcept>

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
            throw invalid_argument("error! input integer plz");
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
                        throw runtime_error("error! divide by zero");
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
    catch (const invalid_argument& e)
    {
        cerr << e.what() << endl;
        return 1;
    }
    catch (const out_of_range& e)
    {
        cerr << "error! number is too large: " << e.what() << endl;
        return 1;
    }
    catch (const runtime_error& e)
    {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;

}