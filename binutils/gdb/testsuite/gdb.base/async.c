

int
foo (void)
{
 int y;
 volatile int x;

 x = 5; x = 5; x = 5;
 y = 3;

 return x + y;
}

int
baz (void)
{
  return 5;
}

int
main (void)
{
 int y, z;
 
 y = 2;
 z = 9;
 y = foo ();
 z = y;
 y = y + 2; /* jump here */
 y = baz ();
 return 0; /* until here */
}
