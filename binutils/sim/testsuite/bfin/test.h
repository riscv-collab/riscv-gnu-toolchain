#ifndef __ASSEMBLER__
typedef unsigned long bu32;
typedef long bs32;
typedef unsigned short bu16;
typedef short bs16;
typedef unsigned char bu8;
typedef char bs8;
#define ARRAY_SIZE(a) (sizeof (a) / sizeof ((a)[0]))
#define BFIN_MMR_16(mmr) mmr, __pad_##mmr
#include "test-dma.h"
#else
#define __ADSPBF537__ /* XXX: Hack for .S files.  */
#endif
#ifndef __FDPIC__
#include <blackfin.h>
#endif

/* AZ AN AC0_COPY V_COPY CC AQ RND_MOD AC0 AC1 AV0 AV0S AV1 AV1S V VS */

#define _AZ		(1 << 0)
#define _AN		(1 << 1)
#define _AC0_COPY	(1 << 2)
#define _V_COPY		(1 << 3)
#define _CC		(1 << 5)
#define _AQ		(1 << 6)
#define _RND_MOD	(1 << 8)
#define _AC0		(1 << 12)
#define _AC1		(1 << 13)
#define _AV0		(1 << 16)
#define _AV0S		(1 << 17)
#define _AV1		(1 << 18)
#define _AV1S		(1 << 19)
#define _V		(1 << 24)
#define _VS		(1 << 25)

#define _SET		1
#define _UNSET		0

#define PASS		do { puts ("pass"); _exit (0); } while (0)
#define FAIL		do { puts ("fail"); _exit (1); } while (0)
#define DBG_PASS	do { asm volatile ("outc 'p'; outc 'a'; outc 's'; outc 's'; outc '\n'; hlt;"); } while (1)
#define DBG_FAIL	do { asm volatile ("outc 'f'; outc 'a'; outc 'i'; outc 'l'; outc '\n'; abort;"); } while (1)

#define HI(x) (((x) >> 16) & 0xffff)
#define LO(x) ((x) & 0xffff)

#define INIT_R_REGS(val) init_r_regs val
#define INIT_P_REGS(val) init_p_regs val
#define INIT_B_REGS(val) init_b_regs val
#define INIT_I_REGS(val) init_i_regs val
#define INIT_L_REGS(val) init_l_regs val
#define INIT_M_REGS(val) init_m_regs val
#define include(...)
#define CHECK_INIT_DEF(...) nop;
#define CHECK_INIT(...) nop;
#define CHECKMEM32(...)
#define GEN_INT_INIT(...) nop;

#define LD32_LABEL(reg, sym) loadsym reg, sym
#define LD32(reg, val) imm32 reg, val
#define CHECKREG(reg, val) CHECKREG reg, val
#define CHECKREG_SYM_JUMPLESS(reg, sym, scratch_reg) \
	loadsym scratch_reg, sym; \
	cc = reg == scratch_reg; \
	/* Need to avoid jumping for trace buffer.  */ \
	if !cc jump fail_lvl;
#define CHECKREG_SYM(reg, sym, scratch_reg) \
	loadsym scratch_reg, sym; \
	cc = reg == scratch_reg; \
	if cc jump 9f; \
	dbg_fail; \
9:

#define WR_MMR(mmr, val, mmr_reg, val_reg) \
	imm32 mmr_reg, mmr; \
	imm32 val_reg, val; \
	[mmr_reg] = val_reg;
#define WR_MMR_LABEL(mmr, sym, mmr_reg, sym_reg) \
	loadsym sym_reg, sym; \
	imm32 mmr_reg, mmr; \
	[mmr_reg] = sym_reg;
#define RD_MMR(mmr, mmr_reg, val_reg) \
	imm32 mmr_reg, mmr; \
	val_reg = [mmr_reg];

/* Legacy CPLB bits */
#define CPLB_L1_CACHABLE CPLB_L1_CHBL
#define CPLB_USER_RO CPLB_USER_RD

#define DATA_ADDR_1 0xff800000
#define DATA_ADDR_2 0xff900000
#define DATA_ADDR_3 (DATA_ADDR_1 + 0x2000)

/* The libgloss headers omit these defines.  */
#define EVT_OVERRIDE 0xFFE02100
#define EVT_IMASK IMASK

#define PAGE_SIZE_1K PAGE_SIZE_1KB
#define PAGE_SIZE_4K PAGE_SIZE_4KB
#define PAGE_SIZE_1M PAGE_SIZE_1MB
#define PAGE_SIZE_4M PAGE_SIZE_4MB

#define CPLB_USER_RW (CPLB_USER_RD | CPLB_USER_WR)

#define DMC_AB_SRAM      0x0
#define DMC_AB_CACHE     0xc
#define DMC_ACACHE_BSRAM 0x8

#define CPLB_L1SRAM  (1 << 5)
#define CPLB_DA0ACC  (1 << 6)

#define FAULT_CPLB0   (1 << 0)
#define FAULT_CPLB1   (1 << 1)
#define FAULT_CPLB2   (1 << 2)
#define FAULT_CPLB3   (1 << 3)
#define FAULT_CPLB4   (1 << 4)
#define FAULT_CPLB5   (1 << 5)
#define FAULT_CPLB6   (1 << 6)
#define FAULT_CPLB7   (1 << 7)
#define FAULT_CPLB8   (1 << 8)
#define FAULT_CPLB9   (1 << 9)
#define FAULT_CPLB10  (1 << 10)
#define FAULT_CPLB11  (1 << 11)
#define FAULT_CPLB12  (1 << 12)
#define FAULT_CPLB13  (1 << 13)
#define FAULT_CPLB14  (1 << 14)
#define FAULT_CPLB15  (1 << 15)
#define FAULT_READ    (0 << 16)
#define FAULT_WRITE   (1 << 16)
#define FAULT_USER    (0 << 17)
#define FAULT_SUPV    (1 << 17)
#define FAULT_DAG0    (0 << 18)
#define FAULT_DAG1    (1 << 18)
#define FAULT_ILLADDR (1 << 19)
