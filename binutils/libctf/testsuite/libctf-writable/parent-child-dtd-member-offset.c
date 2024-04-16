#include "parent-child-dtd-crash-lib.c"

int main (void)
{
  dtd_crash(ADD_MEMBER_OFFSET, 0);
  dtd_crash(ADD_MEMBER_OFFSET, 1);

  printf("Creation successful.\n");

  return 0;
}
