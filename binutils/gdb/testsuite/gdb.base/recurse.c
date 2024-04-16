/* Trivial code used to test watchpoints in recursive code and 
   auto-deletion of watchpoints as they go out of scope.  */

static int
recurse (int a)
{
  int b = 0;

  if (a == 1)
    return 1;

  b = a;
  b *= recurse (a - 1);
  return b;
}

int main()
{
  recurse (10);
  return 0;
}
