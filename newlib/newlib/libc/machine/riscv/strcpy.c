#include <string.h>
#include <stdint.h>

char* strcpy(char* dst, const char* src)
{
  char* dst0 = dst;

#if !defined(PREFER_SIZE_OVER_SPEED) && !defined(__OPTIMIZE_SIZE__)
  int misaligned = ((uintptr_t)dst | (uintptr_t)src) & (sizeof(long)-1);
  if (__builtin_expect(!misaligned, 1))
  {
    long* ldst = (long*)dst;
    const long* lsrc = (const long*)src;

    while (!__libc_detect_null(*lsrc))
      *ldst++ = *lsrc++;

    dst = (char*)ldst;
    src = (const char*)lsrc;

    char c0 = src[0];
    char c1 = src[1];
    char c2 = src[2];
    if (!(*dst++ = c0)) return dst0;
    if (!(*dst++ = c1)) return dst0;
    char c3 = src[3];
    if (!(*dst++ = c2)) return dst0;
    if (sizeof(long) == 4) goto out;
    char c4 = src[4];
    if (!(*dst++ = c3)) return dst0;
    char c5 = src[5];
    if (!(*dst++ = c4)) return dst0;
    char c6 = src[6];
    if (!(*dst++ = c5)) return dst0;
    if (!(*dst++ = c6)) return dst0;

out:
    *dst++ = 0;
    return dst0;
  }
#endif /* not PREFER_SIZE_OVER_SPEED */

  char ch;
  do
  {
    ch = *src;
    src++;
    dst++;
    *(dst-1) = ch;
  } while(ch);

  return dst0;
}
