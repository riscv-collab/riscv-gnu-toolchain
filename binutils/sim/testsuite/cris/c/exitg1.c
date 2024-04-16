/* Check exit_group(2) trivially.  Newlib doesn't have it and the
   pre-v32 glibc requires updated headers we'd have to check or adjust
   for.
#progos: linux
#output: exit_group\n
*/
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef EXITVAL
#define EXITVAL 0
#endif
int main (int argc, char **argv)
{
  printf ("exit_group\n");
  syscall (SYS_exit_group, EXITVAL);
  printf ("failed\n");
  abort ();
}
