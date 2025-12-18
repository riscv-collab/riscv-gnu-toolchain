/* SPDX-License-Identifier: GPL-2.0-only WITH Linux-syscall-note */
/*
 * Copyright (C) 2012 ARM Ltd.
 * Copyright (C) 2015 Regents of the University of California
 */

#ifndef _ASM_RISCV_BYTEORDER_H
#define _ASM_RISCV_BYTEORDER_H

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#include <linux/byteorder/little_endian.h>
#else
#include <linux/byteorder/big_endian.h>
#endif

#endif /* _ASM_RISCV_BYTEORDER_H */
