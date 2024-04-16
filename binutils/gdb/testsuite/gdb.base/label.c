#include <stdio.h>

int
main (int argc, char **argv)
{
  int i = 0;
  goto there;

here:
  printf("not here\n");
  i = 1;
  
there:
  printf("but here\n");
  if (i == 0)
    goto here;

done:
  return 0;
}

