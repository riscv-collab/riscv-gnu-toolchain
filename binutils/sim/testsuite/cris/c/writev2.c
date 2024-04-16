/* Trivial test of failing writev: invalid file descriptor.
#progos: linux
*/
#include <sys/uio.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

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
  if (writev (99, v, sizeof v / sizeof (v[0])) != -1
      /* The simulator write gives EINVAL instead of EBADF; let's
	 cope.  */
      || (errno != EBADF && errno != EINVAL))
    abort ();

  printf ("pass\n");
  return 0; 
}
