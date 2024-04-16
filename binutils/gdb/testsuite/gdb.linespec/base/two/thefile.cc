/* The commented line must have the same line number in the other
   "thefile.c".  */

#define WANT_F2
#include "../../lspec.h"

static int dupname(int y)
{
 label: return y;
}

static int twodup ()
{
  return 0;
}

int n(int y)
{
  int v = dupname(y) - 23;	/* thefile breakpoint */
  return v + twodup ();		/* after dupname */
}

int NameSpace::overload(double x)
{
  return (int) x - 23;
}

int z2 ()
{
  return 0;
}
