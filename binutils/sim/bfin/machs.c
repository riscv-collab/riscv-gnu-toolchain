/* Simulator for Analog Devices Blackfin processors.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc. and Mike Frysinger.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <stdlib.h>

#include "sim-main.h"
#include "sim/sim-bfin.h"
#include "bfd.h"

#include "sim-hw.h"
#include "sim-options.h"

#include "devices.h"
#include "arch.h"
#include "dv-bfin_cec.h"
#include "dv-bfin_dmac.h"

static const SIM_MACH bfin_mach;

struct bfin_memory_layout {
  address_word addr, len;
  unsigned mask;	/* see mapmask in sim_core_attach() */
};
struct bfin_dev_layout {
  address_word base, len;
  unsigned int dmac;
  const char *dev;
};
struct bfin_dmac_layout {
  address_word base;
  unsigned int dma_count;
};
struct bfin_port_layout {
  /* Which device this routes to (name/port).  */
  const char *dst, *dst_port;
  /* Which device this routes from (name/port).  */
  const char *src, *src_port;
};
struct bfin_model_data {
  bu32 chipid;
  int model_num;
  const struct bfin_memory_layout *mem;
  size_t mem_count;
  const struct bfin_dev_layout *dev;
  size_t dev_count;
  const struct bfin_dmac_layout *dmac;
  size_t dmac_count;
  const struct bfin_port_layout *port;
  size_t port_count;
};

#define LAYOUT(_addr, _len, _mask) { .addr = _addr, .len = _len, .mask = access_##_mask, }
#define _DEVICE(_base, _len, _dev, _dmac) { .base = _base, .len = _len, .dev = _dev, .dmac = _dmac, }
#define DEVICE(_base, _len, _dev) _DEVICE(_base, _len, _dev, 0)
#define PORT(_dst, _dst_port, _src, _src_port) \
  { \
    .dst = _dst, \
    .dst_port = _dst_port, \
    .src = _src, \
    .src_port = _src_port, \
  }
#define SIC(_s, _ip, _d, _op) PORT("bfin_sic", "int"#_ip"@"#_s, _d, _op)

/* [1] Common sim code can't model exec-only memory.
   http://sourceware.org/ml/gdb/2010-02/msg00047.html */

#define bf000_chipid 0
static const struct bfin_memory_layout bf000_mem[] = {};
static const struct bfin_dev_layout bf000_dev[] = {};
static const struct bfin_dmac_layout bf000_dmac[] = {};
static const struct bfin_port_layout bf000_port[] = {};

#define bf50x_chipid 0x2800
#define bf504_chipid bf50x_chipid
#define bf506_chipid bf50x_chipid
static const struct bfin_memory_layout bf50x_mem[] =
{
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC03200, 0x50, read_write),	/* PORT_MUX stub */
  LAYOUT (0xFFC03800, 0x100, read_write),	/* RSI stub */
  LAYOUT (0xFFC0328C, 0xC, read_write),		/* Flash stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFFA00000, 0x4000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA04000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
#define bf504_mem bf50x_mem
#define bf506_mem bf50x_mem
static const struct bfin_dev_layout bf50x_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00400, BFIN_MMR_UART2_SIZE,     "bfin_uart2@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BF50X_MMR_EBIU_AMC_SIZE, "bfin_ebiu_amc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART2_SIZE,     "bfin_uart2@1"),
  DEVICE (0xFFC03400, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
};
#define bf504_dev bf50x_dev
#define bf506_dev bf50x_dev
static const struct bfin_dmac_layout bf50x_dmac[] =
{
  { BFIN_MMR_DMAC0_BASE, 12, },
};
#define bf504_dmac bf50x_dmac
#define bf506_dmac bf50x_dmac
static const struct bfin_port_layout bf50x_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
/*SIC (0,  1, "bfin_dmac@0",       "stat"),*/
  SIC (0,  2, "bfin_ppi@0",        "stat"),
  SIC (0,  3, "bfin_sport@0",      "stat"),
  SIC (0,  4, "bfin_sport@1",      "stat"),
  SIC (0,  5, "bfin_uart2@0",      "stat"),
  SIC (0,  6, "bfin_uart2@1",      "stat"),
  SIC (0,  7, "bfin_spi@0",        "stat"),
  SIC (0,  8, "bfin_spi@1",        "stat"),
  SIC (0,  9, "bfin_can@0",        "stat"),
  SIC (0, 10, "bfin_rsi@0",        "int0"),
/*SIC (0, 11, reserved),*/
  SIC (0, 12, "bfin_counter@0",    "stat"),
  SIC (0, 13, "bfin_counter@1",    "stat"),
  SIC (0, 14, "bfin_dma@0",        "di"),
  SIC (0, 15, "bfin_dma@1",        "di"),
  SIC (0, 16, "bfin_dma@2",        "di"),
  SIC (0, 17, "bfin_dma@3",        "di"),
  SIC (0, 18, "bfin_dma@4",        "di"),
  SIC (0, 19, "bfin_dma@5",        "di"),
  SIC (0, 20, "bfin_dma@6",        "di"),
  SIC (0, 21, "bfin_dma@7",        "di"),
  SIC (0, 22, "bfin_dma@8",        "di"),
  SIC (0, 23, "bfin_dma@9",        "di"),
  SIC (0, 24, "bfin_dma@10",       "di"),
  SIC (0, 25, "bfin_dma@11",       "di"),
  SIC (0, 26, "bfin_can@0",        "rx"),
  SIC (0, 27, "bfin_can@0",        "tx"),
  SIC (0, 28, "bfin_twi@0",        "stat"),
  SIC (0, 29, "bfin_gpio@5",       "mask_a"),
  SIC (0, 30, "bfin_gpio@5",       "mask_b"),
