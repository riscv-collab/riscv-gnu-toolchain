/* Test of anonymous union in a struct.  */

#include <string.h>

struct outer
{
  int one;
  int two;

  struct
  {
    union {
      int three : 3;
      int four : 4;
    };

    union {
      int five : 3;
      int six : 4;
    };
  } data;
};

int main ()
{
  struct outer val;

  memset (&val, 0, sizeof (val));
  val.data.six = 6;

  return 0;			/* break here */
}
