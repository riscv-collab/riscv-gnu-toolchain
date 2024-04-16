int f2 (int a)
{
  return ++a;
}

int f1 (int a, int b)
{
  return f2(a) + b;
}

int main (int argc, char *argv[])
{
  return f1 (1, 2);
}
