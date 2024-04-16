#include <altivec.h>
#include <stdio.h>

vector unsigned int
vector_fun (volatile vector unsigned int a, volatile vector unsigned int b)
{
  vector unsigned int c;
  a = ((vector unsigned int) vec_splat_u8(2));
  b = ((vector unsigned int) vec_splat_u8(3));

  c = vec_add (a, b);
  return c;
}
 
int
main ()
{
  vector unsigned int y; 
  vector unsigned int x; 
  vector unsigned int z; 
  int a = 0;

  #ifdef _AIX
  /* On AIX, the debugger cannot access vector registers before they
     are first used by the inferior.  Perform such an access here.  */
  x = ((vector unsigned int) vec_splat_u8 (0));
  #endif

  /* This line may look unnecessary but we do need it, because we want to
     have a line to do a next over (so that gdb refetches the registers)
     and we don't want the code to change any vector registers.
     The splat operations below modify the VRs,i
     so we don't want to execute them yet.  */
  a = 9; /* start here */
  x = ((vector unsigned int) vec_splat_u8 (-2));
  y = ((vector unsigned int) vec_splat_u8 (1));
	
  z = vector_fun (x, y);
  x = vec_sld (x,y,2);

  x = vec_add (x, ((vector unsigned int){5,6,7,8}));
  z = (vector unsigned int) vec_splat_u8 ( -2);
  y = vec_add (x, z);
  z = (vector unsigned int) vec_cmpeq (x,y);

  return 0;
}
