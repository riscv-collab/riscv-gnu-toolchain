int f2 (int a)
{
  return ++a;
}

int f1 (int a, int b)
{
  return f2(a) + b;
}

int block (void)
{
  int i = 99;
  {
    double i = 1.1;
    double f = 2.2;
    {
      const char *i = "stuff";
      const char *f = "foo";
      const char *b = "bar";
      return 0; /* Block break here.  */
    }
  }
}

int main (int argc, char *argv[])
{
  block ();
  return f1 (1, 2);
}
