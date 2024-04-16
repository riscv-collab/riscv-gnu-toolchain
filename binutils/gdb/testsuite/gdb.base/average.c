/* This is a sample program for the HP WDB debugger. */

#include <stdio.h>
#include <stdlib.h>

extern int sum(int *, int, int);

#define num   10

static int my_list[num] = {3,4,2,0,2,1,8,3,6,7};

void print_average(int *list, int low, int high) 
    {
        int total = 0, num_elements = 0, average = 0;
        total = sum(list, low, high);
        num_elements = high - low;  /* note this is an off-by-one bug */

        average = total / num_elements;
        printf("%10.d\n", average);
    }

int main(void)
{
    char c;
    int first = 0, last = 0;	/* stop-in-main */
    last = num-1;

    /* Try two test cases. */
    print_average (my_list, first, last);
    print_average (my_list, first, last - 3); /* stop-at-call */

    exit(0);
}
