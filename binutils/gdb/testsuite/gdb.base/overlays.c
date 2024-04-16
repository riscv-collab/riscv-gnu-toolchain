/* Support program for testing gdb's ability to debug overlays
   in the inferior.  */

#include "ovlymgr.h"

extern int foo (int);
extern int bar (int);
extern int baz (int);
extern int grbx (int);

int main ()
{
  int a, b, c, d, e;

  OverlayLoad (0);
  OverlayLoad (4);
  a = foo (1);
  OverlayLoad (1);
  OverlayLoad (5);
  b = bar (1);
  OverlayLoad (2);
  OverlayLoad (6);
  c = baz (1);
  OverlayLoad (3);
  OverlayLoad (7);
  d = grbx (1);
  e = a + b + c + d;
  return (e != ('f' + 'o' +'o'
		+ 'b' + 'a' + 'r'
		+ 'b' + 'a' + 'z'
		+ 'g' + 'r' + 'b' + 'x'));

}