/*SIC (0, 31, reserved),*/
  SIC (1,  0, "bfin_gptimer@0",    "stat"),
  SIC (1,  1, "bfin_gptimer@1",    "stat"),
  SIC (1,  2, "bfin_gptimer@2",    "stat"),
  SIC (1,  3, "bfin_gptimer@3",    "stat"),
  SIC (1,  4, "bfin_gptimer@4",    "stat"),
  SIC (1,  5, "bfin_gptimer@5",    "stat"),
  SIC (1,  6, "bfin_gptimer@6",    "stat"),
  SIC (1,  7, "bfin_gptimer@7",    "stat"),
  SIC (1,  8, "bfin_gpio@6",       "mask_a"),
  SIC (1,  9, "bfin_gpio@6",       "mask_b"),
  SIC (1, 10, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (1, 10, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (1, 11, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (1, 11, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (1, 12, "bfin_wdog@0",       "gpi"),
  SIC (1, 13, "bfin_gpio@7",       "mask_a"),
  SIC (1, 14, "bfin_gpio@7",       "mask_b"),
  SIC (1, 15, "bfin_acm@0",        "stat"),
  SIC (1, 16, "bfin_acm@1",        "int"),
/*SIC (1, 17, reserved),*/
/*SIC (1, 18, reserved),*/
  SIC (1, 19, "bfin_pwm@0",        "trip"),
  SIC (1, 20, "bfin_pwm@0",        "sync"),
  SIC (1, 21, "bfin_pwm@1",        "trip"),
  SIC (1, 22, "bfin_pwm@1",        "sync"),
  SIC (1, 23, "bfin_rsi@0",        "int1"),
};
#define bf504_port bf50x_port
#define bf506_port bf50x_port

#define bf51x_chipid 0x27e8
#define bf512_chipid bf51x_chipid
#define bf514_chipid bf51x_chipid
#define bf516_chipid bf51x_chipid
#define bf518_chipid bf51x_chipid
static const struct bfin_memory_layout bf51x_mem[] =
{
  LAYOUT (0xFFC00680, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC03200, 0x50, read_write),	/* PORT_MUX stub */
  LAYOUT (0xFFC03800, 0xD0, read_write),	/* RSI stub */
  LAYOUT (0xFFC03FE0, 0x20, read_write),	/* RSI peripheral stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
#define bf512_mem bf51x_mem
#define bf514_mem bf51x_mem
#define bf516_mem bf51x_mem
#define bf518_mem bf51x_mem
static const struct bfin_dev_layout bf512_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1"),
  DEVICE (0xFFC03400, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
  DEVICE (0xFFC03600, BFIN_MMR_OTP_SIZE,       "bfin_otp"),
};
#define bf514_dev bf512_dev
static const struct bfin_dev_layout bf516_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1"),
  DEVICE (0xFFC03000, BFIN_MMR_EMAC_SIZE,      "bfin_emac"),
  DEVICE (0, 0x20, "bfin_emac/eth_phy"),
  DEVICE (0xFFC03400, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
  DEVICE (0xFFC03600, BFIN_MMR_OTP_SIZE,       "bfin_otp"),
};
#define bf518_dev bf516_dev
#define bf512_dmac bf50x_dmac
#define bf514_dmac bf50x_dmac
#define bf516_dmac bf50x_dmac
#define bf518_dmac bf50x_dmac
static const struct bfin_port_layout bf51x_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
/*SIC (0,  1, "bfin_dmac@0",       "stat"),*/
  SIC (0,  2, "bfin_dmar@0",       "block"),
  SIC (0,  3, "bfin_dmar@1",       "block"),
  SIC (0,  4, "bfin_dmar@0",       "overflow"),
  SIC (0,  5, "bfin_dmar@1",       "overflow"),
  SIC (0,  6, "bfin_ppi@0",        "stat"),
  SIC (0,  7, "bfin_emac",         "stat"),
  SIC (0,  8, "bfin_sport@0",      "stat"),
  SIC (0,  9, "bfin_sport@1",      "stat"),
  SIC (0, 10, "bfin_ptp",          "stat"),
/*SIC (0, 11, reserved),*/
  SIC (0, 12, "bfin_uart@0",       "stat"),
  SIC (0, 13, "bfin_uart@1",       "stat"),
  SIC (0, 14, "bfin_rtc",          "rtc"),
  SIC (0, 15, "bfin_dma@0",        "di"),
  SIC (0, 16, "bfin_dma@3",        "di"),
  SIC (0, 17, "bfin_dma@4",        "di"),
  SIC (0, 18, "bfin_dma@5",        "di"),
  SIC (0, 19, "bfin_dma@6",        "di"),
  SIC (0, 20, "bfin_twi@0",        "stat"),
  SIC (0, 21, "bfin_dma@7",        "di"),
  SIC (0, 22, "bfin_dma@8",        "di"),
  SIC (0, 23, "bfin_dma@9",        "di"),
  SIC (0, 24, "bfin_dma@10",       "di"),
  SIC (0, 25, "bfin_dma@11",       "di"),
  SIC (0, 26, "bfin_otp",          "stat"),
  SIC (0, 27, "bfin_counter@0",    "stat"),
  SIC (0, 28, "bfin_dma@1",        "di"),
  SIC (0, 29, "bfin_gpio@7",       "mask_a"),
  SIC (0, 30, "bfin_dma@2",        "di"),
  SIC (0, 31, "bfin_gpio@7",       "mask_b"),
  SIC (1,  0, "bfin_gptimer@0",    "stat"),
  SIC (1,  1, "bfin_gptimer@1",    "stat"),
  SIC (1,  2, "bfin_gptimer@2",    "stat"),
  SIC (1,  3, "bfin_gptimer@3",    "stat"),
  SIC (1,  4, "bfin_gptimer@4",    "stat"),
  SIC (1,  5, "bfin_gptimer@5",    "stat"),
  SIC (1,  6, "bfin_gptimer@6",    "stat"),
  SIC (1,  7, "bfin_gptimer@7",    "stat"),
  SIC (1,  8, "bfin_gpio@6",       "mask_a"),
  SIC (1,  9, "bfin_gpio@6",       "mask_b"),
  SIC (1, 10, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (1, 10, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (1, 11, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (1, 11, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (1, 12, "bfin_wdog@0",       "gpi"),
  SIC (1, 13, "bfin_gpio@5",       "mask_a"),
  SIC (1, 14, "bfin_gpio@5",       "mask_b"),
  SIC (1, 15, "bfin_spi@0",        "stat"),
  SIC (1, 16, "bfin_spi@1",        "stat"),
/*SIC (1, 17, reserved),*/
/*SIC (1, 18, reserved),*/
  SIC (1, 19, "bfin_rsi@0",        "int0"),
  SIC (1, 20, "bfin_rsi@0",        "int1"),
  SIC (1, 21, "bfin_pwm@0",        "trip"),
  SIC (1, 22, "bfin_pwm@0",        "sync"),
  SIC (1, 23, "bfin_ptp",          "stat"),
};
#define bf512_port bf51x_port
#define bf514_port bf51x_port
#define bf516_port bf51x_port
#define bf518_port bf51x_port

#define bf522_chipid 0x27e4
#define bf523_chipid 0x27e0
#define bf524_chipid bf522_chipid
#define bf525_chipid bf523_chipid
#define bf526_chipid bf522_chipid
#define bf527_chipid bf523_chipid
static const struct bfin_memory_layout bf52x_mem[] =
{
  LAYOUT (0xFFC00680, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC03200, 0x50, read_write),	/* PORT_MUX stub */
  LAYOUT (0xFFC03800, 0x500, read_write),	/* MUSB stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
#define bf522_mem bf52x_mem
#define bf523_mem bf52x_mem
#define bf524_mem bf52x_mem
#define bf525_mem bf52x_mem
#define bf526_mem bf52x_mem
#define bf527_mem bf52x_mem
static const struct bfin_dev_layout bf522_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1"),
  DEVICE (0xFFC03600, BFIN_MMR_OTP_SIZE,       "bfin_otp"),
  DEVICE (0xFFC03700, BFIN_MMR_NFC_SIZE,       "bfin_nfc"),
};
#define bf523_dev bf522_dev
#define bf524_dev bf522_dev
#define bf525_dev bf522_dev
static const struct bfin_dev_layout bf526_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1"),
  DEVICE (0xFFC03000, BFIN_MMR_EMAC_SIZE,      "bfin_emac"),
  DEVICE (0, 0x20, "bfin_emac/eth_phy"),
  DEVICE (0xFFC03600, BFIN_MMR_OTP_SIZE,       "bfin_otp"),
  DEVICE (0xFFC03700, BFIN_MMR_NFC_SIZE,       "bfin_nfc"),
};
#define bf527_dev bf526_dev
#define bf522_dmac bf50x_dmac
#define bf523_dmac bf50x_dmac
#define bf524_dmac bf50x_dmac
#define bf525_dmac bf50x_dmac
#define bf526_dmac bf50x_dmac
#define bf527_dmac bf50x_dmac
static const struct bfin_port_layout bf52x_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
/*SIC (0,  1, "bfin_dmac@0",       "stat"),*/
  SIC (0,  2, "bfin_dmar@0",       "block"),
  SIC (0,  3, "bfin_dmar@1",       "block"),
  SIC (0,  4, "bfin_dmar@0",       "overflow"),
  SIC (0,  5, "bfin_dmar@1",       "overflow"),
  SIC (0,  6, "bfin_ppi@0",        "stat"),
  SIC (0,  7, "bfin_emac",         "stat"),
  SIC (0,  8, "bfin_sport@0",      "stat"),
  SIC (0,  9, "bfin_sport@1",      "stat"),
/*SIC (0, 10, reserved),*/
/*SIC (0, 11, reserved),*/
  SIC (0, 12, "bfin_uart@0",       "stat"),
  SIC (0, 13, "bfin_uart@1",       "stat"),
  SIC (0, 14, "bfin_rtc",          "rtc"),
  SIC (0, 15, "bfin_dma@0",        "di"),
  SIC (0, 16, "bfin_dma@3",        "di"),
  SIC (0, 17, "bfin_dma@4",        "di"),
  SIC (0, 18, "bfin_dma@5",        "di"),
  SIC (0, 19, "bfin_dma@6",        "di"),
  SIC (0, 20, "bfin_twi@0",        "stat"),
  SIC (0, 21, "bfin_dma@7",        "di"),
  SIC (0, 22, "bfin_dma@8",        "di"),
  SIC (0, 23, "bfin_dma@9",        "di"),
  SIC (0, 24, "bfin_dma@10",       "di"),
  SIC (0, 25, "bfin_dma@11",       "di"),
  SIC (0, 26, "bfin_otp",          "stat"),
  SIC (0, 27, "bfin_counter@0",    "stat"),
  SIC (0, 28, "bfin_dma@1",        "di"),
  SIC (0, 29, "bfin_gpio@7",       "mask_a"),
  SIC (0, 30, "bfin_dma@2",        "di"),
  SIC (0, 31, "bfin_gpio@7",       "mask_b"),
  SIC (1,  0, "bfin_gptimer@0",    "stat"),
  SIC (1,  1, "bfin_gptimer@1",    "stat"),
  SIC (1,  2, "bfin_gptimer@2",    "stat"),
  SIC (1,  3, "bfin_gptimer@3",    "stat"),
  SIC (1,  4, "bfin_gptimer@4",    "stat"),
  SIC (1,  5, "bfin_gptimer@5",    "stat"),
  SIC (1,  6, "bfin_gptimer@6",    "stat"),
  SIC (1,  7, "bfin_gptimer@7",    "stat"),
  SIC (1,  8, "bfin_gpio@6",       "mask_a"),
  SIC (1,  9, "bfin_gpio@6",       "mask_b"),
  SIC (1, 10, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (1, 10, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (1, 11, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (1, 11, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (1, 12, "bfin_wdog@0",       "gpi"),
  SIC (1, 13, "bfin_gpio@5",       "mask_a"),
  SIC (1, 14, "bfin_gpio@5",       "mask_b"),
  SIC (1, 15, "bfin_spi@0",        "stat"),
  SIC (1, 16, "bfin_nfc",          "stat"),
  SIC (1, 17, "bfin_hostdp",       "stat"),
  SIC (1, 18, "bfin_hostdp",       "done"),
  SIC (1, 20, "bfin_usb",          "int0"),
  SIC (1, 21, "bfin_usb",          "int1"),
  SIC (1, 22, "bfin_usb",          "int2"),
};
#define bf522_port bf52x_port
#define bf523_port bf52x_port
#define bf524_port bf52x_port
#define bf525_port bf52x_port
#define bf526_port bf52x_port
#define bf527_port bf52x_port

#define bf531_chipid 0x27a5
#define bf532_chipid bf531_chipid
#define bf533_chipid bf531_chipid
static const struct bfin_memory_layout bf531_mem[] =
{
  LAYOUT (0xFFC00640, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_memory_layout bf532_mem[] =
{
  LAYOUT (0xFFC00640, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA0C000, 0x4000, read_write_exec),	/* Inst C [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_memory_layout bf533_mem[] =
{
  LAYOUT (0xFFC00640, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA0C000, 0x4000, read_write_exec),	/* Inst C [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_dev_layout bf533_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
};
#define bf531_dev bf533_dev
#define bf532_dev bf533_dev
static const struct bfin_dmac_layout bf533_dmac[] =
{
  { BFIN_MMR_DMAC0_BASE, 8, },
};
#define bf531_dmac bf533_dmac
#define bf532_dmac bf533_dmac
static const struct bfin_port_layout bf533_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
/*SIC (0,  1, "bfin_dmac@0",       "stat"),*/
  SIC (0,  2, "bfin_ppi@0",        "stat"),
  SIC (0,  3, "bfin_sport@0",      "stat"),
  SIC (0,  4, "bfin_sport@1",      "stat"),
  SIC (0,  5, "bfin_spi@0",        "stat"),
  SIC (0,  6, "bfin_uart@0",       "stat"),
  SIC (0,  7, "bfin_rtc",          "rtc"),
  SIC (0,  8, "bfin_dma@0",        "di"),
  SIC (0,  9, "bfin_dma@1",        "di"),
  SIC (0, 10, "bfin_dma@2",        "di"),
  SIC (0, 11, "bfin_dma@3",        "di"),
  SIC (0, 12, "bfin_dma@4",        "di"),
  SIC (0, 13, "bfin_dma@5",        "di"),
  SIC (0, 14, "bfin_dma@6",        "di"),
  SIC (0, 15, "bfin_dma@7",        "di"),
  SIC (0, 16, "bfin_gptimer@0",    "stat"),
  SIC (0, 17, "bfin_gptimer@1",    "stat"),
  SIC (0, 18, "bfin_gptimer@2",    "stat"),
  SIC (0, 19, "bfin_gpio@5",       "mask_a"),
  SIC (0, 20, "bfin_gpio@5",       "mask_b"),
  SIC (0, 21, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (0, 21, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (0, 22, "bfin_dma@258",      "di"),	/* mdma */
  SIC (0, 22, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (0, 23, "bfin_wdog@0",       "gpi"),
};
#define bf531_port bf533_port
#define bf532_port bf533_port

#define bf534_chipid 0x27c6
#define bf536_chipid 0x27c8
#define bf537_chipid bf536_chipid
static const struct bfin_memory_layout bf534_mem[] =
{
  LAYOUT (0xFFC00680, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC03200, 0x10, read_write),	/* PORT_MUX stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_memory_layout bf536_mem[] =
{
  LAYOUT (0xFFC00680, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC03200, 0x10, read_write),	/* PORT_MUX stub */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_memory_layout bf537_mem[] =
{
  LAYOUT (0xFFC00680, 0xC, read_write),		/* TIMER stub */
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC03200, 0x10, read_write),	/* PORT_MUX stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_dev_layout bf534_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1"),
  DEVICE (0, 0, "glue-or@1"),
  DEVICE (0, 0, "glue-or@1/interrupt-ranges 0 5"),
  DEVICE (0, 0, "glue-or@2"),
  DEVICE (0, 0, "glue-or@2/interrupt-ranges 0 8"),
  DEVICE (0, 0, "glue-or@17"),
  DEVICE (0, 0, "glue-or@17/interrupt-ranges 0 2"),
  DEVICE (0, 0, "glue-or@18"),
  DEVICE (0, 0, "glue-or@18/interrupt-ranges 0 2"),
  DEVICE (0, 0, "glue-or@27"),
  DEVICE (0, 0, "glue-or@27/interrupt-ranges 0 2"),
  DEVICE (0, 0, "glue-or@31"),
  DEVICE (0, 0, "glue-or@31/interrupt-ranges 0 2"),
};
static const struct bfin_dev_layout bf537_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1"),
  DEVICE (0xFFC03000, BFIN_MMR_EMAC_SIZE,      "bfin_emac"),
  DEVICE (0, 0x20, "bfin_emac/eth_phy"),
  DEVICE (0, 0, "glue-or@1"),
  DEVICE (0, 0, "glue-or@1/interrupt-ranges 0 5"),
  DEVICE (0, 0, "glue-or@2"),
  DEVICE (0, 0, "glue-or@2/interrupt-ranges 0 8"),
  DEVICE (0, 0, "glue-or@17"),
  DEVICE (0, 0, "glue-or@17/interrupt-ranges 0 2"),
  DEVICE (0, 0, "glue-or@18"),
  DEVICE (0, 0, "glue-or@18/interrupt-ranges 0 2"),
  DEVICE (0, 0, "glue-or@27"),
  DEVICE (0, 0, "glue-or@27/interrupt-ranges 0 2"),
  DEVICE (0, 0, "glue-or@31"),
  DEVICE (0, 0, "glue-or@31/interrupt-ranges 0 2"),
};
#define bf536_dev bf537_dev
#define bf534_dmac bf50x_dmac
#define bf536_dmac bf50x_dmac
#define bf537_dmac bf50x_dmac
static const struct bfin_port_layout bf537_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
  SIC (0,  1, "glue-or@1",         "int"),
/*PORT ("glue-or@1", "int", "bfin_dmac@0",   "stat"),*/
  PORT ("glue-or@1", "int", "bfin_dmar@0",   "block"),
  PORT ("glue-or@1", "int", "bfin_dmar@1",   "block"),
  PORT ("glue-or@1", "int", "bfin_dmar@0",   "overflow"),
  PORT ("glue-or@1", "int", "bfin_dmar@1",   "overflow"),
  SIC (0,  2, "glue-or@2",         "int"),
  PORT ("glue-or@2", "int", "bfin_can@0",    "stat"),
  PORT ("glue-or@2", "int", "bfin_emac",     "stat"),
  PORT ("glue-or@2", "int", "bfin_sport@0",  "stat"),
  PORT ("glue-or@2", "int", "bfin_sport@1",  "stat"),
  PORT ("glue-or@2", "int", "bfin_ppi@0",    "stat"),
  PORT ("glue-or@2", "int", "bfin_spi@0",    "stat"),
  PORT ("glue-or@2", "int", "bfin_uart@0",   "stat"),
  PORT ("glue-or@2", "int", "bfin_uart@1",   "stat"),
  SIC (0,  3, "bfin_rtc",          "rtc"),
  SIC (0,  4, "bfin_dma@0",        "di"),
  SIC (0,  5, "bfin_dma@3",        "di"),
  SIC (0,  6, "bfin_dma@4",        "di"),
  SIC (0,  7, "bfin_dma@5",        "di"),
  SIC (0,  8, "bfin_dma@6",        "di"),
  SIC (0,  9, "bfin_twi@0",        "stat"),
  SIC (0, 10, "bfin_dma@7",        "di"),
  SIC (0, 11, "bfin_dma@8",        "di"),
  SIC (0, 12, "bfin_dma@9",        "di"),
  SIC (0, 13, "bfin_dma@10",       "di"),
  SIC (0, 14, "bfin_dma@11",       "di"),
  SIC (0, 15, "bfin_can@0",        "rx"),
  SIC (0, 16, "bfin_can@0",        "tx"),
  SIC (0, 17, "glue-or@17",        "int"),
  PORT ("glue-or@17", "int", "bfin_dma@1",   "di"),
  PORT ("glue-or@17", "int", "bfin_gpio@7",  "mask_a"),
  SIC (0, 18, "glue-or@18",        "int"),
  PORT ("glue-or@18", "int", "bfin_dma@2",   "di"),
  PORT ("glue-or@18", "int", "bfin_gpio@7",  "mask_b"),
  SIC (0, 19, "bfin_gptimer@0",    "stat"),
  SIC (0, 20, "bfin_gptimer@1",    "stat"),
  SIC (0, 21, "bfin_gptimer@2",    "stat"),
  SIC (0, 22, "bfin_gptimer@3",    "stat"),
  SIC (0, 23, "bfin_gptimer@4",    "stat"),
  SIC (0, 24, "bfin_gptimer@5",    "stat"),
  SIC (0, 25, "bfin_gptimer@6",    "stat"),
  SIC (0, 26, "bfin_gptimer@7",    "stat"),
  SIC (0, 27, "glue-or@27",        "int"),
  PORT ("glue-or@27", "int", "bfin_gpio@5",   "mask_a"),
  PORT ("glue-or@27", "int", "bfin_gpio@6",   "mask_a"),
  SIC (0, 28, "bfin_gpio@6",       "mask_b"),
  SIC (0, 29, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (0, 29, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (0, 30, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (0, 30, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (0, 31, "glue-or@31",        "int"),
  PORT ("glue-or@31", "int", "bfin_wdog@0",   "gpi"),
  PORT ("glue-or@31", "int", "bfin_gpio@5",   "mask_b"),
};
#define bf534_port bf537_port
#define bf536_port bf537_port

#define bf538_chipid 0x27c4
#define bf539_chipid bf538_chipid
static const struct bfin_memory_layout bf538_mem[] =
{
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC01500, 0x70, read_write),	/* PORTC/D/E stub */
  LAYOUT (0xFFC02500, 0x60, read_write),	/* SPORT2 stub */
  LAYOUT (0xFFC02600, 0x60, read_write),	/* SPORT3 stub */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA0C000, 0x4000, read_write_exec),	/* Inst C [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
#define bf539_mem bf538_mem
static const struct bfin_dev_layout bf538_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
 _DEVICE (0xFFC02000, BFIN_MMR_UART_SIZE,      "bfin_uart@1", 1),
 _DEVICE (0xFFC02100, BFIN_MMR_UART_SIZE,      "bfin_uart@2", 1),
  DEVICE (0xFFC02200, BFIN_MMR_TWI_SIZE,       "bfin_twi@1"),
 _DEVICE (0xFFC02300, BFIN_MMR_SPI_SIZE,       "bfin_spi@1", 1),
 _DEVICE (0xFFC02400, BFIN_MMR_SPI_SIZE,       "bfin_spi@2", 1),
};
#define bf539_dev bf538_dev
static const struct bfin_dmac_layout bf538_dmac[] =
{
  { BFIN_MMR_DMAC0_BASE,  8, },
  { BFIN_MMR_DMAC1_BASE, 12, },
};
#define bf539_dmac bf538_dmac
static const struct bfin_port_layout bf538_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
  SIC (0,  1, "bfin_dmac@0",       "stat"),
  SIC (0,  2, "bfin_ppi@0",        "stat"),
  SIC (0,  3, "bfin_sport@0",      "stat"),
  SIC (0,  4, "bfin_sport@1",      "stat"),
  SIC (0,  5, "bfin_spi@0",        "stat"),
  SIC (0,  6, "bfin_uart@0",       "stat"),
  SIC (0,  7, "bfin_rtc",          "rtc"),
  SIC (0,  8, "bfin_dma@0",        "di"),
  SIC (0,  9, "bfin_dma@1",        "di"),
  SIC (0, 10, "bfin_dma@2",        "di"),
  SIC (0, 11, "bfin_dma@3",        "di"),
  SIC (0, 12, "bfin_dma@4",        "di"),
  SIC (0, 13, "bfin_dma@5",        "di"),
  SIC (0, 14, "bfin_dma@6",        "di"),
  SIC (0, 15, "bfin_dma@7",        "di"),
  SIC (0, 16, "bfin_gptimer@0",    "stat"),
  SIC (0, 17, "bfin_gptimer@1",    "stat"),
  SIC (0, 18, "bfin_gptimer@2",    "stat"),
  SIC (0, 19, "bfin_gpio@5",       "mask_a"),
  SIC (0, 20, "bfin_gpio@5",       "mask_b"),
  SIC (0, 21, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (0, 21, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (0, 22, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (0, 22, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (0, 23, "bfin_wdog@0",       "gpi"),
  SIC (0, 24, "bfin_dmac@1",       "stat"),
  SIC (0, 25, "bfin_sport@2",      "stat"),
  SIC (0, 26, "bfin_sport@3",      "stat"),
/*SIC (0, 27, reserved),*/
  SIC (0, 28, "bfin_spi@1",        "stat"),
  SIC (0, 29, "bfin_spi@2",        "stat"),
  SIC (0, 30, "bfin_uart@1",       "stat"),
  SIC (0, 31, "bfin_uart@2",       "stat"),
  SIC (1,  0, "bfin_can@0",        "stat"),
  SIC (1,  1, "bfin_dma@8",        "di"),
  SIC (1,  2, "bfin_dma@9",        "di"),
  SIC (1,  3, "bfin_dma@10",       "di"),
  SIC (1,  4, "bfin_dma@11",       "di"),
  SIC (1,  5, "bfin_dma@12",       "di"),
  SIC (1,  6, "bfin_dma@13",       "di"),
  SIC (1,  7, "bfin_dma@14",       "di"),
  SIC (1,  8, "bfin_dma@15",       "di"),
  SIC (1,  9, "bfin_dma@16",       "di"),
  SIC (1, 10, "bfin_dma@17",       "di"),
  SIC (1, 11, "bfin_dma@18",       "di"),
  SIC (1, 12, "bfin_dma@19",       "di"),
  SIC (1, 13, "bfin_twi@0",        "stat"),
  SIC (1, 14, "bfin_twi@1",        "stat"),
  SIC (1, 15, "bfin_can@0",        "rx"),
  SIC (1, 16, "bfin_can@0",        "tx"),
  SIC (1, 17, "bfin_dma@260",      "di"),	/* mdma2 */
  SIC (1, 17, "bfin_dma@261",      "di"),	/* mdma2 */
  SIC (1, 18, "bfin_dma@262",      "di"),	/* mdma3 */
  SIC (1, 18, "bfin_dma@263",      "di"),	/* mdma3 */
};
#define bf539_port bf538_port

#define bf54x_chipid 0x27de
#define bf542_chipid bf54x_chipid
#define bf544_chipid bf54x_chipid
#define bf547_chipid bf54x_chipid
#define bf548_chipid bf54x_chipid
#define bf549_chipid bf54x_chipid
static const struct bfin_memory_layout bf54x_mem[] =
{
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub XXX: not on BF542/4 */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFFC02500, 0x60, read_write),	/* SPORT2 stub */
  LAYOUT (0xFFC02600, 0x60, read_write),	/* SPORT3 stub */
  LAYOUT (0xFFC03800, 0x70, read_write),	/* ATAPI stub */
  LAYOUT (0xFFC03900, 0x100, read_write),	/* RSI stub */
  LAYOUT (0xFFC03C00, 0x500, read_write),	/* MUSB stub */
  LAYOUT (0xFEB00000, 0x20000, read_write_exec),	/* L2 */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x8000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA08000, 0x4000, read_write_exec),	/* Inst B [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
#define bf542_mem bf54x_mem
#define bf544_mem bf54x_mem
#define bf547_mem bf54x_mem
#define bf548_mem bf54x_mem
#define bf549_mem bf54x_mem
static const struct bfin_dev_layout bf542_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART2_SIZE,     "bfin_uart2@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00700, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC00A00, BF54X_MMR_EBIU_AMC_SIZE, "bfin_ebiu_amc"),
  DEVICE (0xFFC00A20, BFIN_MMR_EBIU_DDRC_SIZE, "bfin_ebiu_ddrc"),
 _DEVICE (0xFFC01300, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@1", 1),
  DEVICE (0xFFC01400, BFIN_MMR_PINT_SIZE,      "bfin_pint@0"),
  DEVICE (0xFFC01430, BFIN_MMR_PINT_SIZE,      "bfin_pint@1"),
 _DEVICE (0xFFC01460, BFIN_MMR_PINT_SIZE,      "bfin_pint@2", 2),
 _DEVICE (0xFFC01490, BFIN_MMR_PINT_SIZE,      "bfin_pint@3", 2),
  DEVICE (0xFFC014C0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@0"),
  DEVICE (0xFFC014E0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@1"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@2"),
  DEVICE (0xFFC01520, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@3"),
  DEVICE (0xFFC01540, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@4"),
  DEVICE (0xFFC01560, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@5"),
  DEVICE (0xFFC01580, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@6"),
  DEVICE (0xFFC015A0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@7"),
  DEVICE (0xFFC015C0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@8"),
  DEVICE (0xFFC015E0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@9"),
  DEVICE (0xFFC01600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC01610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC01620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC01630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC01640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC01650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC01660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC01670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART2_SIZE,     "bfin_uart2@1"),
 _DEVICE (0xFFC02100, BFIN_MMR_UART2_SIZE,     "bfin_uart2@2", 1),
  DEVICE (0xFFC02300, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
 _DEVICE (0xFFC02900, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@2", 1),
 _DEVICE (0xFFC03100, BFIN_MMR_UART2_SIZE,     "bfin_uart2@3", 1),
  DEVICE (0xFFC03B00, BFIN_MMR_NFC_SIZE,       "bfin_nfc"),
  DEVICE (0xFFC04300, BFIN_MMR_OTP_SIZE,       "bfin_otp"),
};
static const struct bfin_dev_layout bf544_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART2_SIZE,     "bfin_uart2@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@8"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@9"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@10"),
  DEVICE (0xFFC00700, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC00A00, BF54X_MMR_EBIU_AMC_SIZE, "bfin_ebiu_amc"),
  DEVICE (0xFFC00A20, BFIN_MMR_EBIU_DDRC_SIZE, "bfin_ebiu_ddrc"),
 _DEVICE (0xFFC01000, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@0", 1),
 _DEVICE (0xFFC01300, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@1", 1),
  DEVICE (0xFFC01400, BFIN_MMR_PINT_SIZE,      "bfin_pint@0"),
  DEVICE (0xFFC01430, BFIN_MMR_PINT_SIZE,      "bfin_pint@1"),
 _DEVICE (0xFFC01460, BFIN_MMR_PINT_SIZE,      "bfin_pint@2", 2),
 _DEVICE (0xFFC01490, BFIN_MMR_PINT_SIZE,      "bfin_pint@3", 2),
  DEVICE (0xFFC014C0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@0"),
  DEVICE (0xFFC014E0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@1"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@2"),
  DEVICE (0xFFC01520, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@3"),
  DEVICE (0xFFC01540, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@4"),
  DEVICE (0xFFC01560, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@5"),
  DEVICE (0xFFC01580, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@6"),
  DEVICE (0xFFC015A0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@7"),
  DEVICE (0xFFC015C0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@8"),
  DEVICE (0xFFC015E0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@9"),
  DEVICE (0xFFC01600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC01610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC01620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC01630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC01640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC01650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC01660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC01670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART2_SIZE,     "bfin_uart2@1"),
 _DEVICE (0xFFC02100, BFIN_MMR_UART2_SIZE,     "bfin_uart2@2", 1),
  DEVICE (0xFFC02200, BFIN_MMR_TWI_SIZE,       "bfin_twi@1"),
  DEVICE (0xFFC02300, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
 _DEVICE (0xFFC02900, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@2", 1),
 _DEVICE (0xFFC03100, BFIN_MMR_UART2_SIZE,     "bfin_uart2@3", 1),
  DEVICE (0xFFC03B00, BFIN_MMR_NFC_SIZE,       "bfin_nfc"),
  DEVICE (0xFFC04300, BFIN_MMR_OTP_SIZE,       "bfin_otp"),
};
static const struct bfin_dev_layout bf547_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00300, BFIN_MMR_RTC_SIZE,       "bfin_rtc"),
  DEVICE (0xFFC00400, BFIN_MMR_UART2_SIZE,     "bfin_uart2@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@8"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@9"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@10"),
  DEVICE (0xFFC00700, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC00A00, BF54X_MMR_EBIU_AMC_SIZE, "bfin_ebiu_amc"),
  DEVICE (0xFFC00A20, BFIN_MMR_EBIU_DDRC_SIZE, "bfin_ebiu_ddrc"),
 _DEVICE (0xFFC01000, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@0", 1),
 _DEVICE (0xFFC01300, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@1", 1),
  DEVICE (0xFFC01400, BFIN_MMR_PINT_SIZE,      "bfin_pint@0"),
  DEVICE (0xFFC01430, BFIN_MMR_PINT_SIZE,      "bfin_pint@1"),
 _DEVICE (0xFFC01460, BFIN_MMR_PINT_SIZE,      "bfin_pint@2", 2),
 _DEVICE (0xFFC01490, BFIN_MMR_PINT_SIZE,      "bfin_pint@3", 2),
  DEVICE (0xFFC014C0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@0"),
  DEVICE (0xFFC014E0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@1"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@2"),
  DEVICE (0xFFC01520, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@3"),
  DEVICE (0xFFC01540, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@4"),
  DEVICE (0xFFC01560, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@5"),
  DEVICE (0xFFC01580, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@6"),
  DEVICE (0xFFC015A0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@7"),
  DEVICE (0xFFC015C0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@8"),
  DEVICE (0xFFC015E0, BFIN_MMR_GPIO2_SIZE,     "bfin_gpio2@9"),
  DEVICE (0xFFC01600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC01610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC01620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC01630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC01640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC01650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC01660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC01670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC02000, BFIN_MMR_UART2_SIZE,     "bfin_uart2@1"),
 _DEVICE (0xFFC02100, BFIN_MMR_UART2_SIZE,     "bfin_uart2@2", 1),
  DEVICE (0xFFC02200, BFIN_MMR_TWI_SIZE,       "bfin_twi@1"),
  DEVICE (0xFFC02300, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
 _DEVICE (0xFFC02400, BFIN_MMR_SPI_SIZE,       "bfin_spi@2", 1),
 _DEVICE (0xFFC02900, BFIN_MMR_EPPI_SIZE,      "bfin_eppi@2", 1),
 _DEVICE (0xFFC03100, BFIN_MMR_UART2_SIZE,     "bfin_uart2@3", 1),
  DEVICE (0xFFC03B00, BFIN_MMR_NFC_SIZE,       "bfin_nfc"),
};
#define bf548_dev bf547_dev
#define bf549_dev bf547_dev
static const struct bfin_dmac_layout bf54x_dmac[] =
{
  { BFIN_MMR_DMAC0_BASE, 12, },
  { BFIN_MMR_DMAC1_BASE, 12, },
};
#define bf542_dmac bf54x_dmac
#define bf544_dmac bf54x_dmac
#define bf547_dmac bf54x_dmac
#define bf548_dmac bf54x_dmac
#define bf549_dmac bf54x_dmac
#define PINT_PIQS(p, b, g) \
  PORT (p, "piq0@"#b,  g, "p0"), \
  PORT (p, "piq1@"#b,  g, "p1"), \
  PORT (p, "piq2@"#b,  g, "p2"), \
  PORT (p, "piq3@"#b,  g, "p3"), \
  PORT (p, "piq4@"#b,  g, "p4"), \
  PORT (p, "piq5@"#b,  g, "p5"), \
  PORT (p, "piq6@"#b,  g, "p6"), \
  PORT (p, "piq7@"#b,  g, "p7"), \
  PORT (p, "piq8@"#b,  g, "p8"), \
  PORT (p, "piq9@"#b,  g, "p9"), \
  PORT (p, "piq10@"#b, g, "p10"), \
  PORT (p, "piq11@"#b, g, "p11"), \
  PORT (p, "piq12@"#b, g, "p12"), \
  PORT (p, "piq13@"#b, g, "p13"), \
  PORT (p, "piq14@"#b, g, "p14"), \
  PORT (p, "piq15@"#b, g, "p15")
static const struct bfin_port_layout bf54x_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
  SIC (0,  1, "bfin_dmac@0",       "stat"),
  SIC (0,  2, "bfin_eppi@0",       "stat"),
  SIC (0,  3, "bfin_sport@0",      "stat"),
  SIC (0,  4, "bfin_sport@1",      "stat"),
  SIC (0,  5, "bfin_spi@0",        "stat"),
  SIC (0,  6, "bfin_uart2@0",      "stat"),
  SIC (0,  7, "bfin_rtc",          "rtc"),
  SIC (0,  8, "bfin_dma@12",       "di"),
  SIC (0,  9, "bfin_dma@0",        "di"),
  SIC (0, 10, "bfin_dma@1",        "di"),
  SIC (0, 11, "bfin_dma@2",        "di"),
  SIC (0, 12, "bfin_dma@3",        "di"),
  SIC (0, 13, "bfin_dma@4",        "di"),
  SIC (0, 14, "bfin_dma@6",        "di"),
  SIC (0, 15, "bfin_dma@7",        "di"),
  SIC (0, 16, "bfin_gptimer@8",    "stat"),
  SIC (0, 17, "bfin_gptimer@9",    "stat"),
  SIC (0, 18, "bfin_gptimer@10",   "stat"),
  SIC (0, 19, "bfin_pint@0",       "stat"),
  PINT_PIQS ("bfin_pint@0", 0, "bfin_gpio2@0"),
  PINT_PIQS ("bfin_pint@0", 1, "bfin_gpio2@1"),
  SIC (0, 20, "bfin_pint@1",       "stat"),
  PINT_PIQS ("bfin_pint@1", 0, "bfin_gpio2@0"),
  PINT_PIQS ("bfin_pint@1", 1, "bfin_gpio2@1"),
  SIC (0, 21, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (0, 21, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (0, 22, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (0, 22, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (0, 23, "bfin_wdog@0",       "gpi"),
  SIC (0, 24, "bfin_dmac@1",       "stat"),
  SIC (0, 25, "bfin_sport@2",      "stat"),
  SIC (0, 26, "bfin_sport@3",      "stat"),
  SIC (0, 27, "bfin_mxvr",         "data"),
  SIC (0, 28, "bfin_spi@1",        "stat"),
  SIC (0, 29, "bfin_spi@2",        "stat"),
  SIC (0, 30, "bfin_uart2@1",      "stat"),
  SIC (0, 31, "bfin_uart2@2",      "stat"),
  SIC (1,  0, "bfin_can@0",        "stat"),
  SIC (1,  1, "bfin_dma@18",       "di"),
  SIC (1,  2, "bfin_dma@19",       "di"),
  SIC (1,  3, "bfin_dma@20",       "di"),
  SIC (1,  4, "bfin_dma@21",       "di"),
  SIC (1,  5, "bfin_dma@13",       "di"),
  SIC (1,  6, "bfin_dma@14",       "di"),
  SIC (1,  7, "bfin_dma@5",        "di"),
  SIC (1,  8, "bfin_dma@23",       "di"),
  SIC (1,  9, "bfin_dma@8",        "di"),
  SIC (1, 10, "bfin_dma@9",        "di"),
  SIC (1, 11, "bfin_dma@10",       "di"),
  SIC (1, 12, "bfin_dma@11",       "di"),
  SIC (1, 13, "bfin_twi@0",        "stat"),
  SIC (1, 14, "bfin_twi@1",        "stat"),
  SIC (1, 15, "bfin_can@0",        "rx"),
  SIC (1, 16, "bfin_can@0",        "tx"),
  SIC (1, 17, "bfin_dma@260",      "di"),	/* mdma2 */
  SIC (1, 17, "bfin_dma@261",      "di"),	/* mdma2 */
  SIC (1, 18, "bfin_dma@262",      "di"),	/* mdma3 */
  SIC (1, 18, "bfin_dma@263",      "di"),	/* mdma3 */
  SIC (1, 19, "bfin_mxvr",         "stat"),
  SIC (1, 20, "bfin_mxvr",         "message"),
  SIC (1, 21, "bfin_mxvr",         "packet"),
  SIC (1, 22, "bfin_eppi@1",       "stat"),
  SIC (1, 23, "bfin_eppi@2",       "stat"),
  SIC (1, 24, "bfin_uart2@3",      "stat"),
  SIC (1, 25, "bfin_hostdp",       "stat"),
/*SIC (1, 26, reserved),*/
  SIC (1, 27, "bfin_pixc",         "stat"),
  SIC (1, 28, "bfin_nfc",          "stat"),
  SIC (1, 29, "bfin_atapi",        "stat"),
  SIC (1, 30, "bfin_can@1",        "stat"),
  SIC (1, 31, "bfin_dmar@0",       "block"),
  SIC (1, 31, "bfin_dmar@1",       "block"),
  SIC (1, 31, "bfin_dmar@0",       "overflow"),
  SIC (1, 31, "bfin_dmar@1",       "overflow"),
  SIC (2,  0, "bfin_dma@15",       "di"),
  SIC (2,  1, "bfin_dma@16",       "di"),
  SIC (2,  2, "bfin_dma@17",       "di"),
  SIC (2,  3, "bfin_dma@22",       "di"),
  SIC (2,  4, "bfin_counter@0",    "stat"),
  SIC (2,  5, "bfin_kpad@0",       "stat"),
  SIC (2,  6, "bfin_can@1",        "rx"),
  SIC (2,  7, "bfin_can@1",        "tx"),
  SIC (2,  8, "bfin_sdh",          "mask0"),
  SIC (2,  9, "bfin_sdh",          "mask1"),
/*SIC (2, 10, reserved),*/
  SIC (2, 11, "bfin_usb",          "int0"),
  SIC (2, 12, "bfin_usb",          "int1"),
  SIC (2, 13, "bfin_usb",          "int2"),
  SIC (2, 14, "bfin_usb",          "dma"),
  SIC (2, 15, "bfin_otp",          "stat"),
/*SIC (2, 16, reserved),*/
/*SIC (2, 17, reserved),*/
/*SIC (2, 18, reserved),*/
/*SIC (2, 19, reserved),*/
/*SIC (2, 20, reserved),*/
/*SIC (2, 21, reserved),*/
  SIC (2, 22, "bfin_gptimer@0",    "stat"),
  SIC (2, 23, "bfin_gptimer@1",    "stat"),
  SIC (2, 24, "bfin_gptimer@2",    "stat"),
  SIC (2, 25, "bfin_gptimer@3",    "stat"),
  SIC (2, 26, "bfin_gptimer@4",    "stat"),
  SIC (2, 27, "bfin_gptimer@5",    "stat"),
  SIC (2, 28, "bfin_gptimer@6",    "stat"),
  SIC (2, 29, "bfin_gptimer@7",    "stat"),
  SIC (2, 30, "bfin_pint@2",       "stat"),
  PINT_PIQS ("bfin_pint@2", 0, "bfin_gpio2@2"),
  PINT_PIQS ("bfin_pint@2", 1, "bfin_gpio2@3"),
  PINT_PIQS ("bfin_pint@2", 2, "bfin_gpio2@4"),
  PINT_PIQS ("bfin_pint@2", 3, "bfin_gpio2@5"),
  PINT_PIQS ("bfin_pint@2", 4, "bfin_gpio2@6"),
  PINT_PIQS ("bfin_pint@2", 5, "bfin_gpio2@7"),
  PINT_PIQS ("bfin_pint@2", 6, "bfin_gpio2@8"),
  PINT_PIQS ("bfin_pint@2", 7, "bfin_gpio2@9"),
  SIC (2, 31, "bfin_pint@3",       "stat"),
  PINT_PIQS ("bfin_pint@3", 0, "bfin_gpio2@2"),
  PINT_PIQS ("bfin_pint@3", 1, "bfin_gpio2@3"),
  PINT_PIQS ("bfin_pint@3", 2, "bfin_gpio2@4"),
  PINT_PIQS ("bfin_pint@3", 3, "bfin_gpio2@5"),
  PINT_PIQS ("bfin_pint@3", 4, "bfin_gpio2@6"),
  PINT_PIQS ("bfin_pint@3", 5, "bfin_gpio2@7"),
  PINT_PIQS ("bfin_pint@3", 6, "bfin_gpio2@8"),
  PINT_PIQS ("bfin_pint@3", 7, "bfin_gpio2@9"),
};
#define bf542_port bf54x_port
#define bf544_port bf54x_port
#define bf547_port bf54x_port
#define bf548_port bf54x_port
#define bf549_port bf54x_port

/* This is only Core A of course ...  */
#define bf561_chipid 0x27bb
static const struct bfin_memory_layout bf561_mem[] =
{
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFEB00000, 0x20000, read_write_exec),	/* L2 */
  LAYOUT (0xFF800000, 0x4000, read_write),	/* Data A */
  LAYOUT (0xFF804000, 0x4000, read_write),	/* Data A Cache */
  LAYOUT (0xFF900000, 0x4000, read_write),	/* Data B */
  LAYOUT (0xFF904000, 0x4000, read_write),	/* Data B Cache */
  LAYOUT (0xFFA00000, 0x4000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA10000, 0x4000, read_write_exec),	/* Inst Cache [1] */
};
static const struct bfin_dev_layout bf561_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@3"),
  DEVICE (0xFFC00640, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@4"),
  DEVICE (0xFFC00650, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@5"),
  DEVICE (0xFFC00660, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@6"),
  DEVICE (0xFFC00670, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@7"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC00A00, BFIN_MMR_EBIU_AMC_SIZE,  "bfin_ebiu_amc"),
  DEVICE (0xFFC00A10, BFIN_MMR_EBIU_SDC_SIZE,  "bfin_ebiu_sdc"),
 _DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0", 1),
  DEVICE (0xFFC01200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@1"),
 _DEVICE (0xFFC01300, BFIN_MMR_PPI_SIZE,       "bfin_ppi@1", 1),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
  DEVICE (0xFFC01600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@8"),
  DEVICE (0xFFC01610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@9"),
  DEVICE (0xFFC01620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@10"),
  DEVICE (0xFFC01630, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@11"),
  DEVICE (0xFFC01700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@7"),
};
static const struct bfin_dmac_layout bf561_dmac[] =
{
  { BFIN_MMR_DMAC0_BASE, 12, },
  { BFIN_MMR_DMAC1_BASE, 12, },
  /* XXX: IMDMA: { 0xFFC01800, 4, }, */
};
static const struct bfin_port_layout bf561_port[] =
{
  /* SIC0 */
  SIC (0,  0, "bfin_pll",          "pll"),
/*SIC (0,  1, "bfin_dmac@0",       "stat"),*/
/*SIC (0,  2, "bfin_dmac@1",       "stat"),*/
/*SIC (0,  3, "bfin_imdmac",       "stat"),*/
  SIC (0,  4, "bfin_ppi@0",        "stat"),
  SIC (0,  5, "bfin_ppi@1",        "stat"),
  SIC (0,  6, "bfin_sport@0",      "stat"),
  SIC (0,  7, "bfin_sport@1",      "stat"),
  SIC (0,  8, "bfin_spi@0",        "stat"),
  SIC (0,  9, "bfin_uart@0",       "stat"),
/*SIC (0, 10, reserved),*/
  SIC (0, 11, "bfin_dma@12",       "di"),
  SIC (0, 12, "bfin_dma@13",       "di"),
  SIC (0, 13, "bfin_dma@14",       "di"),
  SIC (0, 14, "bfin_dma@15",       "di"),
  SIC (0, 15, "bfin_dma@16",       "di"),
  SIC (0, 16, "bfin_dma@17",       "di"),
  SIC (0, 17, "bfin_dma@18",       "di"),
  SIC (0, 18, "bfin_dma@19",       "di"),
  SIC (0, 19, "bfin_dma@20",       "di"),
  SIC (0, 20, "bfin_dma@21",       "di"),
  SIC (0, 21, "bfin_dma@22",       "di"),
  SIC (0, 22, "bfin_dma@23",       "di"),
  SIC (0, 23, "bfin_dma@0",        "di"),
  SIC (0, 24, "bfin_dma@1",        "di"),
  SIC (0, 25, "bfin_dma@2",        "di"),
  SIC (0, 26, "bfin_dma@3",        "di"),
  SIC (0, 27, "bfin_dma@4",        "di"),
  SIC (0, 28, "bfin_dma@5",        "di"),
  SIC (0, 29, "bfin_dma@6",        "di"),
  SIC (0, 30, "bfin_dma@7",        "di"),
  SIC (0, 31, "bfin_dma@8",        "di"),
  SIC (1,  0, "bfin_dma@9",        "di"),
  SIC (1,  1, "bfin_dma@10",       "di"),
  SIC (1,  2, "bfin_dma@11",       "di"),
  SIC (1,  3, "bfin_gptimer@0",    "stat"),
  SIC (1,  4, "bfin_gptimer@1",    "stat"),
  SIC (1,  5, "bfin_gptimer@2",    "stat"),
  SIC (1,  6, "bfin_gptimer@3",    "stat"),
  SIC (1,  7, "bfin_gptimer@4",    "stat"),
  SIC (1,  8, "bfin_gptimer@5",    "stat"),
  SIC (1,  9, "bfin_gptimer@6",    "stat"),
  SIC (1, 10, "bfin_gptimer@7",    "stat"),
  SIC (1, 11, "bfin_gptimer@8",    "stat"),
  SIC (1, 12, "bfin_gptimer@9",    "stat"),
  SIC (1, 13, "bfin_gptimer@10",   "stat"),
  SIC (1, 14, "bfin_gptimer@11",   "stat"),
  SIC (1, 15, "bfin_gpio@5",       "mask_a"),
  SIC (1, 16, "bfin_gpio@5",       "mask_b"),
  SIC (1, 17, "bfin_gpio@6",       "mask_a"),
  SIC (1, 18, "bfin_gpio@6",       "mask_b"),
  SIC (1, 19, "bfin_gpio@7",       "mask_a"),
  SIC (1, 20, "bfin_gpio@7",       "mask_b"),
  SIC (1, 21, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (1, 21, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (1, 22, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (1, 22, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (1, 23, "bfin_dma@260",      "di"),	/* mdma2 */
  SIC (1, 23, "bfin_dma@261",      "di"),	/* mdma2 */
  SIC (1, 24, "bfin_dma@262",      "di"),	/* mdma3 */
  SIC (1, 24, "bfin_dma@263",      "di"),	/* mdma3 */
  SIC (1, 25, "bfin_imdma@0",      "di"),
  SIC (1, 26, "bfin_imdma@1",      "di"),
  SIC (1, 27, "bfin_wdog@0",       "gpi"),
  SIC (1, 27, "bfin_wdog@1",       "gpi"),
/*SIC (1, 28, reserved),*/
/*SIC (1, 29, reserved),*/
  SIC (1, 30, "bfin_sic",          "sup_irq@0"),
  SIC (1, 31, "bfin_sic",          "sup_irq@1"),
};

#define bf592_chipid 0x20cb
static const struct bfin_memory_layout bf592_mem[] =
{
  LAYOUT (0xFFC00800, 0x60, read_write),	/* SPORT0 stub */
  LAYOUT (0xFFC00900, 0x60, read_write),	/* SPORT1 stub */
  LAYOUT (0xFF800000, 0x8000, read_write),	/* Data A */
  LAYOUT (0xFFA00000, 0x4000, read_write_exec),	/* Inst A [1] */
  LAYOUT (0xFFA04000, 0x4000, read_write_exec),	/* Inst B [1] */
};
static const struct bfin_dev_layout bf592_dev[] =
{
  DEVICE (0xFFC00200, BFIN_MMR_WDOG_SIZE,      "bfin_wdog@0"),
  DEVICE (0xFFC00400, BFIN_MMR_UART_SIZE,      "bfin_uart@0"),
  DEVICE (0xFFC00500, BFIN_MMR_SPI_SIZE,       "bfin_spi@0"),
  DEVICE (0xFFC00600, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@0"),
  DEVICE (0xFFC00610, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@1"),
  DEVICE (0xFFC00620, BFIN_MMR_GPTIMER_SIZE,   "bfin_gptimer@2"),
  DEVICE (0xFFC00700, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@5"),
  DEVICE (0xFFC01000, BFIN_MMR_PPI_SIZE,       "bfin_ppi@0"),
  DEVICE (0xFFC01300, BFIN_MMR_SPI_SIZE,       "bfin_spi@1"),
  DEVICE (0xFFC01400, BFIN_MMR_TWI_SIZE,       "bfin_twi@0"),
  DEVICE (0xFFC01500, BFIN_MMR_GPIO_SIZE,      "bfin_gpio@6"),
};
static const struct bfin_dmac_layout bf592_dmac[] =
{
  /* XXX: there are only 9 channels, but mdma code below assumes that they
          start right after the dma channels ... */
  { BFIN_MMR_DMAC0_BASE, 12, },
};
static const struct bfin_port_layout bf592_port[] =
{
  SIC (0,  0, "bfin_pll",          "pll"),
/*SIC (0,  1, "bfin_dmac@0",       "stat"),*/
  SIC (0,  2, "bfin_ppi@0",        "stat"),
  SIC (0,  3, "bfin_sport@0",      "stat"),
  SIC (0,  4, "bfin_sport@1",      "stat"),
  SIC (0,  5, "bfin_spi@0",        "stat"),
  SIC (0,  6, "bfin_spi@1",        "stat"),
  SIC (0,  7, "bfin_uart@0",       "stat"),
  SIC (0,  8, "bfin_dma@0",        "di"),
  SIC (0,  9, "bfin_dma@1",        "di"),
  SIC (0, 10, "bfin_dma@2",        "di"),
  SIC (0, 11, "bfin_dma@3",        "di"),
  SIC (0, 12, "bfin_dma@4",        "di"),
  SIC (0, 13, "bfin_dma@5",        "di"),
  SIC (0, 14, "bfin_dma@6",        "di"),
  SIC (0, 15, "bfin_dma@7",        "di"),
  SIC (0, 16, "bfin_dma@8",        "di"),
  SIC (0, 17, "bfin_gpio@5",       "mask_a"),
  SIC (0, 18, "bfin_gpio@5",       "mask_b"),
  SIC (0, 19, "bfin_gptimer@0",    "stat"),
  SIC (0, 20, "bfin_gptimer@1",    "stat"),
  SIC (0, 21, "bfin_gptimer@2",    "stat"),
  SIC (0, 22, "bfin_gpio@6",       "mask_a"),
  SIC (0, 23, "bfin_gpio@6",       "mask_b"),
  SIC (0, 24, "bfin_twi@0",        "stat"),
/* XXX: 25 - 28 are supposed to be reserved; see comment in machs.c:bf592_dmac[]  */
  SIC (0, 25, "bfin_dma@9",        "di"),
  SIC (0, 26, "bfin_dma@10",       "di"),
  SIC (0, 27, "bfin_dma@11",       "di"),
  SIC (0, 28, "bfin_dma@12",       "di"),
/*SIC (0, 25, reserved),*/
/*SIC (0, 26, reserved),*/
/*SIC (0, 27, reserved),*/
/*SIC (0, 28, reserved),*/
  SIC (0, 29, "bfin_dma@256",      "di"),	/* mdma0 */
  SIC (0, 29, "bfin_dma@257",      "di"),	/* mdma0 */
  SIC (0, 30, "bfin_dma@258",      "di"),	/* mdma1 */
  SIC (0, 30, "bfin_dma@259",      "di"),	/* mdma1 */
  SIC (0, 31, "bfin_wdog",         "gpi"),
};

static const struct bfin_model_data bfin_model_data[] =
{
#define P(n) \
  [MODEL_BF##n] = { \
    bf##n##_chipid, n, \
    bf##n##_mem , ARRAY_SIZE (bf##n##_mem ), \
    bf##n##_dev , ARRAY_SIZE (bf##n##_dev ), \
    bf##n##_dmac, ARRAY_SIZE (bf##n##_dmac), \
    bf##n##_port, ARRAY_SIZE (bf##n##_port), \
  },
#include "proc_list.def"
#undef P
};

#define CORE_DEVICE(dev, DEV) \
  DEVICE (BFIN_COREMMR_##DEV##_BASE, BFIN_COREMMR_##DEV##_SIZE, "bfin_"#dev)
static const struct bfin_dev_layout bfin_core_dev[] =
{
  CORE_DEVICE (cec, CEC),
  CORE_DEVICE (ctimer, CTIMER),
  CORE_DEVICE (evt, EVT),
  CORE_DEVICE (jtag, JTAG),
  CORE_DEVICE (mmu, MMU),
  CORE_DEVICE (pfmon, PFMON),
  CORE_DEVICE (trace, TRACE),
  CORE_DEVICE (wp, WP),
};

static void
dv_bfin_hw_port_parse (SIM_DESC sd, const struct bfin_model_data *mdata,
		       const char *dev)
{
  size_t i;
  const char *sdev;

  sdev = strchr (dev, '/');
  if (sdev)
    ++sdev;
  else
    sdev = dev;

  for (i = 0; i < mdata->port_count; ++i)
    {
      const struct bfin_port_layout *port = &mdata->port[i];

      /* There might be more than one mapping.  */
      if (!strcmp (sdev, port->src))
	sim_hw_parse (sd, "/core/%s > %s %s /core/%s", dev,
		      port->src_port, port->dst_port, port->dst);
    }
}

#define dv_bfin_hw_parse(sd, dv, DV) \
  do { \
    bu32 base = BFIN_MMR_##DV##_BASE; \
    bu32 size = BFIN_MMR_##DV##_SIZE; \
    sim_hw_parse (sd, "/core/bfin_"#dv"/reg %#x %i", base, size); \
    sim_hw_parse (sd, "/core/bfin_"#dv"/type %i",  mdata->model_num); \
    dv_bfin_hw_port_parse (sd, mdata, "bfin_"#dv); \
  } while (0)

static void
bfin_model_hw_tree_init (SIM_DESC sd, SIM_CPU *cpu)
{
  const SIM_MODEL *model = CPU_MODEL (cpu);
  const struct bfin_model_data *mdata = CPU_MODEL_DATA (cpu);
  const struct bfin_board_data *board = STATE_BOARD_DATA (sd);
  int mnum = MODEL_NUM (model);
  unsigned i, j, dma_chan;

  /* Map the core devices.  */
  for (i = 0; i < ARRAY_SIZE (bfin_core_dev); ++i)
    {
      const struct bfin_dev_layout *dev = &bfin_core_dev[i];
      sim_hw_parse (sd, "/core/%s/reg %#x %i", dev->dev, dev->base, dev->len);
    }
  sim_hw_parse (sd, "/core/bfin_ctimer > ivtmr ivtmr /core/bfin_cec");

  if (mnum == MODEL_BF000)
    goto done;

  /* Map the system devices.  */
  dv_bfin_hw_parse (sd, sic, SIC);
  for (i = 7; i < 16; ++i)
    sim_hw_parse (sd, "/core/bfin_sic > ivg%i ivg%i /core/bfin_cec", i, i);

  dv_bfin_hw_parse (sd, pll, PLL);

  dma_chan = 0;
  for (i = 0; i < mdata->dmac_count; ++i)
    {
      const struct bfin_dmac_layout *dmac = &mdata->dmac[i];

      sim_hw_parse (sd, "/core/bfin_dmac@%u/type %i", i, mdata->model_num);

      /* Hook up the non-mdma channels.  */
      for (j = 0; j < dmac->dma_count; ++j)
	{
	  char dev[64];

	  sprintf (dev, "bfin_dmac@%u/bfin_dma@%u", i, dma_chan);
	  sim_hw_parse (sd, "/core/%s/reg %#x %i", dev,
			dmac->base + j * BFIN_MMR_DMA_SIZE, BFIN_MMR_DMA_SIZE);
	  dv_bfin_hw_port_parse (sd, mdata, dev);

	  ++dma_chan;
	}

      /* Hook up the mdma channels -- assume every DMAC has 4.  */
      for (j = 0; j < 4; ++j)
	{
	  char dev[64];

	  sprintf (dev, "bfin_dmac@%u/bfin_dma@%u", i, j + BFIN_DMAC_MDMA_BASE);
	  sim_hw_parse (sd, "/core/%s/reg %#x %i", dev,
			dmac->base + (j + dmac->dma_count) * BFIN_MMR_DMA_SIZE,
			BFIN_MMR_DMA_SIZE);
	  dv_bfin_hw_port_parse (sd, mdata, dev);
	}
    }

  for (i = 0; i < mdata->dev_count; ++i)
    {
      const struct bfin_dev_layout *dev = &mdata->dev[i];

      if (dev->len)
	{
	  sim_hw_parse (sd, "/core/%s/reg %#x %i", dev->dev, dev->base, dev->len);
	  sim_hw_parse (sd, "/core/%s/type %i", dev->dev, mdata->model_num);
	}
      else
	{
	  sim_hw_parse (sd, "/core/%s", dev->dev);
	}

      dv_bfin_hw_port_parse (sd, mdata, dev->dev);
      if (strchr (dev->dev, '/'))
	continue;

      if (!strncmp (dev->dev, "bfin_uart", 9)
	  || !strncmp (dev->dev, "bfin_emac", 9)
	  || !strncmp (dev->dev, "bfin_sport", 10))
	{
	  const char *sint = dev->dev + 5;
	  sim_hw_parse (sd, "/core/%s > tx   %s_tx   /core/bfin_dmac@%u", dev->dev, sint, dev->dmac);
	  sim_hw_parse (sd, "/core/%s > rx   %s_rx   /core/bfin_dmac@%u", dev->dev, sint, dev->dmac);
	}
      else if (!strncmp (dev->dev, "bfin_wdog", 9))
	{
	  sim_hw_parse (sd, "/core/%s > reset rst  /core/bfin_cec", dev->dev);
	  sim_hw_parse (sd, "/core/%s > nmi   nmi  /core/bfin_cec", dev->dev);
	}
    }

 done:
  /* Add any additional user board content.  */
  if (board->hw_file)
    sim_do_commandf (sd, "hw-file %s", board->hw_file);

  /* Trigger all the new devices' finish func.  */
  hw_tree_finish (dv_get_device (cpu, "/"));
}

#include "bfroms/all.h"

struct bfrom {
  bu32 addr, len, alias_len;
  int sirev;
  const char *buf;
};

#define BFROMA(addr, rom, sirev, alias_len) \
  { addr, sizeof (bfrom_bf##rom##_0_##sirev), alias_len, \
    sirev, bfrom_bf##rom##_0_##sirev, }
#define BFROM(rom, sirev, alias_len) BFROMA (0xef000000, rom, sirev, alias_len)
#define BFROM_STUB { 0, 0, 0, 0, NULL, }
static const struct bfrom bf50x_roms[] =
{
  BFROM (50x, 0, 0x1000000),
  BFROM_STUB,
};
static const struct bfrom bf51x_roms[] =
{
  BFROM (51x, 2, 0x1000000),
  BFROM (51x, 1, 0x1000000),
  BFROM (51x, 0, 0x1000000),
  BFROM_STUB,
};
static const struct bfrom bf526_roms[] =
{
  BFROM (526, 2, 0x1000000),
  BFROM (526, 1, 0x1000000),
  BFROM (526, 0, 0x1000000),
  BFROM_STUB,
};
static const struct bfrom bf527_roms[] =
{
  BFROM (527, 2, 0x1000000),
  BFROM (527, 1, 0x1000000),
  BFROM (527, 0, 0x1000000),
  BFROM_STUB,
};
static const struct bfrom bf533_roms[] =
{
  BFROM (533, 6, 0x1000000),
  BFROM (533, 5, 0x1000000),
  BFROM (533, 4, 0x1000000),
  BFROM (533, 3, 0x1000000),
  BFROM (533, 2, 0x1000000),
  BFROM (533, 1, 0x1000000),
  BFROM_STUB,
};
static const struct bfrom bf537_roms[] =
{
  BFROM (537, 3, 0x100000),
  BFROM (537, 2, 0x100000),
  BFROM (537, 1, 0x100000),
  BFROM (537, 0, 0x100000),
  BFROM_STUB,
};
static const struct bfrom bf538_roms[] =
{
  BFROM (538, 5, 0x1000000),
  BFROM (538, 4, 0x1000000),
  BFROM (538, 3, 0x1000000),
  BFROM (538, 2, 0x1000000),
  BFROM (538, 1, 0x1000000),
  BFROM (538, 0, 0x1000000),
  BFROM_STUB,
};
static const struct bfrom bf54x_roms[] =
{
  BFROM (54x, 4, 0x1000),
  BFROM (54x, 2, 0x1000),
  BFROM (54x, 1, 0x1000),
  BFROM (54x, 0, 0x1000),
  BFROMA (0xffa14000, 54x_l1, 4, 0x10000),
  BFROMA (0xffa14000, 54x_l1, 2, 0x10000),
  BFROMA (0xffa14000, 54x_l1, 1, 0x10000),
  BFROMA (0xffa14000, 54x_l1, 0, 0x10000),
  BFROM_STUB,
};
static const struct bfrom bf561_roms[] =
{
  /* XXX: No idea what the actual wrap limit is here.  */
  BFROM (561, 5, 0x1000),
  BFROM_STUB,
};
static const struct bfrom bf59x_roms[] =
{
  BFROM (59x, 1, 0x1000000),
  BFROM (59x, 0, 0x1000000),
  BFROMA (0xffa10000, 59x_l1, 1, 0x10000),
  BFROM_STUB,
};

static void
bfin_model_map_bfrom (SIM_DESC sd, SIM_CPU *cpu)
{
  const struct bfin_model_data *mdata = CPU_MODEL_DATA (cpu);
  const struct bfin_board_data *board = STATE_BOARD_DATA (sd);
  int mnum = mdata->model_num;
  const struct bfrom *bfrom;
  unsigned int sirev;

  if (mnum >= 500 && mnum <= 509)
    bfrom = bf50x_roms;
  else if (mnum >= 510 && mnum <= 519)
    bfrom = bf51x_roms;
  else if (mnum >= 520 && mnum <= 529)
    bfrom = (mnum & 1) ? bf527_roms : bf526_roms;
  else if (mnum >= 531 && mnum <= 533)
    bfrom = bf533_roms;
  else if (mnum == 535)
    return; /* Stub.  */
  else if (mnum >= 534 && mnum <= 537)
    bfrom = bf537_roms;
  else if (mnum >= 538 && mnum <= 539)
    bfrom = bf538_roms;
  else if (mnum >= 540 && mnum <= 549)
    bfrom = bf54x_roms;
  else if (mnum == 561)
    bfrom = bf561_roms;
  else if (mnum >= 590 && mnum <= 599)
    bfrom = bf59x_roms;
  else
    return;

  if (board->sirev_valid)
    sirev = board->sirev;
  else
    sirev = bfrom->sirev;
  while (bfrom->buf)
    {
      /* Map all the ranges for this model/sirev.  */
      if (bfrom->sirev == sirev)
        sim_core_attach (sd, NULL, 0, access_read_exec, 0, bfrom->addr,
			 bfrom->alias_len ? : bfrom->len, bfrom->len, NULL,
			 (char *)bfrom->buf);
      ++bfrom;
    }
}

void
bfin_model_cpu_init (SIM_DESC sd, SIM_CPU *cpu)
{
  const SIM_MODEL *model = CPU_MODEL (cpu);
  const struct bfin_model_data *mdata = CPU_MODEL_DATA (cpu);
  int mnum = MODEL_NUM (model);
  size_t idx;

  /* These memory maps are supposed to be cpu-specific, but the common sim
     code does not yet allow that (2nd arg is "cpu" rather than "NULL".  */
  sim_core_attach (sd, NULL, 0, access_read_write, 0, BFIN_L1_SRAM_SCRATCH,
		   BFIN_L1_SRAM_SCRATCH_SIZE, 0, NULL, NULL);

  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) != OPERATING_ENVIRONMENT)
    return;

  if (mnum == MODEL_BF000)
    goto core_only;

  /* Map in the on-chip memories (SRAMs).  */
  mdata = &bfin_model_data[MODEL_NUM (model)];
  for (idx = 0; idx < mdata->mem_count; ++idx)
    {
      const struct bfin_memory_layout *mem = &mdata->mem[idx];
      sim_core_attach (sd, NULL, 0, mem->mask, 0, mem->addr,
		       mem->len, 0, NULL, NULL);
    }

  /* Map the on-chip ROMs.  */
  bfin_model_map_bfrom (sd, cpu);

 core_only:
  /* Finally, build up the tree for this cpu model.  */
  bfin_model_hw_tree_init (sd, cpu);
}

bu32
bfin_model_get_chipid (SIM_DESC sd)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  const struct bfin_model_data *mdata = CPU_MODEL_DATA (cpu);
  const struct bfin_board_data *board = STATE_BOARD_DATA (sd);
  return
	 (board->sirev << 28) |
	 (mdata->chipid << 12) |
	 (((0xE5 << 1) | 1) & 0xFF);
}

bu32
bfin_model_get_dspid (SIM_DESC sd)
{
  const struct bfin_board_data *board = STATE_BOARD_DATA (sd);
  return
	 (0xE5 << 24) |
	 (0x04 << 16) |
	 (board->sirev);
}

static void
bfin_model_init (SIM_CPU *cpu)
{
  CPU_MODEL_DATA (cpu) = (void *) &bfin_model_data[MODEL_NUM (CPU_MODEL (cpu))];
}

static bu32
bfin_extract_unsigned_integer (const unsigned char *addr, int len)
{
  bu32 retval;
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  retval = 0;

  for (p = endaddr; p > startaddr;)
    retval = (retval << 8) | *--p;

  return retval;
}

static void
bfin_store_unsigned_integer (unsigned char *addr, int len, bu32 val)
{
  unsigned char *p;
  unsigned char *startaddr = addr;
  unsigned char *endaddr = startaddr + len;

  for (p = startaddr; p < endaddr;)
    {
      *p++ = val & 0xff;
      val >>= 8;
    }
}

static bu32 *
bfin_get_reg (SIM_CPU *cpu, int rn)
{
  switch (rn)
    {
    case SIM_BFIN_R0_REGNUM: return &DREG (0);
    case SIM_BFIN_R1_REGNUM: return &DREG (1);
    case SIM_BFIN_R2_REGNUM: return &DREG (2);
    case SIM_BFIN_R3_REGNUM: return &DREG (3);
    case SIM_BFIN_R4_REGNUM: return &DREG (4);
    case SIM_BFIN_R5_REGNUM: return &DREG (5);
    case SIM_BFIN_R6_REGNUM: return &DREG (6);
    case SIM_BFIN_R7_REGNUM: return &DREG (7);
    case SIM_BFIN_P0_REGNUM: return &PREG (0);
    case SIM_BFIN_P1_REGNUM: return &PREG (1);
    case SIM_BFIN_P2_REGNUM: return &PREG (2);
    case SIM_BFIN_P3_REGNUM: return &PREG (3);
    case SIM_BFIN_P4_REGNUM: return &PREG (4);
    case SIM_BFIN_P5_REGNUM: return &PREG (5);
    case SIM_BFIN_SP_REGNUM: return &SPREG;
    case SIM_BFIN_FP_REGNUM: return &FPREG;
    case SIM_BFIN_I0_REGNUM: return &IREG (0);
    case SIM_BFIN_I1_REGNUM: return &IREG (1);
    case SIM_BFIN_I2_REGNUM: return &IREG (2);
    case SIM_BFIN_I3_REGNUM: return &IREG (3);
    case SIM_BFIN_M0_REGNUM: return &MREG (0);
    case SIM_BFIN_M1_REGNUM: return &MREG (1);
    case SIM_BFIN_M2_REGNUM: return &MREG (2);
    case SIM_BFIN_M3_REGNUM: return &MREG (3);
    case SIM_BFIN_B0_REGNUM: return &BREG (0);
    case SIM_BFIN_B1_REGNUM: return &BREG (1);
    case SIM_BFIN_B2_REGNUM: return &BREG (2);
    case SIM_BFIN_B3_REGNUM: return &BREG (3);
    case SIM_BFIN_L0_REGNUM: return &LREG (0);
    case SIM_BFIN_L1_REGNUM: return &LREG (1);
    case SIM_BFIN_L2_REGNUM: return &LREG (2);
    case SIM_BFIN_L3_REGNUM: return &LREG (3);
    case SIM_BFIN_RETS_REGNUM: return &RETSREG;
    case SIM_BFIN_A0_DOT_X_REGNUM: return &AXREG (0);
    case SIM_BFIN_A0_DOT_W_REGNUM: return &AWREG (0);
    case SIM_BFIN_A1_DOT_X_REGNUM: return &AXREG (1);
    case SIM_BFIN_A1_DOT_W_REGNUM: return &AWREG (1);
    case SIM_BFIN_LC0_REGNUM: return &LCREG (0);
    case SIM_BFIN_LT0_REGNUM: return &LTREG (0);
    case SIM_BFIN_LB0_REGNUM: return &LBREG (0);
    case SIM_BFIN_LC1_REGNUM: return &LCREG (1);
    case SIM_BFIN_LT1_REGNUM: return &LTREG (1);
    case SIM_BFIN_LB1_REGNUM: return &LBREG (1);
    case SIM_BFIN_CYCLES_REGNUM: return &CYCLESREG;
    case SIM_BFIN_CYCLES2_REGNUM: return &CYCLES2REG;
    case SIM_BFIN_USP_REGNUM: return &USPREG;
    case SIM_BFIN_SEQSTAT_REGNUM: return &SEQSTATREG;
    case SIM_BFIN_SYSCFG_REGNUM: return &SYSCFGREG;
    case SIM_BFIN_RETI_REGNUM: return &RETIREG;
    case SIM_BFIN_RETX_REGNUM: return &RETXREG;
    case SIM_BFIN_RETN_REGNUM: return &RETNREG;
    case SIM_BFIN_RETE_REGNUM: return &RETEREG;
    case SIM_BFIN_PC_REGNUM: return &PCREG;
    default: return NULL;
  }
}

static int
bfin_reg_fetch (SIM_CPU *cpu, int rn, void *buf, int len)
{
  bu32 value, *reg;

  reg = bfin_get_reg (cpu, rn);
  if (reg)
    value = *reg;
  else if (rn == SIM_BFIN_ASTAT_REGNUM)
    value = ASTAT;
  else if (rn == SIM_BFIN_CC_REGNUM)
    value = CCREG;
  else
    return -1;

  /* Handle our KSP/USP shadowing in SP.  While in supervisor mode, we
     have the normal SP/USP behavior.  User mode is tricky though.  */
  if (STATE_ENVIRONMENT (CPU_STATE (cpu)) == OPERATING_ENVIRONMENT
      && cec_is_user_mode (cpu))
    {
      if (rn == SIM_BFIN_SP_REGNUM)
	value = KSPREG;
      else if (rn == SIM_BFIN_USP_REGNUM)
	value = SPREG;
    }

  bfin_store_unsigned_integer (buf, 4, value);

  return 4;
}

static int
bfin_reg_store (SIM_CPU *cpu, int rn, const void *buf, int len)
{
  bu32 value, *reg;

  value = bfin_extract_unsigned_integer (buf, 4);
  reg = bfin_get_reg (cpu, rn);

  if (reg)
    /* XXX: Need register trace ?  */
    *reg = value;
  else if (rn == SIM_BFIN_ASTAT_REGNUM)
    SET_ASTAT (value);
  else if (rn == SIM_BFIN_CC_REGNUM)
    SET_CCREG (value);
  else
    return -1;

  return 4;
}

static sim_cia
bfin_pc_get (SIM_CPU *cpu)
{
  return PCREG;
}

static void
bfin_pc_set (SIM_CPU *cpu, sim_cia newpc)
{
  SET_PCREG (newpc);
}

static const char *
bfin_insn_name (SIM_CPU *cpu, int i)
{
  static const char * const insn_name[] = {
#define I(insn) #insn,
#include "insn_list.def"
#undef I
  };
  return insn_name[i];
}

static void
bfin_init_cpu (SIM_CPU *cpu)
{
  CPU_REG_FETCH (cpu) = bfin_reg_fetch;
  CPU_REG_STORE (cpu) = bfin_reg_store;
  CPU_PC_FETCH (cpu) = bfin_pc_get;
  CPU_PC_STORE (cpu) = bfin_pc_set;
  CPU_MAX_INSNS (cpu) = BFIN_INSN_MAX;
  CPU_INSN_NAME (cpu) = bfin_insn_name;
}

static void
bfin_prepare_run (SIM_CPU *cpu)
{
}

static const SIM_MODEL bfin_models[] =
{
#define P(n) { "bf"#n, & bfin_mach, MODEL_BF##n, NULL, bfin_model_init },
#include "proc_list.def"
#undef P
  { 0, NULL, 0, NULL, NULL, }
};

static const SIM_MACH_IMP_PROPERTIES bfin_imp_properties =
{
  sizeof (SIM_CPU),
  0,
};

static const SIM_MACH bfin_mach =
{
  "bfin", "bfin", MACH_BFIN,
  32, 32, & bfin_models[0], & bfin_imp_properties,
  bfin_init_cpu,
  bfin_prepare_run
};

const SIM_MACH * const bfin_sim_machs[] =
{
  & bfin_mach,
  NULL
};

/* Device option parsing.  */

static DECLARE_OPTION_HANDLER (bfin_mach_option_handler);

enum {
  OPTION_MACH_SIREV = OPTION_START,
  OPTION_MACH_HW_BOARD_FILE,
};

static const OPTION bfin_mach_options[] =
{
  { {"sirev", required_argument, NULL, OPTION_MACH_SIREV },
      '\0', "NUMBER", "Set CPU silicon revision",
      bfin_mach_option_handler, NULL },

  { {"hw-board-file", required_argument, NULL, OPTION_MACH_HW_BOARD_FILE },
      '\0', "FILE", "Add the supplemental devices listed in the file",
      bfin_mach_option_handler, NULL },

  { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

static SIM_RC
bfin_mach_option_handler (SIM_DESC sd, sim_cpu *current_cpu, int opt,
			  char *arg, int is_command)
{
  struct bfin_board_data *board = STATE_BOARD_DATA (sd);

  switch (opt)
    {
    case OPTION_MACH_SIREV:
      board->sirev_valid = 1;
      /* Accept (and throw away) a leading "0." in the version.  */
      if (!strncmp (arg, "0.", 2))
	arg += 2;
      board->sirev = atoi (arg);
      if (board->sirev > 0xf)
	{
	  sim_io_eprintf (sd, "sirev '%s' needs to fit into 4 bits\n", arg);
	  return SIM_RC_FAIL;
	}
      return SIM_RC_OK;

    case OPTION_MACH_HW_BOARD_FILE:
      board->hw_file = xstrdup (arg);
      return SIM_RC_OK;

    default:
      sim_io_eprintf (sd, "Unknown Blackfin option %d\n", opt);
      return SIM_RC_FAIL;
    }
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern MODULE_INIT_FN sim_install_bfin_mach;

SIM_RC
sim_install_bfin_mach (SIM_DESC sd)
{
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);
  return sim_add_option_table (sd, NULL, bfin_mach_options);
}
