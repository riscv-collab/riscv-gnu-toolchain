/*
#progos: linux
*/

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main (void)
{
  if (sched_getscheduler (getpid ()) != SCHED_OTHER)
    abort ();
  printf ("pass\n");
  exit (0);
}
