#include <stdlib.h>
#include <stdio.h>

int
main (int argc, char **argv)
{
  int *foo = NULL;

  printf ("Oh no, a bug!\n"); /* set breakpoint here */

  return *foo;
}
