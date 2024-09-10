// serial.h - NS16550a serial port
//

#ifndef _SERIAL_H_
#define _SERIAL_H_

extern void com0_init(void);
extern void com0_putc(char c);
extern char com0_getc(void);

#endif // _SERIAL_H_