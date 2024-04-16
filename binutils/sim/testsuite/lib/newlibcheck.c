/* Used by the test harness to see if toolchain uses newlib.  */
#include <newlib.h>
#if defined(__NEWLIB__) || defined(_NEWLIB_VERSION)
int main()
{
  return 0;
}
#else
# error "not newlib"
#endif
