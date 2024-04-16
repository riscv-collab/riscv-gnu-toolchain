/* Common Blackfin device stuff.

   Copyright (C) 2010-2024 Free Software Foundation, Inc.
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

#ifndef DEVICES_H
#define DEVICES_H

#include "hw-base.h"
#include "hw-main.h"
#include "hw-device.h"
#include "hw-tree.h"

#include "bfin-sim.h"

/* We keep the same inital structure layout with DMA enabled devices.  */
struct dv_bfin {
  bu32 base;
  struct hw *dma_master;
  bool acked;
};

#define BFIN_MMR_16(mmr) mmr, __pad_##mmr

/* Most peripherals have either one interrupt or these three.  */
#define DV_PORT_TX   0
#define DV_PORT_RX   1
#define DV_PORT_STAT 2

unsigned int dv_get_bus_num (struct hw *);

static inline bu8 dv_load_1 (const void *ptr)
{
  const unsigned char *c = ptr;
  return c[0];
}

static inline void dv_store_1 (void *ptr, bu8 val)
{
  unsigned char *c = ptr;
  c[0] = val;
}

static inline bu16 dv_load_2 (const void *ptr)
{
  const unsigned char *c = ptr;
  return (c[1] << 8) | dv_load_1 (ptr);
}

static inline void dv_store_2 (void *ptr, bu16 val)
{
  unsigned char *c = ptr;
  c[1] = val >> 8;
  dv_store_1 (ptr, val);
}

static inline bu32 dv_load_4 (const void *ptr)
{
  const unsigned char *c = ptr;
  return (c[3] << 24) | (c[2] << 16) | dv_load_2 (ptr);
}

static inline void dv_store_4 (void *ptr, bu32 val)
{
  unsigned char *c = ptr;
  c[3] = val >> 24;
  c[2] = val >> 16;
  dv_store_2 (ptr, val);
}

/* Helpers for MMRs where only the specified bits are W1C.  The
   rest are left unmodified.  */
#define dv_w1c(ptr, val, bits) (*(ptr) &= ~((val) & (bits)))
static inline void dv_w1c_2 (bu16 *ptr, bu16 val, bu16 bits)
{
  dv_w1c (ptr, val, bits);
}
static inline void dv_w1c_4 (bu32 *ptr, bu32 val, bu32 bits)
{
  dv_w1c (ptr, val, bits);
}

/* Helpers for MMRs where all bits are RW except for the specified
   bits -- those ones are W1C.  */
#define dv_w1c_partial(ptr, val, bits) \
  (*(ptr) = ((val) | (*(ptr) & (bits))) & ~((val) & (bits)))
static inline void dv_w1c_2_partial (bu16 *ptr, bu16 val, bu16 bits)
{
  dv_w1c_partial (ptr, val, bits);
}
static inline void dv_w1c_4_partial (bu32 *ptr, bu32 val, bu32 bits)
{
  dv_w1c_partial (ptr, val, bits);
}

/* XXX: Grubbing around in device internals is probably wrong, but
        until someone shows me what's right ...  */
static inline struct hw *
dv_get_device (SIM_CPU *cpu, const char *device_name)
{
  SIM_DESC sd = CPU_STATE (cpu);
  void *root = STATE_HW (sd);
  return hw_tree_find_device (root, device_name);
}

static inline void *
dv_get_state (SIM_CPU *cpu, const char *device_name)
{
  return hw_data (dv_get_device (cpu, device_name));
}

#define DV_STATE(cpu, dv) dv_get_state (cpu, "/core/bfin_"#dv)

#define DV_STATE_CACHED(cpu, dv) \
  ({ \
    struct bfin_##dv *__##dv = BFIN_CPU_STATE.dv##_cache; \
    if (!__##dv) \
      BFIN_CPU_STATE.dv##_cache = __##dv = dv_get_state (cpu, "/core/bfin_"#dv); \
    __##dv; \
  })

void dv_bfin_mmr_invalid (struct hw *, address_word, unsigned nr_bytes, bool write);
bool dv_bfin_mmr_require (struct hw *, address_word, unsigned nr_bytes, unsigned size, bool write);
/* For 32-bit memory mapped registers that allow 16-bit or 32-bit access.  */
bool dv_bfin_mmr_require_16_32 (struct hw *, address_word, unsigned nr_bytes, bool write);
/* For 32-bit memory mapped registers that only allow 16-bit access.  */
#define dv_bfin_mmr_require_16(hw, addr, nr_bytes, write) dv_bfin_mmr_require (hw, addr, nr_bytes, 2, write)
/* For 32-bit memory mapped registers that only allow 32-bit access.  */
#define dv_bfin_mmr_require_32(hw, addr, nr_bytes, write) dv_bfin_mmr_require (hw, addr, nr_bytes, 4, write)

#define HW_TRACE_WRITE() \
  HW_TRACE ((me, "write 0x%08lx (%s) length %u with 0x%x", \
	     (unsigned long) addr, mmr_name (mmr_off), nr_bytes, value))
#define HW_TRACE_READ() \
  HW_TRACE ((me, "read 0x%08lx (%s) length %u", \
	     (unsigned long) addr, mmr_name (mmr_off), nr_bytes))

#define HW_TRACE_DMA_WRITE() \
  HW_TRACE ((me, "dma write 0x%08lx length %u", \
	     (unsigned long) addr, nr_bytes))
#define HW_TRACE_DMA_READ() \
  HW_TRACE ((me, "dma read 0x%08lx length %u", \
	     (unsigned long) addr, nr_bytes))

#endif
