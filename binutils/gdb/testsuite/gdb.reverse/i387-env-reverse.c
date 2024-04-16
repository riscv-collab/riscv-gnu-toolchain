#include <stdio.h>
#include <math.h>
#include <stdlib.h>


float no1,no2,no3,no4,no5,no6,no7;
float result,resultd,resultld; 
float *float_memory;
long double ldx = 88888888888888888888.88, ldy = 9999999999999999999.99;
double x = 100.345, y = 25.7789;
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

/* initialization of floats */
void init_floats()
{
  no1 = 10.45;
  no2 = 20.77;
  no3 = 156.89874646;
  no4 = 14.56;
  no5 = 11.11;
  no6 = 66.77;
  no7 = 88.88;
  float_memory = malloc(sizeof(float) * 4);
  *float_memory = 256.256;
  *(float_memory + 1) = 356.356;
  *(float_memory + 2) = 456.456;
  *(float_memory + 3) = 556.556;
}

int main()
{
  init_floats();
  empty_fpu_stack();    /* BEGIN I387-FLOAT-REVERSE */  
  
  asm("nop");   /* TEST ENV */
  asm ("fsave %0" : "=m"(*float_memory) : );   
  asm ("frstor %0" : : "m"(*float_memory));
  asm ("fstsw %ax");     /* test eax register */

  asm ("fld1");
  asm ("fldl2t");
  asm ("fldl2e");
  asm ("fldpi");
  asm ("fldlg2");
  asm ("fldln2");
  asm ("fldz");
  asm ("nop");
  
  return 1;             /* END I387-FLOAT-REVERSE */
}
