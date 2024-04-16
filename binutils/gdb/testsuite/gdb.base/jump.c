/* This program is used to test the "jump" command.  There's nothing
   particularly deep about the functionality nor names in here.
   */

static int square (int x)
{
  return x*x;			/* out-of-func */
}


int main ()
{
  int i = 99;

  i++;
  i = square (i);		/* bp-on-call */
  i--;				/* bp-on-non-call */
  return 0;
}
