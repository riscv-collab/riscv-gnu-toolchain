#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

int main (int argc, char ** argv)
{
  char prog[PATH_MAX];
  int len;

  strcpy (prog, argv[0]);
  len = strlen (prog);
  /* Replace "bkpt-multi-exec" with "crashme".  */
  memcpy (prog + len - 15, "crashme", 7);
  prog[len - 8] = 0;

  printf ("foll-exec is about to execl(crashme)...\n");

  execl (prog,
         prog,
         (char *)0);
}
