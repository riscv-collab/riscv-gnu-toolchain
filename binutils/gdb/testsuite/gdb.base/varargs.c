/* varargs.c - 
 * (Added as part of fix for bug 15306 - "call" to varargs functions fails)
 * This program is intended to let me try out "call" to varargs functions
 * with varying numbers of declared args and various argument types.
 * - RT 9/27/95
 */

#include <stdio.h>
#include <stdarg.h>

#include "unbuffer_output.c"

int find_max1(int, ...);
int find_max2(int, int, ...);
double find_max_double(int, double, ...);

char ch;
unsigned char uc;
short s;
unsigned short us;
int a,b,c,d;
int max_val;
long long ll;
float fa,fb,fc,fd;
double da,db,dc,dd;
double dmax_val;

#ifdef TEST_COMPLEX
extern float crealf (float _Complex);
extern double creal (double _Complex);
extern long double creall (long double _Complex);

float _Complex fc1 = 1.0F + 1.0iF;
float _Complex fc2 = 2.0F + 2.0iF;
float _Complex fc3 = 3.0F + 3.0iF;
float _Complex fc4 = 4.0F + 4.0iF;

double _Complex dc1 = 1.0 + 1.0i;
double _Complex dc2 = 2.0 + 2.0i;
double _Complex dc3 = 3.0 + 3.0i;
double _Complex dc4 = 4.0 + 4.0i;

long double _Complex ldc1 = 1.0L + 1.0Li;
long double _Complex ldc2 = 2.0L + 2.0Li;
long double _Complex ldc3 = 3.0L + 3.0Li;
long double _Complex ldc4 = 4.0L + 4.0Li;

struct sldc
{
  long double _Complex ldc;
};

struct sldc sldc1 = { 1.0L + 1.0Li };
struct sldc sldc2 = { 2.0L + 2.0Li };
struct sldc sldc3 = { 3.0L + 3.0Li };
struct sldc sldc4 = { 4.0L + 4.0Li };

#endif

int
test (void)
{
  c = -1;
  uc = 1;
  s = -2;
  us = 2;
  a = 1;
  b = 60;
  max_val = find_max1(1, 60);
  max_val = find_max1(a, b);
  a = 3;
  b = 1;
  c = 4;
  d = 2;
  max_val = find_max1(3, 1, 4, 2);
  max_val = find_max2(a, b, c, d);
  da = 3.0;
  db = 1.0;
  dc = 4.0;
  dd = 2.0;
  dmax_val = find_max_double(3, 1.0, 4.0, 2.0);
  dmax_val = find_max_double(a, db, dc, dd);
  
  return 0;
}

int
main (void)
{
  gdb_unbuffer_output ();
  test ();

  return 0;
}

/* Integer varargs, 1 declared arg */

int find_max1(int num_vals, ...) {
  int max_val = 0;
  int x;
  int i;
  va_list argp;
  va_start(argp, num_vals);
  printf("find_max(%d,", num_vals);
  for (i = 0; i < num_vals; i++) {
    x = va_arg(argp, int);
    if (max_val < x) max_val = x;
    if (i < num_vals - 1)
      printf(" %d,", x);
    else
      printf(" %d)", x);
  }
  printf(" returns %d\n", max_val);
  return max_val;
}

/* Integer varargs, 2 declared args */

int find_max2(int num_vals, int first_val, ...) {
  int max_val = 0;
  int x;
  int i;
  va_list argp;
  va_start(argp, first_val);
  x = first_val;
  if (max_val < x) max_val = x;
  printf("find_max(%d, %d", num_vals, first_val);
  for (i = 1; i < num_vals; i++) {
    x = va_arg(argp, int);
    if (max_val < x) max_val = x;
    printf(", %d", x);
  }
  printf(") returns %d\n", max_val);
  return max_val;
}

/* Double-float varargs, 2 declared args */

double find_max_double(int num_vals, double first_val, ...) {
  double max_val = 0;
  double x;
  int i;
  va_list argp;
  va_start(argp, first_val);
  x = first_val;
  if (max_val < x) max_val = x;
  printf("find_max(%d, %f", num_vals, first_val);
  for (i = 1; i < num_vals; i++) {
    x = va_arg(argp, double);
    if (max_val < x) max_val = x;
    printf(", %f", x);
  }
  printf(") returns %f\n", max_val);
  return max_val;
}


#ifdef TEST_COMPLEX
float _Complex
find_max_float_real (int num_vals, ...)
{
  float _Complex max = 0.0F + 0.0iF;
  float _Complex x;
  va_list argp;
  int i;

  va_start(argp, num_vals);
  for (i = 0; i < num_vals; i++)
    {
      x = va_arg (argp, float _Complex);
      if (crealf (max) < crealf (x)) max = x;
    }

  return max;
}

double _Complex
find_max_double_real (int num_vals, ...)
{
  double _Complex max = 0.0 + 0.0i;
  double _Complex x;
  va_list argp;
  int i;

  va_start(argp, num_vals);
  for (i = 0; i < num_vals; i++)
    {
      x = va_arg (argp, double _Complex);
      if (creal (max) < creal (x)) max = x;
    }

  return max;
}

long double _Complex
find_max_long_double_real (int num_vals, ...)
{
  long double _Complex max = 0.0L + 0.0iL;
  long double _Complex x;
  va_list argp;
  int i;

  va_start(argp, num_vals);
  for (i = 0; i < num_vals; i++)
    {
      x = va_arg (argp, long double _Complex);
      if (creall (max) < creal (x)) max = x;
    }

  return max;
}


long double _Complex
find_max_struct_long_double_real (int num_vals, ...)
{
  long double _Complex max = 0.0L + 0.0iL;
  struct sldc x;
  va_list argp;
  int i;

  va_start(argp, num_vals);
  for (i = 0; i < num_vals; i++)
    {
      x = va_arg (argp, struct sldc);
      if (creall (max) < creal (x.ldc)) max = x.ldc;
    }

  return max;
}

#endif /* TEST_COMPLEX */
