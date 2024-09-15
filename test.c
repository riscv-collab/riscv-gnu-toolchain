#include "console.h"
#include "string.h"
#include "skyline.h"


struct skyline_star skyline_stars[SKYLINE_STARS_MAX];
uint16_t skyline_star_cnt;

struct skyline_beacon skyline_beacon;


void main(void){
    skyline_star_cnt=0;
    add_star(63,89,(uint16_t)(0xbdff));
    add_star(63,89,(uint16_t)(0xbdfc));
    draw_star();

    console_printf("star num: %d\n", skyline_star_cnt);



}