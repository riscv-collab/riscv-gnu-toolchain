#include <stdio.h>
int nothing ()

{
    int x = 3 ;
    return x ;
}


int main ()

{
    int y ;
    y = nothing () ;
    printf ("hello\n") ;
    return 0;
}
