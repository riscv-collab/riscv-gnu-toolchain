/* This is a sample program for the HP/DDE debugger. */
#include <stdio.h>

int sum(int *list, int low, int high)
    {
        int i = 0, s = 0;	/* stop-in-sum */
        for (i = low; i <= high; i++)
            s += list[i];
        return(s);
    }
