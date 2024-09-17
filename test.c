#include "skyline.h"
#include "console.h"  // Assuming console_printf is defined here
#include "string.h"

#define FRAMEBUFFER_WIDTH 640
#define FRAMEBUFFER_HEIGHT 480

struct skyline_star skyline_stars[SKYLINE_STARS_MAX];
uint16_t skyline_star_cnt;

struct skyline_window * skyline_win_list;
struct skyline_beacon skyline_beacon;

uint16_t framebuffer[FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT];



void main(void) {
    // Initialize the framebuffer
    memset(framebuffer, 0, sizeof(framebuffer));

    // Step 1: Initialize global variables
    skyline_win_list = NULL;  // Start with an empty window list

    // Step 2: Add a window using the add_window function
    // Parameters: x = 100, y = 150, width = 50, height = 30, color = 0x07E0 (Green)
    add_window(100, 150, 50, 30, 0x07E0);

    // Step 3: Retrieve the window from the window list
    struct skyline_window* current_window = skyline_win_list;
    if (current_window == NULL) {
        console_printf("Failed to add window.\n");
    }

    // Step 4: Draw the window onto the framebuffer
    draw_window(framebuffer, current_window);

    // Step 5: Verify that the window has been drawn
    // For testing purposes, we'll check a few pixels within the window boundaries
    int x, y;
    uint16_t expected_color = 0x07E0;
    int pixels_correct = 1;  // Flag to indicate if pixels are correct

    for (y = 0; y < current_window->h; y++) {
        for (x = 0; x < current_window->w; x++) {
            int fb_index = (current_window->y + y) * FRAMEBUFFER_WIDTH + (current_window->x + x);
            if (framebuffer[fb_index] != expected_color) {
                pixels_correct = 0;
                console_printf("Pixel at (%d, %d) has incorrect color 0x%04X\n",
                               current_window->x + x, current_window->y + y, framebuffer[fb_index]);
                break;
            }
        }
        if (!pixels_correct) {
            break;
        }
    }

    // Step 6: Print confirmation
    if (pixels_correct) {
        console_printf("Window drawn successfully.\n");
    } else {
        console_printf("Window drawing failed.\n");
    }
}
