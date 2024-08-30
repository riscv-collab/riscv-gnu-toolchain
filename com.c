#include <stdint.h>

#include "com.h"

#define UART0_BASE 0x10000000
#define UART0_IRQ 10

struct ns16550a_regs {
  union {
    uint8_t rbr; // DLAB=0 read
    uint8_t thr; // DLAB=0 write
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

#define WLS0 (1 << 0)
#define DLAB (1 << 7)
#define DR (1 << 0)
#define THRE (1 << 5)
#define ERBFI (1 << 0)
#define ETBEI (1 << 1)

#define UART0 (*(volatile struct ns16550a_regs*)UART0_BASE)

// 
// EXPORTED FUNCTION DEFINITIONS
// 

void com_init(void) {
  UART0.ier = 0x00;

  // Configure UART
  UART0.lcr = DLAB;
  UART0.dll = 0x01;
  UART0.dlm = 0x00;
  UART0.lcr = 0;
}

void com_putc(char c) {
  // Spin until THR is empty
  while (!(UART0.lsr & THRE))
    continue;

  UART0.thr = c;
}

char com_getc(void) {
  // Spin until data ready
  while (!(UART0.lsr & DR))
    continue;
    
  return UART0.rbr;
}