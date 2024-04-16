/* Check that the syscall set_thread_area is supported and does the right thing.
#progos: linux
*/

#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#ifndef SYS_set_thread_area
#define SYS_set_thread_area 243
#endif

int main (void)
{
  int ret;

  /* Check the error check that the low 8 bits must be 0.  */
  ret = syscall (SYS_set_thread_area, 0xfeeb1ff0);
  if (ret != -1 || errno != EINVAL)
    {
      perror ("tls1");
      abort ();
    }

  ret = syscall (SYS_set_thread_area, 0xcafebe00);
  if (ret != 0)
    {
      perror ("tls2");
      abort ();
    }

  /* Check that we got the right result.  */
#ifdef __arch_v32
  asm ("move $pid,%0\n\tclear.b %0" : "=rm" (ret));
#else
  asm ("move $brp,%0" : "=rm" (ret));
#endif

  if (ret != 0xcafebe00)
    {
      perror ("tls2");
      abort ();
    }

  printf ("pass\n");
  exit (0);
}
