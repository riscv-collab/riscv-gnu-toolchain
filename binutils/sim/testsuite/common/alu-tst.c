#define WITH_TARGET_WORD_MSB 0
#define WITH_TARGET_WORD_BITSIZE 64
#define WITH_HOST_WORD_BITSIZE (sizeof (int) * 8)
#define WITH_TARGET_BYTE_ORDER BIG_ENDIAN /* does not matter */

#define ASSERT(EXPRESSION) \
{ \
  if (!(EXPRESSION)) { \
    fprintf (stderr, "%s:%d: assertion failed - %s\n", \
             __FILE__, __LINE__, #EXPRESSION); \
    abort (); \
  } \
}

#define SIM_BITS_INLINE (INCLUDE_MODULE | INCLUDED_BY_MODULE)

#include <stdlib.h>
#include <string.h>

#define PACKAGE "sim"

#include "sim-basics.h"

#include "sim-alu.h"

#include <stdio.h>

typedef struct {
  char *op;
  uint64_t arg;
} alu_op;

typedef struct {
  uint64_t begin;
  alu_op ops[4];
  uint64_t result;
  int carry_borrow;
  int overflow;
} alu_test;

#define MAX_INT8 UNSIGNED64 (127)
#define MIN_INT8 UNSIGNED64 (128)

#define MAX_INT16 UNSIGNED64 (32767)
#define MIN_INT16 UNSIGNED64 (32768)

#define MAX_INT32 UNSIGNED64 (0x7fffffff)
#define MIN_INT32 UNSIGNED64 (0x80000000)

#define MAX_INT64 UNSIGNED64 (0x7fffffffffffffff)
#define MIN_INT64 UNSIGNED64 (0x8000000000000000)

static void
print_hex (uint64_t val, int nr_bits)
{
  switch (nr_bits)
    {
    case 8:
      printf ("0x%02lx", (long) (uint8_t) (val));
      break;
    case 16:
      printf ("0x%04lx", (long) (uint16_t) (val));
      break;
    case 32:
      printf ("0x%08lx", (long) (uint32_t) (val));
      break;
    case 64:
      printf ("0x%016llx", (long long) (uint64_t) (val));
      break;
    default:
      abort ();
    }
}


int errors = 0;


#define N 8
#include "alu-n-tst.h"
#undef N

#define N 16
#include "alu-n-tst.h"
#undef N

#define N 32
#include "alu-n-tst.h"
#undef N

#define N 64
#include "alu-n-tst.h"
#undef N



int
main ()
{
  do_alu_8_tests ();
  do_alu_16_tests ();
  do_alu_32_tests ();
  do_alu_64_tests ();
  return (errors != 0);
}
