/* Check for a sim bug, whereby an invalid seek (to a negative offset)
   did not return an error.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int
main (void)
{
  FILE *f;
  const char fname[] = "sk1test.dat";
  const char tsttxt[]
    = "A random line of text, used to test correct read, write and seek.\n";
  char buf[sizeof tsttxt] = "";
  int fd;

  f = fopen (fname, "wb");
  if (f == NULL
      || fwrite (tsttxt, 1, strlen (tsttxt), f) != strlen (tsttxt)
      || fclose (f) != 0)
    {
      printf ("fail\n");
      exit (1);
    }

  fd = open (fname, O_RDONLY);
  if (fd < 0
      || lseek (fd, -1L, SEEK_CUR) != -1
      || errno != EINVAL
      || read (fd, buf, strlen (tsttxt)) != strlen (tsttxt)
      || strcmp (buf, tsttxt) != 0)
    {
      printf ("fail\n");
      exit (1);
    }

  printf ("pass\n");
  exit (0);
}
