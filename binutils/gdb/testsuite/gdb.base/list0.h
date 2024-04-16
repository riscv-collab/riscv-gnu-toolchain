/* An include file that actually causes code to be generated in the including file.  This is known to cause problems on some systems. */

extern void bar(int);
static void foo (int x)
/* ! the next line has a control character, see PR symtab/24423.

   ! */
{
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
    bar (x++);
}
