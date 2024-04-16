#include "ss.h"
#include <stdio.h>

typedef float f;

float sg = 5.5;
int sgi = 2;
static int sgs = 7;

int shr1(int x)
{
  f mumble;
  int l;
  l = 1;
  {
    int l;
    l = 2;
  }
  mumble = 7.7;
  sg = 6.6;
  sgi++;
  sgs = 8;
  printf("address of sgs is %p\n", &sgs);
  return 2*x;
}

static int shr1_local(int x)
{
  return 2*x;
}

int structarg(struct s x)
{
  return x.a;
}

int pstructarg(struct s *x)
{
  return x->a;
}



