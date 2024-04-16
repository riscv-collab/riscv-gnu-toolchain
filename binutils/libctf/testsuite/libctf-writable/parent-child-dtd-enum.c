#include "parent-child-dtd-crash-lib.c"

int main (void)
{
  dtd_crash(ADD_ENUM, 0);
  dtd_crash(ADD_ENUM, 1);

  printf("Creation successful.\n");

  return 0;
}
