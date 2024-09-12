// serial.c - NS16550a serial port
// 

#include "serial.h"

#include <stdint.h>

#include "memory.h"
#include "halt.h"
#include "intr.h"

#ifndef UART0_IOBASE
#define UART0_IOBASE 0x10000000
#endif


// INTERNAL TYPE DEFINITIONS
// 

struct ns16550a_regs {
	union {
		char rbr; // DLAB=0 read
		char thr; // DLAB=0 write
		uint8_t dll; // DLAB=1
	};
	
	union {
		uint8_t ier; // DLAB=0
		uint8_t dlm; // DLAB=1
	};
	
	union {
		uint8_t iir; // read
		uint8_t fcr; // write
	};

	uint8_t lcr;
	uint8_t mcr;
	uint8_t lsr;
	uint8_t msr;
	uint8_t scr;
};

#define LCR_DLAB (1 << 7)
#define LSR_OE (1 << 1)
#define LSR_DR (1 << 0)
#define LSR_THRE (1 << 5)
#define IER_ERBFI (1 << 0)
#define IER_ETBEI (1 << 1)

// The functions below provide polled serial input and output. They are used
// by the console functions to produce output from pritnf().

#define UART0 (*(volatile struct ns16550a_regs*)UART0_IOBASE)

void com0_init(void) {
	UART0.ier = 0x00;

	// Configure UART0. We set the baud rate divisor to 1, the lowest value,
	// for the fastest baud rate. In a physical system, the actual baud rate
	// depends on the attached oscillator frequency. In a virtualized system,
	// it doesn't matter.
	
	UART0.lcr = LCR_DLAB;
	UART0.dll = 0x01;
	UART0.dlm = 0x00;

	// The com0_putc and com0_getc functions assume DLAB=0.

	UART0.lcr = 0;
}

void com0_putc(char c) {
	// Spin until THR is empty
	while (!(UART0.lsr & LSR_THRE))
		continue;

	UART0.thr = c;
}

char com0_getc(void) {
	// Spin until RBR contains a byte
	while (!(UART0.lsr & LSR_DR))
		continue;
	
	return UART0.rbr;
}