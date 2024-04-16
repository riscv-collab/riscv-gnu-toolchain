/* Loop and vars for tests of display commands
*/
#include <stdio.h>
#define LOOP 10

int sum = 0;

/* Call to force a variable onto the stack so we can see its address.  */
void force_mem (int *arg) { }

int do_loops()
{
    int i=0;
    int k=0;
    int j=0;
    float f=3.1415;
    int *p_i = &i;

    for( i = 0; i < LOOP; i++ ) { /* set breakpoint 1 here */
        for( j = 0; j < LOOP; j++ ) {
            for( k = 0; k < LOOP; k++ ) {
                sum++; f++; force_mem (&k);
            }
        }
    } 
    return i; /* set breakpoint 2 here */
}

int do_vars()
{
    int       j;
    int       i = 9;
    float     f = 1.234;
    char      c = 'Q';
    int    *p_i = &i;
    float  *p_f = &f;
    char   *p_c = "rubarb and fries";

    /* Need some code here to set breaks on.
     */
    for( j = 0; j < LOOP; j++ ) {
        if( p_c[j] == c ) { /* set breakpoint 3 here */
            j++;
        } 
        else {
            i++;
        }
    }

    return *p_i;
}

int
main()
{
    do_loops();
    do_vars();    
    return 0;
}
