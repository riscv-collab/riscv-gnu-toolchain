#include <stdlib.h>
#include <stdint.h>

/* Test that things still (sort of) work when compiled without -g.  */

int dataglobal = 3;			/* Should go in global data */
static int datalocal = 4;		/* Should go in local data */
int bssglobal;				/* Should go in global bss */
static int bsslocal;			/* Should go in local bss */

/* Non-int-sized global data variables.  */
uint8_t dataglobal8 = 0xff;
uint32_t dataglobal32_1 = 0x7fffffff;
uint32_t dataglobal32_2 = 0x000000ff;
uint64_t dataglobal64_1 = 0x7fffffffffffffff;
uint64_t dataglobal64_2 = 0x00000000000000ff;

int
inner (int x)
{
  return x + dataglobal + datalocal + bssglobal + bsslocal;
}

static short
middle (int x)
{
  return 2 * inner (x);
}

short
top (int x)
{
  return 2 * middle (x);
}

int
main (int argc, char **argv)
{
  return top (argc);
}

int *x;

int array_index (char *arr, int i)
{
  /* The basic concept is just "return arr[i];".  But call malloc so that gdb
     will be able to call functions.  */
  char retval;
  x = (int *) malloc (sizeof (int));
  *x = i;
  retval = arr[*x];
  free (x);
  return retval;
}

float
multf (float v1, float v2)
{
  return v1 * v2;
}

float
multf_noproto (v1, v2)
  float v1, v2;
{
  return v1 * v2;
}

double
mult (double v1, double v2)
{
  return v1 * v2;
}

double
mult_noproto (v1, v2)
  double v1, v2;
{
  return v1 * v2;
}

uint8_t
add8 (uint8_t v1, uint8_t v2)
{
  return v1 + v2;
}

uint8_t
add8_noproto (v1, v2)
  uint8_t v1, v2;
{
  return v1 + v2;
}
