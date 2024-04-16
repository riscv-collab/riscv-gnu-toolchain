/* Trivial test of writev.
#progos: linux
#output: abcdefghijklmn\npass\n
*/
#include <sys/uio.h>
#include <stdlib.h>
#include <stdio.h>

#define X(x) {x, sizeof (x) -1}
struct iovec v[] = {
  X("a"),
  X("bcd"),
  X("efghi"),
  X("j"),
  X("klmn\n"),
};

int main (void)
{
  if (writev (1, v, sizeof v / sizeof (v[0])) != 15)
    abort ();

  printf ("pass\n");
  return 0; 
}
