
#include "dw4-sig-types.h"

extern myns::bar_type myset;

static myns::bar_type *
bar ()
{
  return &myset;
}

void
foo ()
{
  bar ();
}
