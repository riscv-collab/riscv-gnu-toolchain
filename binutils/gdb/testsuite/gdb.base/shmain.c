/* A test */

#include "ss.h"
#include <stdio.h>

#include "unbuffer_output.c"

extern int structarg(struct s);
extern int pstructarg(struct s*);
extern int shr1(int);
extern int shr2(int);
extern float sg;

int eglob;

struct {
 int a;  
 int b;
} s;

int g;

int local_structarg(struct s x)
{
  return x.b;
}

int mainshr1(int g)
{
  return 2*g;
}

int main()
{
  struct s y;

  gdb_unbuffer_output ();

  g = 1;
  g = shr1(g);
  g = shr2(g);
  g = mainshr1(g);
  sg = 1.1;
  y.a = 3;
  y.b = 4;
  g = local_structarg(y);
  g = structarg(y);
  g = pstructarg(&y);
  return 0;
}
