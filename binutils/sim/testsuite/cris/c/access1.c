/* Check access(2) trivially.  Newlib doesn't have it.
#progos: linux
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
int main (int argc, char **argv)
{
  if (access (argv[0], R_OK|W_OK|X_OK) == 0
      && access ("/dev/null", R_OK|W_OK) == 0
      && access ("/dev/null", X_OK) == -1
      && errno == EACCES)
    printf ("pass\n");
  exit (0);
}
