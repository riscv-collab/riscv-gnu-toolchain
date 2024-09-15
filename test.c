#include "skyline.h"
#include "console.h"  // Assuming console_printf is defined here
#include <string.h>   // For memset

struct skyline_star skyline_stars[SKYLINE_STARS_MAX];
uint16_t skyline_star_cnt;


void print_stars() {
    console_printf("Total stars: %d\n", skyline_star_cnt);
    for (int i = 0; i < skyline_star_cnt; i++) {
        console_printf("Star %d: x=%d, y=%d, color=0x%04x\n", i, skyline_stars[i].x, skyline_stars[i].y, skyline_stars[i].color);
    }
}

void main(void) {
    // Initialize stars array and count
    memset(skyline_stars, 0, sizeof(skyline_stars));
    skyline_star_cnt = 0;

    // Test case: Add one star
    add_star(100, 200, 0xbdff);  // Add a star at position (100, 200) with color 0xbdff

    console_printf("Test Case: Add one star\n");
    print_stars();

    // Expected output:
    // Total stars: 1
    // Star 0: x=100, y=200, color=0xbdff
}
