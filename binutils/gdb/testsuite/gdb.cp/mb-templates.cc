
#include <iostream>
using namespace std;

template<class T>
void foo(T i)
{
  std::cout << "hi\n"; // set breakpoint here
}

template<class T>
void multi_line_foo(T i)
{
  std::cout // set multi-line breakpoint here
    << "hi\n";
}

int main()
{
    foo<int>(0);
    foo<double>(0);
    foo<int>(1);
    foo<double>(1);
    foo<int>(2);
    foo<double>(2);

    multi_line_foo<int>(0);
    multi_line_foo<double>(0);

    return 0;
}
