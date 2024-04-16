#include "parent-child-dtd-crash-lib.c"

int main (void)
{
  dtd_crash(ADD_MEMBER_ENCODED, 0);
  dtd_crash(ADD_MEMBER_ENCODED, 1);

  printf("Creation successful.\n");

  return 0;
}
