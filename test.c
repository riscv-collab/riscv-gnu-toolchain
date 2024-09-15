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

void main(void){
    // Initialize the framebuffer to a known value (e.g., 0x0000)
    memset(framebuffer, 0x00, sizeof(framebuffer));

    // Define a window with specific properties
    struct skyline_window win;
    win.x = 100;             // X coordinate of upper-left corner
    win.y = 150;             // Y coordinate of upper-left corner
    win.w = 20;              // Width of the window
    win.h = 10;              // Height of the window
    win.color = 0x07E0;      // Green color in RGB565 format

    // Call the draw_window function to draw the window in the framebuffer
    draw_window(framebuffer, &win);

    // Verify if the window was drawn correctly in the framebuffer
    for (int y = win.y; y < win.y + win.h; y++) {
        for (int x = win.x; x < win.x + win.w; x++) {
            uint16_t pixel = framebuffer[y * FRAMEBUFFER_WIDTH + x];
            if (pixel != win.color) {
                console_printf("Error: Unexpected pixel color at (%d, %d): 0x%04x\n", x, y, pixel);
            } else {
                console_printf("Correct pixel color at (%d, %d): 0x%04x\n", x, y, pixel);
            }
        }
    }

    // Print a few pixels outside the window to verify they are unchanged
    console_printf("Pixel at (99, 150): 0x%04x (should be 0x0000)\n", framebuffer[150 * FRAMEBUFFER_WIDTH + 99]);
    console_printf("Pixel at (121, 150): 0x%04x (should be 0x0000)\n", framebuffer[150 * FRAMEBUFFER_WIDTH + 121]);
    console_printf("Pixel at (100, 149): 0x%04x (should be 0x0000)\n", framebuffer[149 * FRAMEBUFFER_WIDTH + 100]);
    console_printf("Pixel at (100, 161): 0x%04x (should be 0x0000)\n", framebuffer[161 * FRAMEBUFFER_WIDTH + 100]);
}