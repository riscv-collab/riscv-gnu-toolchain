/* { dg-do run { target { x86_64-*-* && lp64 } } } */
/* { dg-options "-g" } */

volatile int vv;

__attribute__((noinline, noclone)) long
foo (long x)
{
  long f = __builtin_clzl (x);
  long g = f;
  asm volatile ("" : "+r" (f));
  vv++;		/* { dg-final { gdb-test 12 "g" "43" } } */
  return f;	/* { dg-final { gdb-test 12 "f" "43" } } */
}

__attribute__((noinline, noclone)) long
bar (long x)
{
  long f = __builtin_clzl (x);
  long g = f;
  asm volatile ("" : "+r" (f));
  vv++;		/* { dg-final { gdb-test 22 "g" "33" } } */
  return f;	/* { dg-final { gdb-test 22 "f" "33" } } */
}

int
main ()
{
  long x = vv;
  foo (x + 0x123456UL);
  bar (x + 0x7fffffffUL);
  return 0;
}
