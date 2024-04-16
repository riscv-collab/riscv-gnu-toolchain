#include <stdio.h>


void long_line (); int oof (int);
void bar (int x)
/* -
   -
   - */
{
    x++;

    long_line ();
}

static void __attribute__ ((used))
unused ()
{
    /* Not used for anything */
}
/* This routine has a very long line that will break searching in older versions of GDB.  */

void

long_line ()
{
  oof (67);

  oof (6789);

  oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*  5 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 10 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 15 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 20 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 25 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 30 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 35 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 40 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 45 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 50 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 55 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 60 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /* 65 */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (12); /*    */      oof (12);  oof (12);  oof (12);  oof (12);  oof (12);  oof (1234); /* 70 */
}

int oof (int n)
/* o
   o
   o */
{
  return n + 1;
}
