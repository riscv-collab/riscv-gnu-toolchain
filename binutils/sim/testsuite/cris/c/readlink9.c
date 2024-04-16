/* Check that odd cases of readlink work.
#progos: linux
#cc: additional_flags=-DX="$pwd"
*/

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
  /* We assume that "sim/testsuite" isn't renamed to anything that
     together with "<builddir>/" is shorter than 7 characters.  */
  char buf[7];

  if (readlink("/proc/42/exe", buf, sizeof (buf)) != sizeof (buf)
      || strncmp (buf, X, sizeof (buf)) != 0)
    abort ();

  printf ("pass\n");
  exit (0);
}
