/* Tests that stdin can be redirected from a normal file.  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (void)
{
   const char* fname = "freopen.dat";
   const char tsttxt[]
       = "A random line of text, used to test correct freopen etc.\n";
   FILE* instream;
   FILE *old_stderr;
   char c1;

   /* Like the freopen call in flex.  */
   old_stderr = freopen (fname, "w+", stderr);
   if (old_stderr == NULL
      || fwrite (tsttxt, 1, strlen (tsttxt), stderr) != strlen (tsttxt)
      || fclose (stderr) != 0)
   {
      printf ("fail\n");
      exit (1);
   }

   instream = freopen(fname, "r", stdin);
   if (instream == NULL) {
      printf("fail\n");
      exit(1);
   }

   c1 = getc(instream);
   if (c1 != 'A') {
      printf("fail\n");
      exit(1);
   }

   printf ("pass\n");
   exit(0);
}
