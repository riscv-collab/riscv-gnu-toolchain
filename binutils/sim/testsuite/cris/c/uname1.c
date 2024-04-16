/* Check that the right machine name appears in the uname result.
#progos: linux
*/
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
int main (void)
{
  struct utsname buf;
  if (uname (&buf) != 0
      || strcmp (buf.machine,
#ifdef __arch_v32
		 "crisv32"
#else
		 "cris"
#endif
		 ) != 0)
    abort ();
  printf ("pass\n");
  exit (0);
}
