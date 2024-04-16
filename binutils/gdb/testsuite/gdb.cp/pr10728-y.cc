#include "pr10728-x.h"
struct Y{};

X y()
{
  X xx;
  static Y yy;
  xx.y1 = &yy;
  xx.y2 = xx.y1+1;
  return xx;
}
