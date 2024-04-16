/* Simple shared library */

int multiply(int a, int b)
{
  return a * b;
}

int square(int num)
{
  int res = multiply(num, num);
  return res;
}
