/* The commented line must have the same line number in the other
   "thefile.c".  */

#define WANT_F1
#include "../../lspec.h"






static int twodup ()
{
  return 0;
}

int m(int x)
{
  return x + 23 + twodup ();	/* thefile breakpoint */
}

int NameSpace::overload(int x)
{
  return x + 23;
}

int z1 ()
{
  return 0;
}
