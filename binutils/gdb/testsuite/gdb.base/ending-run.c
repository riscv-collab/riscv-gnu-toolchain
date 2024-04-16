/* Test program for <next-at-end> and
 * <leaves-core-file-on-quit> bugs.
 */
#include <stdio.h>
#include <stdlib.h>

#include "unbuffer_output.c"

int callee (int x)
{
    int y = x * x;		/* -break1- */
    return (y - 2);
}

int main()
{

    int *p;
    int i;

    gdb_unbuffer_output ();

    p = (int *) malloc( 4 );

    for (i = 1; i < 10; i++)
        {
            printf( "%d ", callee( i ));
            fflush (stdout);
        }
    printf( " Goodbye!\n" ); fflush (stdout); /* -break2- */
    return 0;
}
