/* pr 13484 */

#include <stdio.h>

int x;

void foo()
{
  x++;
  printf("This is foo\n");
}

int main()
{
  foo();
  return 0;
}

/* Ensure the new file will have more sections.  It may exploit code not
   updating its SECTION_COUNT on reread_symbols.  */

#ifndef NO_SECTIONS
# define VAR0(n) __attribute__ ((section ("sect" #n))) int var##n;
# define VAR1(n) VAR0 (n ## 0) VAR0(n ## 1) VAR0(n ## 2) VAR0(n ## 3)
# define VAR2(n) VAR1 (n ## 0) VAR1(n ## 1) VAR1(n ## 2) VAR1(n ## 3)
# define VAR3(n) VAR2 (n ## 0) VAR2(n ## 1) VAR2(n ## 2) VAR2(n ## 3)
VAR3 (0)
#endif
