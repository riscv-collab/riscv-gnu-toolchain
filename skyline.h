// skyline.h
//

#ifndef _SKYLINE_H_
#define _SKYLINE_H_

#include <stdint.h>

#define SKYLINE_WIDTH 640
#define SKYLINE_HEIGHT 480
#define SKYLINE_STARS_MAX 1000

struct skyline_star {
    uint16_t x;
    uint16_t y;
    uint8_t dia;
    uint16_t color;
};

struct skyline_window {
    struct skyline_window * next;
    uint16_t x;
    uint16_t y;
    uint8_t w;
    uint8_t h;
    uint16_t color;
};

struct skyline_beacon {
    const uint16_t * img;
    uint16_t x;
    uint16_t y;
    uint8_t dia;
    uint16_t period;
    uint16_t ontime;
};

// The following global variables are defined elsewhere. The skyline module
// (what you write) should not define these itself, but should access them.

extern struct skyline_star skyline_stars[SKYLINE_STARS_MAX];
extern uint16_t skyline_star_cnt;

extern struct skyline_window * skyline_win_list;
extern struct skyline_beacon skyline_beacon;

// skyline_init() is called before any of the functions bellow. You can assume
// that all of the global varaibles aboe have been zero-initialized.

extern void skyline_init(void);

// add_star() adds a start at position (x,y) to the skyline_stars array with the
// specified color and update skyline_star_cnt. If there is no room for the star
// in the array, the request should be ignored (star array remains unchanged).

extern void add_star(uint16_t x, uint16_t y, uint16_t color);

// remove_star() removes the star at location (x,y) from the star array, if such
// a star exists, and should update the star count. You may assume that at most
// one star will be added at each location. Remember that the stars must be
// contiguous in the star array.

extern void remove_star(uint16_t x, uint16_t y);

// draw_star() draws a star to the frame buffer /fbuf/. Do not assume that the
// pointer is to a star in the star array. (During testing, the function may be
// called with a pointer to a star struct that is not in the array.) You may
// assume that the coordinates of the star are inside the screen area. That is,
// you may assume that 0 <= x < SKYLINE_WIDTH and 0 <= y < SKYLINE_HEIGHT.

extern void draw_star(uint16_t * fbuf, const struct skyline_star * star);

// add_window() adds a window of the specified color and size at the specified
// position to the window list. The upper left corner of the window is at the
// (x,y) coordinate. The width and height of the window are given by /w/ and
// /h/. The window color is given by /color/.

extern void add_window (
    uint16_t x, uint16_t y,
    uint8_t w, uint8_t h,
    uint16_t color);

// remove_window() removes a window the upper left corner of which is at
// position (x,y), if such a window exists. You may assume that there is at most
// one window at those coordinates.

extern void remove_window(uint16_t x, uint16_t y);

// draw_window() draws a window to the frame buffer /fbuf/.

extern void draw_window(uint16_t * fbuf, const struct skyline_window * win);

extern void start_beacon (
    const uint16_t * img,
    uint16_t x,
    uint16_t y,
    uint8_t dia,
    uint16_t period,
    uint16_t ontime);

extern void draw_beacon (
    uint16_t * fbuf,
    uint64_t t, // current time
    const struct skyline_beacon * bcn);

#endif // _SKYLINE_H_