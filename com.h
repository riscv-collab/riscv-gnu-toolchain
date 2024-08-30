#ifndef _COM_H_
#define _COM_H_

extern void com_init(void);
extern void com_putc(char c);
extern char com_getc(void);
extern void com_isr(void);

#endif // _COM_H_