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

const uint16_t beacon_img[9] = {
    0x07E0, 0x07E0, 0x07E0,
    0x07E0, 0x07E0, 0x07E0,
    0x07E0, 0x07E0, 0x07E0
};

extern void add_window(uint16_t x, uint16_t y, uint8_t w, uint8_t h, uint16_t color);
extern void draw_beacon(uint16_t *fbuf, uint64_t t, const struct skyline_beacon *bcn);
extern void free_window_list(void);

void main(void) {
    // Initialize the framebuffer to black
    memset(framebuffer, 0x00, sizeof(framebuffer));

    // Step 1: Initialize global variables
    skyline_win_list = NULL;  // Start with an empty window list

    // Step 2: Add a window (optional, if draw_beacon is within a window)
    // For this test, we'll assume draw_beacon draws directly on the framebuffer

    // Step 3: Set up the skyline_beacon structure
    skyline_beacon.img = beacon_img;   // Pointer to beacon image data
    skyline_beacon.x = 100;            // x-coordinate on the framebuffer
    skyline_beacon.y = 100;            // y-coordinate on the framebuffer
    skyline_beacon.dia = 3;            // Diameter (3x3 beacon)
    skyline_beacon.period = 10;        // Period of 10 ticks
    skyline_beacon.ontime = 5;         // On for 5 ticks

    // Step 4: Simulate different time ticks and draw the beacon
    for (uint64_t current_time = 0; current_time < 20; current_time++) {
        // Clear the framebuffer for each tick (optional)
        memset(framebuffer, 0x00, sizeof(framebuffer));

        // Call draw_beacon
        draw_beacon(framebuffer, current_time, &skyline_beacon);

        // Verify the beacon is drawn correctly when it should be on
        if ((current_time % skyline_beacon.period) < skyline_beacon.ontime) {
            // Beacon should be on
            int expected_color = 0x07E0;  // Green
            int y, x;
            int success = 1;

            for (y = 0; y < skyline_beacon.dia; y++) {
                for (x = 0; x < skyline_beacon.dia; x++) {
                    int fb_x = skyline_beacon.x + x;
                    int fb_y = skyline_beacon.y + y;
                    int fb_index = fb_y * FRAMEBUFFER_WIDTH + fb_x;
                    uint16_t pixel_color = framebuffer[fb_index];

                    if (pixel_color != expected_color) {
                        console_printf("Tick %llu: Pixel at (%d, %d) expected 0x%04X, got 0x%04X\n",
                                       current_time, fb_x, fb_y, expected_color, pixel_color);
                        success = 0;
                    }
                }
            }

            if (success) {
                console_printf("Tick %llu: Beacon drawn successfully.\n", current_time);
            } else {
                console_printf("Tick %llu: Beacon drawing failed.\n", current_time);
            }
        } else {
            // Beacon should be off; verify that no pixels are set
            int y, x;
            int success = 1;

            for (y = 0; y < skyline_beacon.dia; y++) {
                for (x = 0; x < skyline_beacon.dia; x++) {
                    int fb_x = skyline_beacon.x + x;
                    int fb_y = skyline_beacon.y + y;
                    int fb_index = fb_y * FRAMEBUFFER_WIDTH + fb_x;
                    uint16_t pixel_color = framebuffer[fb_index];

                    if (pixel_color != 0x0000) {  // Assuming 0x0000 is black
                        console_printf("Tick %llu: Pixel at (%d, %d) expected 0x0000, got 0x%04X\n",
                                       current_time, fb_x, fb_y, pixel_color);
                        success = 0;
                    }
                }
            }

            if (success) {
                console_printf("Tick %llu: Beacon correctly not drawn.\n", current_time);
            } else {
                console_printf("Tick %llu: Beacon should not be drawn, but pixels are set.\n", current_time);
            }
        }
    }
}
