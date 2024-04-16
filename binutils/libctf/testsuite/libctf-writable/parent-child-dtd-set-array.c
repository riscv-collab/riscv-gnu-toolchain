#include "parent-child-dtd-crash-lib.c"

int main (void)
{
  dtd_crash(SET_ARRAY, 0);
  dtd_crash(SET_ARRAY, 1);

  printf("Creation successful.\n");

  return 0;
}
