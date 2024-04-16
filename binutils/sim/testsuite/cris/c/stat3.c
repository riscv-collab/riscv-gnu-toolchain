/* Simulator options:
#sim: --sysroot=$pwd
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
  /* Pick a regular file we know will always be in the sim builddir.  */
  char path[1024] = "/Makefile";
  struct stat buf;

  if (stat (".", &buf) != 0
      || !S_ISDIR (buf.st_mode))
    {
      fprintf (stderr, "cwd is not a directory\n");
      return 1;
    }
  if (stat (path, &buf) != 0
      || !S_ISREG (buf.st_mode))
    {
      fprintf (stderr, "%s: is not a regular file\n", path);
      return 1;
    }
  printf ("pass\n");
  exit (0);
}
