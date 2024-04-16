/* Check for a sim bug, whereby the position was always unsigned
   (truncation instead of sign-extension for 64-bit hosts).  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
  const char correct[] = "correct";
  char buf2[sizeof correct] = {0};
  int fd;

  f = fopen (fname, "wb");
  if (f == NULL
      || fwrite (tsttxt, 1, strlen (tsttxt), f) != strlen (tsttxt)
      || fclose (f) != 0)
    {
      printf ("fail\n");
      exit (1);
    }

  /* We have to use file-descriptor calls instead of stream calls to
     provoke the bug (for stream calls, the lseek call is canonicalized
     to use SEEK_SET).  */
  fd = open (fname, O_RDONLY);
  if (fd < 0
      || read (fd, buf, strlen (tsttxt)) != strlen (tsttxt)
      || strcmp (buf, tsttxt) != 0
      || lseek (fd, -30L, SEEK_CUR) != 36
      || read (fd, buf2, strlen (correct)) != strlen (correct)
      || strcmp (buf2, correct) != 0)
    {
      printf ("fail\n");
      exit (1);
    }

  printf ("pass\n");
  exit (0);
}
