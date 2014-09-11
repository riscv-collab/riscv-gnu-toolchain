#include <string.h>
#include <stdint.h>

#undef memcpy
#undef __memcpy_g

void* __memcpy_g(void* aa, const void* bb, size_t n)
{
  #define BODY(a, b, t) { \
    t tt = *b; \
    a++, b++; \
    *(a-1) = tt; \
  }

  char* a = (char*)aa;
  const char* b = (const char*)bb;
  char* end = a+n;
  uintptr_t msk = sizeof(long)-1;
  if (__builtin_expect(((uintptr_t)a & msk) != ((uintptr_t)b & msk) || n < sizeof(long), 0))
  {
foo:
    if (__builtin_expect(a < end, 1))
      while (a < end)
        BODY(a, b, char);
    return aa;
  }

  if (__builtin_expect(((uintptr_t)a & msk) != 0, 0))
    while ((uintptr_t)a & msk)
      BODY(a, b, char);

  long* __restrict__ la = (long*)a;
  const long* __restrict__ lb = (const long*)b;
  long* lend = (long*)((uintptr_t)end & ~msk);

  if (__builtin_expect(la < lend-8, 0))
  {
    while (la < lend-8)
    {
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
      *la++ = *lb++;
    }
    if (la == lend)
      goto bar;
  }

  do BODY(la, lb, long) while (la < lend);

bar:
  a = (char*)la;
  b = (const char*)lb;
  if (__builtin_expect(a < end, 0))
    goto foo;
  return aa;
}
libc_hidden_def (__memcpy_g)
strong_alias (__memcpy_g, memcpy)
libc_hidden_builtin_def (memcpy)
