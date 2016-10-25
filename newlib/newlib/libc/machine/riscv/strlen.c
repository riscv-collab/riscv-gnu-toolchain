#include <string.h>
#include <stdint.h>

size_t strlen(const char* str)
{
  const char* start = str;

#if defined(PREFER_SIZE_OVER_SPEED) || defined(__OPTIMIZE_SIZE__)
  while (*str++)
    ;
  return str - start - 1;
#else
  if (__builtin_expect((uintptr_t)str & (sizeof(long)-1), 0)) do
  {
    char ch = *str;
    str++;
    if (!ch)
      return str - start - 1;
  } while ((uintptr_t)str & (sizeof(long)-1));

  unsigned long* ls = (unsigned long*)str;
  while (!__libc_detect_null(*ls++))
    ;
  asm volatile ("" : "+r"(ls)); /* prevent "optimization" */

  str = (const char*)ls;
  size_t ret = str - start, sl = sizeof(long);

  char c0 = str[0-sl], c1 = str[1-sl], c2 = str[2-sl], c3 = str[3-sl];
  if (c0 == 0)            return ret + 0 - sl;
  if (c1 == 0)            return ret + 1 - sl;
  if (c2 == 0)            return ret + 2 - sl;
  if (sl == 4 || c3 == 0) return ret + 3 - sl;

  c0 = str[4-sl], c1 = str[5-sl], c2 = str[6-sl], c3 = str[7-sl];
  if (c0 == 0)            return ret + 4 - sl;
  if (c1 == 0)            return ret + 5 - sl;
  if (c2 == 0)            return ret + 6 - sl;

  return ret + 7 - sl;
#endif /* not PREFER_SIZE_OVER_SPEED */
}
