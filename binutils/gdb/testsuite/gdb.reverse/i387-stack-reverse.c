#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/* marks FPU stack as empty */
void empty_fpu_stack()
{
  asm ("ffree %st(1) \n\t"
       "ffree %st(2) \n\t"
       "ffree %st(3) \n\t"
       "ffree %st(4) \n\t"
       "ffree %st(5) \n\t"
       "ffree %st(6) \n\t"
       "ffree %st(7)");
}   

/* tests floating point arithmetic */
void test_arith_floats()
{
  
}

int main()
{
  empty_fpu_stack();    /* BEGIN I387-FLOAT-REVERSE */
  
  asm ("fld1");   /* test st0 register */
  asm ("fldl2t"); /* test st0, st1 */
  asm ("fldl2e"); /* test st0, st1, st2 */
  asm ("fldpi");  /* test st0, st1, st2, st3 */
  asm ("fldlg2"); /* test st0, st1, st2, st3, st4 */
  asm ("fldln2"); /* test st0, st1, st2, st3, st4, st5 */
  asm ("fldz");   /* test st0, st1, st2, st3, st4, st5, st6 */
  asm ("fld1");   /* test st0, st1, st2, st3, st4, st5, st6, st7 */
  asm ("nop");
  
  return 1;             /* END I387-FLOAT-REVERSE */
}
