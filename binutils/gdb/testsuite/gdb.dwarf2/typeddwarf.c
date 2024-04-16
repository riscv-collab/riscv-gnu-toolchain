/* { dg-do run { target { i?86-*-* x86_64-*-* } } } */
/* { dg-options "-g" } */

typedef __SIZE_TYPE__ size_t;
volatile int vv;
extern void *memcpy (void *, const void *, size_t);

__attribute__((noinline, noclone)) void
f1 (double a, double b, double c, float d, float e, int f, unsigned int g, long long h, unsigned long long i)
{
  double j = d;			/* { dg-final { gdb-test 29 "j" "4" } } */
  long long l;			/* { dg-final { gdb-test 29 "l" "4616189618054758400" } } */
  memcpy (&l, &j, sizeof (l));
  long long m;			/* { dg-final { gdb-test 29 "m" "4613937818241073152" } } */
  memcpy (&m, &c, sizeof (l));
  float n = i;			/* { dg-final { gdb-test 29 "n" "9" } } */
  double o = h;			/* { dg-final { gdb-test 29 "o" "8" } } */
  float p = g;			/* { dg-final { gdb-test 29 "p" "7" } } */
  double q = f;			/* { dg-final { gdb-test 29 "q" "6" } } */
  unsigned long long r = a;	/* { dg-final { gdb-test 29 "r" "1" } } */
  long long s = c;		/* { dg-final { gdb-test 29 "s" "3" } } */
  unsigned t = d;		/* { dg-final { gdb-test 29 "t" "4" } } */
  int u = b;			/* { dg-final { gdb-test 29 "u" "2" } } */
  float v = a;			/* { dg-final { gdb-test 29 "v" "1" } } */
  double w = d / 4.0;		/* { dg-final { gdb-test 29 "w" "1" } } */
  double x = a + b + 1.0;	/* { dg-final { gdb-test 29 "x" "4" } } */
  double y = b + c + 2.0;	/* { dg-final { gdb-test 29 "y" "7" } } */
  float z = d + e + 3.0f;	/* { dg-final { xfail-gdb-test 29 "z" "12" "x86_64-*-*"} } */
  vv++;
}

__attribute__((noinline, noclone)) void
f2 (double a, double b, double c, float d, float e, int f, unsigned int g, long long h, unsigned long long i)
{
  double j = d;			/* { dg-final { gdb-test 53 "j" "4" } } */
  long long l;			/* { dg-final { gdb-test 53 "l" "4616189618054758400" } } */
  memcpy (&l, &j, sizeof (l));
  long long m;			/* { dg-final { gdb-test 53 "m" "4613937818241073152" } } */
  memcpy (&m, &c, sizeof (l));
  float n = i;			/* { dg-final { xfail-gdb-test 53 "n" "9" } } */
  double o = h;			/* { dg-final { xfail-gdb-test 53 "o" "8" } } */
  float p = g;			/* { dg-final { gdb-test 53 "p" "7" } } */
  double q = f;			/* { dg-final { gdb-test 53 "q" "6" } } */
  unsigned long long r = a;	/* { dg-final { gdb-test 53 "r" "1" } } */
  long long s = c;		/* { dg-final { gdb-test 53 "s" "3" } } */
  unsigned t = d;		/* { dg-final { gdb-test 53 "t" "4" } } */
  int u = b;			/* { dg-final { gdb-test 53 "u" "2" } } */
  float v = a;			/* { dg-final { gdb-test 53 "v" "1" } } */
  double w = d / 4.0;		/* { dg-final { gdb-test 53 "w" "1" } } */
  double x = a + b - 3 + 1.0e20;/* { dg-final { gdb-test 53 "x" "1e\\+20" } } */
  double y = b + c * 7.0;	/* { dg-final { gdb-test 53 "y" "23" } } */
  float z = d + e + 3.0f;	/* { dg-final { gdb-test 53 "z" "12" } } */
  vv++;
  vv = a;
  vv = b;
  vv = c;
  vv = d;
  vv = e;
  vv = f;
  vv = g;
  vv = h;
  vv = i;
  vv = j;
}

__attribute__((noinline, noclone)) void
f3 (long long a, int b, long long c, unsigned d)
{
  long long w = (a > d) ? a : d;/* { dg-final { gdb-test 73 "w" "4" } } */
  long long x = a + b + 7;	/* { dg-final { gdb-test 73 "x" "10" } } */
  long long y = c + d + 0x912345678LL;/* { dg-final { gdb-test 73 "y" "38960125567" } } */
  int z = (x + y);		/* { dg-final { gdb-test 73 "z" "305419913" } } */
  vv++;
}

__attribute__((noinline, noclone)) void
f4 (_Decimal32 a, _Decimal64 b, _Decimal128 c)
{
  _Decimal32 w = a * 8.0DF + 6.0DF;/* { dg-final { xfail-gdb-test 82 "(int)w" "70" } } */
  _Decimal64 x = b / 8.0DD - 6.0DD;/* { dg-final { xfail-gdb-test 82 "(int)x" "-4" } } */
  _Decimal128 y = -c / 8.0DL;	/* { dg-final { xfail-gdb-test 82 "(int)y" "-8" } } */
  vv++;
}

int
main ()
{
  f1 (1.0, 2.0, 3.0, 4.0f, 5.0f, 6, 7, 8, 9);
  f2 (1.0, 2.0, 3.0, 4.0f, 5.0f, 6, 7, 8, 9);
  f3 (1, 2, 3, 4);
  f4 (8.0DF, 16.0DD, 64.0DL);
  return 0;
}
