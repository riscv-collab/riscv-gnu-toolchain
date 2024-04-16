/* pr 13484 */

#include <stdio.h>

int x;

void bar()
{
  x--;
}

void foo()
{
  x++;
}

int main()
{
  foo();
  bar();
  return 0;
}
