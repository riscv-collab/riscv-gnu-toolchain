// vga.h - VGA device
// 

#ifndef _VGA_H_
#define _VGA_H_

#include <stdint.h>

// The configured frame buffer is VGA_WIDTH x VGA_HEIGHT with 16 bits per pixel.
// From most significant to least significant, the bits are: red (5 bits), green
// (6 bits), blue (5 bits).

#define VGA_WIDTH   640
#define VGA_HEIGHT  480

extern void vga_attach(uint64_t cfgaddr, uint16_t * fbuf);


#endif // _VGA_H_