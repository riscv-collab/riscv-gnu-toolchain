typedef unsigned long size_t;
//#include <string.h>

void* memset(void* dst, int val, size_t n)
{
  void* dst0 = dst;
  void* end = dst + n;
  unsigned char v = val;

  /* is dst word-aligned? */
  if (((long)dst & (sizeof(long)-1)) == 0)
  {
    long lval = v;
    lval |= lval << 8;
    lval |= lval << 16;
    #ifdef __riscv64
    lval |= lval << 32;
    #elif !defined(__riscv32)
    # error
    #endif

    /* set 4 words at a time */
    for ( ; dst <= end - 4*sizeof(long); dst += 4*sizeof(long))
    {
      *(long*)(dst+0*sizeof(long)) = lval;
      *(long*)(dst+1*sizeof(long)) = lval;
      *(long*)(dst+2*sizeof(long)) = lval;
      *(long*)(dst+3*sizeof(long)) = lval;
    }

    /* set a word at a time */
    for ( ; dst <= end - sizeof(long); dst += sizeof(long))
      *(long*)dst = lval;
  }

  /* set a byte at a time */
  for ( ; dst < end; dst++)
    *(unsigned char*)dst = v;

  return dst0;
}

weak_alias (memset, __GI_memset)
