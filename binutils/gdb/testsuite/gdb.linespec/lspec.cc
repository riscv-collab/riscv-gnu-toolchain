#include "lspec.h"

static int dupname (int x) { label: return x; }

int NameSpace::overload()
{
  return 23;
}

int body_elsewhere()
{
  int x = 5;	/* body_elsewhere marker */
#include "body.h"
}

int main()
{
  return dupname(0) + m(0) + n(0) + f1() + f2() + body_elsewhere();
}
