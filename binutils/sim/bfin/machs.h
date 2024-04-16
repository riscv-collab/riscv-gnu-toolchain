/* Simulator for Analog Devices Blackfin processors.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef _BFIN_MACHS_H_
#define _BFIN_MACHS_H_

#define CPU_MODEL_NUM(cpu) MODEL_NUM (CPU_MODEL (cpu))

/* XXX: Some of this probably belongs in CPU_MODEL.  */
struct bfin_board_data {
  unsigned int sirev, sirev_valid;
  const char *hw_file;
};

void bfin_model_cpu_init (SIM_DESC, SIM_CPU *);
bu32 bfin_model_get_chipid (SIM_DESC);
bu32 bfin_model_get_dspid (SIM_DESC);
extern const SIM_MACH * const bfin_sim_machs[];

#define BFIN_COREMMR_CEC_BASE		0xFFE02100
#define BFIN_COREMMR_CEC_SIZE		(4 * 5)
#define BFIN_COREMMR_CTIMER_BASE	0xFFE03000
#define BFIN_COREMMR_CTIMER_SIZE	(4 * 4)
#define BFIN_COREMMR_EVT_BASE		0xFFE02000
#define BFIN_COREMMR_EVT_SIZE		(4 * 16)
#define BFIN_COREMMR_JTAG_BASE		0xFFE05000
#define BFIN_COREMMR_JTAG_SIZE		(4 * 3)
#define BFIN_COREMMR_MMU_BASE		0xFFE00000
#define BFIN_COREMMR_MMU_SIZE		0x2000
#define BFIN_COREMMR_PFMON_BASE		0xFFE08000
#define BFIN_COREMMR_PFMON_SIZE		0x108
#define BFIN_COREMMR_TRACE_BASE		0xFFE06000
#define BFIN_COREMMR_TRACE_SIZE		(4 * 65)
#define BFIN_COREMMR_WP_BASE		0xFFE07000
#define BFIN_COREMMR_WP_SIZE		0x204

#define BFIN_MMR_DMA_SIZE		(4 * 16)
#define BFIN_MMR_DMAC0_BASE		0xFFC00C00
#define BFIN_MMR_DMAC1_BASE		0xFFC01C00
#define BFIN_MMR_EBIU_AMC_SIZE		(4 * 3)
#define BF50X_MMR_EBIU_AMC_SIZE		0x28
#define BF54X_MMR_EBIU_AMC_SIZE		(4 * 7)
#define BFIN_MMR_EBIU_DDRC_SIZE		0xb0
#define BFIN_MMR_EBIU_SDC_SIZE		(4 * 4)
#define BFIN_MMR_EMAC_BASE		0xFFC03000
#define BFIN_MMR_EMAC_SIZE		0x200
#define BFIN_MMR_EPPI_SIZE		0x40
#define BFIN_MMR_GPIO_SIZE		(17 * 4)
#define BFIN_MMR_GPIO2_SIZE		(8 * 4)
#define BFIN_MMR_GPTIMER_SIZE		(4 * 4)
#define BFIN_MMR_NFC_SIZE		0x50
/* XXX: Not exactly true; it's two sets of 4 regs near each other:
          0xFFC03600 0x10 - Control
          0xFFC03680 0x10 - Data  */
#define BFIN_MMR_OTP_SIZE		0xa0
#define BFIN_MMR_PINT_SIZE		0x28
#define BFIN_MMR_PLL_BASE		0xFFC00000
#define BFIN_MMR_PLL_SIZE		(4 * 6)
#define BFIN_MMR_PPI_SIZE		(4 * 5)
#define BFIN_MMR_RTC_SIZE		(4 * 6)
#define BFIN_MMR_SIC_BASE		0xFFC00100
#define BFIN_MMR_SIC_SIZE		0x100
#define BFIN_MMR_SPI_SIZE		(4 * 7)
#define BFIN_MMR_TWI_SIZE		0x90
#define BFIN_MMR_WDOG_SIZE		(4 * 3)
#define BFIN_MMR_UART_SIZE		0x30
#define BFIN_MMR_UART2_SIZE		0x30

#endif
