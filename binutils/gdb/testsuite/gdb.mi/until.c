void
foo (void)
{
 int i, x, y, z;

 x = 0;
 y = 1;
 i = 0;

 while (i < 2)
   i++;				/* in-loop */

 x = i;				/* after-loop */
 y = 2 * x;
 z = x + y;
 y = x + z;			/* until-here */
 x = 9;
 y = 10;			/* until-there */
}

int
main ()
{
  int a = 1;
  foo ();
  a += 2;
  return 0;			/* at-return */
}
