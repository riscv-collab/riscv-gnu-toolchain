#include "console.h"
#include "string.h"
#include "skyline.h"


struct skyline_star skyline_stars[SKYLINE_STARS_MAX];
uint16_t skyline_star_cnt;
struct skyline_beacon skyline_beacon;


#define FRAMEBUFFER_WIDTH 640
#define FRAMEBUFFER_HEIGHT 480
uint16_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];

struct skyline_window *skyline_win_list = NULL; // Initialize window list




void print_framebuffer_region(int x_start, int y_start, int width, int height) {
    for (int y = y_start; y < y_start + height; y++) {
        for (int x = x_start; x < x_start + width; x++) {
            uint16_t pixel = framebuffer[y * FRAMEBUFFER_WIDTH + x];
            console_printf("Pixel at (%d, %d): 0x%04x\n", x, y, pixel);
        }
    }
}



void main(void){
    // Initialize the framebuffer to a known value (e.g., 0x0000)
    memset(framebuffer, 0x00, sizeof(framebuffer));

    // Define beacon image data (e.g., 3x3 green square)
    uint16_t beacon_img[9] = {
        0x07E0, 0x07E0, 0x07E0, // Row 1
        0x07E0, 0x07E0, 0x07E0, // Row 2
        0x07E0, 0x07E0, 0x07E0  // Row 3
    };

    // Define a beacon with specific properties
    struct skyline_beacon beacon;
    beacon.img = beacon_img;  // Image data (3x3)
    beacon.x = 100;           // X coordinate of the center
    beacon.y = 150;           // Y coordinate of the center
    beacon.dia = 3;           // Diameter (3x3 square)
    beacon.period = 10;       // Beacon period (10 ticks)
    beacon.ontime = 5;        // Beacon on-time (5 ticks)

    // Test case 1: Beacon should be drawn (t = 3, within on-time)
    uint64_t t = 3; // Elapsed time
    draw_beacon(framebuffer, t, &beacon);
    
    console_printf("Test Case 1: Beacon should be drawn (t = 3)\n");
    print_framebuffer_region(99, 149, 5, 5); // Print a region around the beacon

    // Reset the framebuffer for the next test
    memset(framebuffer, 0x00, sizeof(framebuffer));

    // Test case 2: Beacon should not be drawn (t = 7, outside on-time)
    t = 7; // Elapsed time
    draw_beacon(framebuffer, t, &beacon);

    console_printf("Test Case 2: Beacon should not be drawn (t = 7)\n");
    print_framebuffer_region(99, 149, 5, 5); // Print a region around the beacon

    // Test case 3: Edge case (partially outside screen bounds)
    memset(framebuffer, 0x00, sizeof(framebuffer)); // Reset framebuffer
    beacon.x = 638; // Close to the right edge
    beacon.y = 478; // Close to the bottom edge

    t = 3; // Within on-time again
    draw_beacon(framebuffer, t, &beacon);

    console_printf("Test Case 3: Beacon partially outside screen bounds\n");
    print_framebuffer_region(637, 477, 5, 5); // Print a region near the bottom-right corner
}