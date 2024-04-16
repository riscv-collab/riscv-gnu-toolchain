#include "hang.H"

struct B
{
  int member_of_B;
};

int var_in_b = 1729;

int dummy2 (void)
{
  return var_in_b;
}
