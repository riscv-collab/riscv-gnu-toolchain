#include <stdio.h>
#include <stdlib.h>

static FILE *static_fun = NULL;

FILE *
force_static_fun (void)
{
  return static_fun;
}
