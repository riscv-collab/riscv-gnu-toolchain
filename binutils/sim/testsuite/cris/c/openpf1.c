/* Check that --sysroot is applied to open(2).
#sim: --sysroot=$pwd

   We assume, with EXE being the name of the executable:
   - The simulator executes with cwd the same directory where the executable
     is located (also argv[0] contains a plain filename without directory
     components -or- argv[0] contains the full non-sysroot path to EXE).
   - There's no /EXE on the host file system.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
int main (int argc, char *argv[])
{
  char *fnam = argv[0];
  FILE *f;
  if (argv[0][0] != '/')
    {
      fnam = malloc (strlen (argv[0]) + 2);
      if (fnam == NULL)
	abort ();
      strcpy (fnam, "/");
      strcat (fnam, argv[0]);
    }
  else
    fnam = strrchr (argv[0], '/');

  f = fopen (fnam, "rb");
  if (f == NULL)
    abort ();
  fclose (f);

  /* Cover another execution path.  */
  if (fopen ("/nonexistent", "rb") != NULL
      || errno != ENOENT)
    abort ();
  printf ("pass\n");
  return 0;
}
