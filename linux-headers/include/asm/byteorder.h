#ifndef _ASM_RISCV_BYTEORDER_H
#define _ASM_RISCV_BYTEORDER_H

#if defined(_RISCVEL)
#include <linux/byteorder/little_endian.h>
#elif defined(_RISCVEB)
#include <linux/byteorder/big_endian.h>
#else
#error "Unknown endianness"
#endif

#endif /* _ASM_RISCV_BYTEORDER_H */
