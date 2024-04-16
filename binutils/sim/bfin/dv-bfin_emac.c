/* Blackfin Ethernet Media Access Controller (EMAC) model.

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

/* This must come before any other includes.  */
#include "defs.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_LINUX_IF_TUN_H
#include <linux/if_tun.h>
#endif

#ifdef HAVE_LINUX_IF_TUN_H
# define WITH_TUN 1
#else
# define WITH_TUN 0
#endif

#include "sim-main.h"
#include "sim-hw.h"
#include "devices.h"
#include "dv-bfin_emac.h"

/* XXX: This doesn't support partial DMA transfers.  */
/* XXX: The TUN pieces should be pushed to the PHY so that we work with
        multiple "networks" and the PHY takes care of it.  */

struct bfin_emac
{
  /* This top portion matches common dv_bfin struct.  */
  bu32 base;
  struct hw *dma_master;
  bool acked;

  int tap;
#if WITH_TUN
  struct ifreq ifr;
#endif
  bu32 rx_crc;

  /* Order after here is important -- matches hardware MMR layout.  */
  bu32 opmode, addrlo, addrhi, hashlo, hashhi, staadd, stadat, flc, vlan1, vlan2;
  bu32 _pad0;
  bu32 wkup_ctl, wkup_ffmsk0, wkup_ffmsk1, wkup_ffmsk2, wkup_ffmsk3;
  bu32 wkup_ffcmd, wkup_ffoff, wkup_ffcrc0, wkup_ffcrc1;
  bu32 _pad1[4];
  bu32 sysctl, systat, rx_stat, rx_stky, rx_irqe, tx_stat, tx_stky, tx_irqe;
  bu32 mmc_ctl, mmc_rirqs, mmc_rirqe, mmc_tirqs, mmc_tirqe;
  bu32 _pad2[3];
  bu16 BFIN_MMR_16(ptp_ctl);
  bu16 BFIN_MMR_16(ptp_ie);
  bu16 BFIN_MMR_16(ptp_istat);
  bu32 ptp_foff, ptp_fv1, ptp_fv2, ptp_fv3, ptp_addend, ptp_accr, ptp_offset;
  bu32 ptp_timelo, ptp_timehi, ptp_rxsnaplo, ptp_rxsnaphi, ptp_txsnaplo;
  bu32 ptp_txsnaphi, ptp_alarmlo, ptp_alarmhi, ptp_id_off, ptp_id_snap;
  bu32 ptp_pps_startlo, ptp_pps_starthi, ptp_pps_period;
  bu32 _pad3[1];
  bu32 rxc_ok, rxc_fcs, rxc_lign, rxc_octet, rxc_dmaovf, rxc_unicst, rxc_multi;
  bu32 rxc_broad, rxc_lnerri, rxc_lnerro, rxc_long, rxc_macctl, rxc_opcode;
  bu32 rxc_pause, rxc_allfrm, rxc_alloct, rxc_typed, rxc_short, rxc_eq64;
  bu32 rxc_lt128, rxc_lt256, rxc_lt512, rxc_lt1024, rxc_ge1024;
  bu32 _pad4[8];
  bu32 txc_ok, txc_1col, txc_gt1col, txc_octet, txc_defer, txc_latecl;
  bu32 txc_xs_col, txc_dmaund, txc_crserr, txc_unicst, txc_multi, txc_broad;
  bu32 txc_xs_dfr, txc_macctl, txc_allfrm, txc_alloct, txc_eq64, txc_lt128;
  bu32 txc_lt256, txc_lt512, txc_lt1024, txc_ge1024, txc_abort;
};
#define mmr_base()      offsetof(struct bfin_emac, opmode)
#define mmr_offset(mmr) (offsetof(struct bfin_emac, mmr) - mmr_base())
#define mmr_idx(mmr)    (mmr_offset (mmr) / 4)

static const char * const mmr_names[BFIN_MMR_EMAC_SIZE / 4] =
{
  "EMAC_OPMODE", "EMAC_ADDRLO", "EMAC_ADDRHI", "EMAC_HASHLO", "EMAC_HASHHI",
  "EMAC_STAADD", "EMAC_STADAT", "EMAC_FLC", "EMAC_VLAN1", "EMAC_VLAN2", NULL,
  "EMAC_WKUP_CTL", "EMAC_WKUP_FFMSK0", "EMAC_WKUP_FFMSK1", "EMAC_WKUP_FFMSK2",
  "EMAC_WKUP_FFMSK3", "EMAC_WKUP_FFCMD", "EMAC_WKUP_FFOFF", "EMAC_WKUP_FFCRC0",
  "EMAC_WKUP_FFCRC1", [mmr_idx (sysctl)] = "EMAC_SYSCTL", "EMAC_SYSTAT",
  "EMAC_RX_STAT", "EMAC_RX_STKY", "EMAC_RX_IRQE", "EMAC_TX_STAT",
  "EMAC_TX_STKY", "EMAC_TX_IRQE", "EMAC_MMC_CTL", "EMAC_MMC_RIRQS",
  "EMAC_MMC_RIRQE", "EMAC_MMC_TIRQS", "EMAC_MMC_TIRQE",
  [mmr_idx (ptp_ctl)] = "EMAC_PTP_CTL", "EMAC_PTP_IE", "EMAC_PTP_ISTAT",
  "EMAC_PTP_FOFF", "EMAC_PTP_FV1", "EMAC_PTP_FV2", "EMAC_PTP_FV3",
  "EMAC_PTP_ADDEND", "EMAC_PTP_ACCR", "EMAC_PTP_OFFSET", "EMAC_PTP_TIMELO",
  "EMAC_PTP_TIMEHI", "EMAC_PTP_RXSNAPLO", "EMAC_PTP_RXSNAPHI",
  "EMAC_PTP_TXSNAPLO", "EMAC_PTP_TXSNAPHI", "EMAC_PTP_ALARMLO",
  "EMAC_PTP_ALARMHI", "EMAC_PTP_ID_OFF", "EMAC_PTP_ID_SNAP",
  "EMAC_PTP_PPS_STARTLO", "EMAC_PTP_PPS_STARTHI", "EMAC_PTP_PPS_PERIOD",
  [mmr_idx (rxc_ok)] = "EMAC_RXC_OK", "EMAC_RXC_FCS", "EMAC_RXC_LIGN",
  "EMAC_RXC_OCTET", "EMAC_RXC_DMAOVF", "EMAC_RXC_UNICST", "EMAC_RXC_MULTI",
  "EMAC_RXC_BROAD", "EMAC_RXC_LNERRI", "EMAC_RXC_LNERRO", "EMAC_RXC_LONG",
  "EMAC_RXC_MACCTL", "EMAC_RXC_OPCODE", "EMAC_RXC_PAUSE", "EMAC_RXC_ALLFRM",
  "EMAC_RXC_ALLOCT", "EMAC_RXC_TYPED", "EMAC_RXC_SHORT", "EMAC_RXC_EQ64",
  "EMAC_RXC_LT128", "EMAC_RXC_LT256", "EMAC_RXC_LT512", "EMAC_RXC_LT1024",
  "EMAC_RXC_GE1024",
  [mmr_idx (txc_ok)] = "EMAC_TXC_OK", "EMAC_TXC_1COL", "EMAC_TXC_GT1COL",
  "EMAC_TXC_OCTET", "EMAC_TXC_DEFER", "EMAC_TXC_LATECL", "EMAC_TXC_XS_COL",
  "EMAC_TXC_DMAUND", "EMAC_TXC_CRSERR", "EMAC_TXC_UNICST", "EMAC_TXC_MULTI",
  "EMAC_TXC_BROAD", "EMAC_TXC_XS_DFR", "EMAC_TXC_MACCTL", "EMAC_TXC_ALLFRM",
  "EMAC_TXC_ALLOCT", "EMAC_TXC_EQ64", "EMAC_TXC_LT128", "EMAC_TXC_LT256",
  "EMAC_TXC_LT512", "EMAC_TXC_LT1024", "EMAC_TXC_GE1024", "EMAC_TXC_ABORT",
};
#define mmr_name(off) (mmr_names[(off) / 4] ? : "<INV>")

static struct hw *
mii_find_phy (struct hw *me, bu8 addr)
{
  struct hw *phy = hw_child (me);
  while (phy && --addr)
    phy = hw_sibling (phy);
  return phy;
}

static void
mii_write (struct hw *me)
{
  SIM_DESC sd = hw_system (me);
  struct bfin_emac *emac = hw_data (me);
  struct hw *phy;
  bu8 addr = PHYAD (emac->staadd);
  bu8 reg = REGAD (emac->staadd);
  bu16 data = emac->stadat;

  phy = mii_find_phy (me, addr);
  if (!phy)
    return;
  sim_hw_io_write_buffer (sd, phy, &data, 1, reg, 2);
}

static void
mii_read (struct hw *me)
{
  SIM_DESC sd = hw_system (me);
  struct bfin_emac *emac = hw_data (me);
  struct hw *phy;
  bu8 addr = PHYAD (emac->staadd);
  bu8 reg = REGAD (emac->staadd);
  bu16 data;

  phy = mii_find_phy (me, addr);
  if (!phy || sim_hw_io_read_buffer (sd, phy, &data, 1, reg, 2) != 2)
    data = 0xffff;

  emac->stadat = data;
}

static unsigned
bfin_emac_io_write_buffer (struct hw *me, const void *source,
			   int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_emac *emac = hw_data (me);
  bu32 mmr_off;
  bu32 value;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  /* XXX: 16bit accesses are allowed ...  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, true))
    return 0;
  value = dv_load_4 (source);

  mmr_off = addr - emac->base;
  valuep = (void *)((uintptr_t)emac + mmr_base() + mmr_off);

  HW_TRACE_WRITE ();

  switch (mmr_off)
    {
    case mmr_offset(hashlo):
    case mmr_offset(hashhi):
    case mmr_offset(stadat):
    case mmr_offset(flc):
    case mmr_offset(vlan1):
    case mmr_offset(vlan2):
    case mmr_offset(wkup_ffmsk0):
    case mmr_offset(wkup_ffmsk1):
    case mmr_offset(wkup_ffmsk2):
    case mmr_offset(wkup_ffmsk3):
    case mmr_offset(wkup_ffcmd):
    case mmr_offset(wkup_ffoff):
    case mmr_offset(wkup_ffcrc0):
    case mmr_offset(wkup_ffcrc1):
    case mmr_offset(sysctl):
    case mmr_offset(rx_irqe):
    case mmr_offset(tx_irqe):
    case mmr_offset(mmc_rirqe):
    case mmr_offset(mmc_tirqe):
      *valuep = value;
      break;
    case mmr_offset(opmode):
      if (!(*valuep & RE) && (value & RE))
	emac->rx_stat &= ~RX_COMP;
      if (!(*valuep & TE) && (value & TE))
	emac->tx_stat &= ~TX_COMP;
      *valuep = value;
      break;
    case mmr_offset(addrlo):
    case mmr_offset(addrhi):
      *valuep = value;
      break;
    case mmr_offset(wkup_ctl):
      dv_w1c_4_partial (valuep, value, 0xf20);
      break;
    case mmr_offset(systat):
      dv_w1c_4 (valuep, value, 0xe1);
      break;
    case mmr_offset(staadd):
      *valuep = value | STABUSY;
      if (value & STAOP)
	mii_write (me);
      else
	mii_read (me);
      *valuep &= ~STABUSY;
      break;
    case mmr_offset(rx_stat):
    case mmr_offset(tx_stat):
      /* Discard writes to these.  */
      break;
    case mmr_offset(rx_stky):
    case mmr_offset(tx_stky):
    case mmr_offset(mmc_rirqs):
    case mmr_offset(mmc_tirqs):
      dv_w1c_4 (valuep, value, -1);
      break;
    case mmr_offset(mmc_ctl):
      /* Writing to bit 0 clears all counters.  */
      *valuep = value & ~1;
      if (value & 1)
	{
	  memset (&emac->rxc_ok, 0, mmr_offset (rxc_ge1024) - mmr_offset (rxc_ok) + 4);
	  memset (&emac->txc_ok, 0, mmr_offset (txc_abort) - mmr_offset (txc_ok) + 4);
	}
      break;
    case mmr_offset(rxc_ok) ... mmr_offset(rxc_ge1024):
    case mmr_offset(txc_ok) ... mmr_offset(txc_abort):
      /* XXX: Are these supposed to be read-only ?  */
      *valuep = value;
      break;
    case mmr_offset(ptp_ctl) ... mmr_offset(ptp_pps_period):
      /* XXX: Only on some models; ignore for now.  */
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, true);
      return 0;
    }

  return nr_bytes;
}

static unsigned
bfin_emac_io_read_buffer (struct hw *me, void *dest,
			  int space, address_word addr, unsigned nr_bytes)
{
  struct bfin_emac *emac = hw_data (me);
  bu32 mmr_off;
  bu32 *valuep;

  /* Invalid access mode is higher priority than missing register.  */
  /* XXX: 16bit accesses are allowed ...  */
  if (!dv_bfin_mmr_require_32 (me, addr, nr_bytes, false))
    return 0;

  mmr_off = addr - emac->base;
  valuep = (void *)((uintptr_t)emac + mmr_base() + mmr_off);

  HW_TRACE_READ ();

  switch (mmr_off)
    {
    case mmr_offset(opmode):
    case mmr_offset(addrlo):
    case mmr_offset(addrhi):
    case mmr_offset(hashlo):
    case mmr_offset(hashhi):
    case mmr_offset(staadd):
    case mmr_offset(stadat):
    case mmr_offset(flc):
    case mmr_offset(vlan1):
    case mmr_offset(vlan2):
    case mmr_offset(wkup_ctl):
    case mmr_offset(wkup_ffmsk0):
    case mmr_offset(wkup_ffmsk1):
    case mmr_offset(wkup_ffmsk2):
    case mmr_offset(wkup_ffmsk3):
    case mmr_offset(wkup_ffcmd):
    case mmr_offset(wkup_ffoff):
    case mmr_offset(wkup_ffcrc0):
    case mmr_offset(wkup_ffcrc1):
    case mmr_offset(sysctl):
    case mmr_offset(systat):
    case mmr_offset(rx_stat):
    case mmr_offset(rx_stky):
    case mmr_offset(rx_irqe):
    case mmr_offset(tx_stat):
    case mmr_offset(tx_stky):
    case mmr_offset(tx_irqe):
    case mmr_offset(mmc_rirqs):
    case mmr_offset(mmc_rirqe):
    case mmr_offset(mmc_tirqs):
    case mmr_offset(mmc_tirqe):
    case mmr_offset(mmc_ctl):
    case mmr_offset(rxc_ok) ... mmr_offset(rxc_ge1024):
    case mmr_offset(txc_ok) ... mmr_offset(txc_abort):
      dv_store_4 (dest, *valuep);
      break;
    case mmr_offset(ptp_ctl) ... mmr_offset(ptp_pps_period):
      /* XXX: Only on some models; ignore for now.  */
      break;
    default:
      dv_bfin_mmr_invalid (me, addr, nr_bytes, false);
      return 0;
    }

  return nr_bytes;
}

static void
attach_bfin_emac_regs (struct hw *me, struct bfin_emac *emac)
{
  address_word attach_address;
  int attach_space;
  unsigned attach_size;
  reg_property_spec reg;

  if (hw_find_property (me, "reg") == NULL)
    hw_abort (me, "Missing \"reg\" property");

  if (!hw_find_reg_array_property (me, "reg", 0, &reg))
    hw_abort (me, "\"reg\" property must contain three addr/size entries");

  hw_unit_address_to_attach_address (hw_parent (me),
				     &reg.address,
				     &attach_space, &attach_address, me);
  hw_unit_size_to_attach_size (hw_parent (me), &reg.size, &attach_size, me);

  if (attach_size != BFIN_MMR_EMAC_SIZE)
    hw_abort (me, "\"reg\" size must be %#x", BFIN_MMR_EMAC_SIZE);

  hw_attach_address (hw_parent (me),
		     0, attach_space, attach_address, attach_size, me);

  emac->base = attach_address;
}

static struct dv_bfin *dma_tx;

static unsigned
bfin_emac_dma_read_buffer (struct hw *me, void *dest, int space,
			   unsigned_word addr, unsigned nr_bytes)
{
  struct bfin_emac *emac = hw_data (me);
  struct dv_bfin *dma = hw_data (emac->dma_master);
  unsigned char *data = dest;
  static bool flop; /* XXX: This sucks.  */
  bu16 len;
  ssize_t ret;

  HW_TRACE_DMA_READ ();

  if (dma_tx == dma)
    {
      /* Handle the TX turn around and write the status.  */
      emac->tx_stat |= TX_OK;
      emac->tx_stky |= TX_OK;

      memcpy (data, &emac->tx_stat, 4);

      dma->acked = true;
      return 4;
    }

  if (!(emac->opmode & RE))
    return 0;

  if (!flop)
    {
      ssize_t pad_ret;
      /* Outgoing DMA buffer has 16bit len prepended to it.  */
      data += 2;

      /* This doesn't seem to work.
      if (emac->sysctl & RXDWA)
	{
	  memset (data, 0, 2);
	  data += 2;
	} */

      ret = read (emac->tap, data, nr_bytes);
      if (ret < 0)
	return 0;
      ret += 4; /* include crc */
      pad_ret = max (ret + 4, 64);
      len = pad_ret;
      memcpy (dest, &len, 2);

      pad_ret = (pad_ret + 3) & ~3;
      if (ret < pad_ret)
	memset (data + ret, 0, pad_ret - ret);
      pad_ret += 4;

      /* XXX: Need to check -- u-boot doesn't look at this.  */
      if (emac->sysctl & RXCKS)
	{
	  pad_ret += 4;
	  emac->rx_crc = 0;
	}
      ret = pad_ret;

      /* XXX: Don't support promiscuous yet.  */
      emac->rx_stat |= RX_ACCEPT;
      emac->rx_stat = (emac->rx_stat & ~RX_FRLEN) | len;

      emac->rx_stat |= RX_COMP;
      emac->rx_stky |= RX_COMP;
    }
  else
    {
      /* Write the RX status and crc info.  */
      emac->rx_stat |= RX_OK;
      emac->rx_stky |= RX_OK;

      ret = 4;
      if (emac->sysctl & RXCKS)
	{
	  memcpy (data, &emac->rx_crc, 4);
	  data += 4;
	  ret += 4;
	}
      memcpy (data, &emac->rx_stat, 4);
    }

  flop = !flop;
  dma->acked = true;
  return ret;
}

static unsigned
bfin_emac_dma_write_buffer (struct hw *me, const void *source,
			    int space, unsigned_word addr,
			    unsigned nr_bytes,
			    int violate_read_only_section)
{
  struct bfin_emac *emac = hw_data (me);
  struct dv_bfin *dma = hw_data (emac->dma_master);
  const unsigned char *data = source;
  bu16 len;
  ssize_t ret;

  HW_TRACE_DMA_WRITE ();

  if (!(emac->opmode & TE))
    return 0;

  /* Incoming DMA buffer has 16bit len prepended to it.  */
  memcpy (&len, data, 2);
  if (!len)
    return 0;

  ret = write (emac->tap, data + 2, len);
  if (ret < 0)
    return 0;
  ret += 2;

  emac->tx_stat |= TX_COMP;
  emac->tx_stky |= TX_COMP;

  dma_tx = dma;
  dma->acked = true;
  return ret;
}

static const struct hw_port_descriptor bfin_emac_ports[] =
{
  { "tx",   DV_PORT_TX,   0, output_port, },
  { "rx",   DV_PORT_RX,   0, output_port, },
  { "stat", DV_PORT_STAT, 0, output_port, },
  { NULL, 0, 0, 0, },
};

static void
bfin_emac_attach_address_callback (struct hw *me,
				   int level,
				   int space,
				   address_word addr,
				   address_word nr_bytes,
				   struct hw *client)
{
  const hw_unit *unit = hw_unit_address (client);
  HW_TRACE ((me, "attach - level=%d, space=%d, addr=0x%lx, nr_bytes=%lu, client=%s",
	     level, space, (unsigned long) addr, (unsigned long) nr_bytes, hw_path (client)));
  /* NOTE: At preset the space is assumed to be zero.  Perhaphs the
     space should be mapped onto something for instance: space0 -
     unified memory; space1 - IO memory; ... */
  sim_core_attach (hw_system (me),
		   NULL, /*cpu*/
		   level + 10 + unit->cells[unit->nr_cells - 1],
		   access_read_write_exec,
		   space, addr,
		   nr_bytes,
		   0, /* modulo */
		   client,
		   NULL);
}

static void
bfin_emac_delete (struct hw *me)
{
  struct bfin_emac *emac = hw_data (me);
  close (emac->tap);
}

static void
bfin_emac_tap_init (struct hw *me)
{
#if WITH_TUN
  struct bfin_emac *emac = hw_data (me);
  int flags;

  emac->tap = open ("/dev/net/tun", O_RDWR);
  if (emac->tap == -1)
    {
      HW_TRACE ((me, "unable to open /dev/net/tun: %s", strerror (errno)));
      return;
    }

  memset (&emac->ifr, 0, sizeof (emac->ifr));
  emac->ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  strcpy (emac->ifr.ifr_name, "tap-gdb");

  flags = 1 * 1024 * 1024;
  if (ioctl (emac->tap, TUNSETIFF, &emac->ifr) < 0
#ifdef TUNSETNOCSUM
      || ioctl (emac->tap, TUNSETNOCSUM) < 0
#endif
#ifdef TUNSETSNDBUF
      || ioctl (emac->tap, TUNSETSNDBUF, &flags) < 0
#endif
     )
    {
      HW_TRACE ((me, "tap ioctl setup failed: %s", strerror (errno)));
      close (emac->tap);
      return;
    }

  flags = fcntl (emac->tap, F_GETFL);
  fcntl (emac->tap, F_SETFL, flags | O_NONBLOCK);
#endif
}

static void
bfin_emac_finish (struct hw *me)
{
  struct bfin_emac *emac;

  emac = HW_ZALLOC (me, struct bfin_emac);

  set_hw_data (me, emac);
  set_hw_io_read_buffer (me, bfin_emac_io_read_buffer);
  set_hw_io_write_buffer (me, bfin_emac_io_write_buffer);
  set_hw_dma_read_buffer (me, bfin_emac_dma_read_buffer);
  set_hw_dma_write_buffer (me, bfin_emac_dma_write_buffer);
  set_hw_ports (me, bfin_emac_ports);
  set_hw_attach_address (me, bfin_emac_attach_address_callback);
  set_hw_delete (me, bfin_emac_delete);

  attach_bfin_emac_regs (me, emac);

  /* Initialize the EMAC.  */
  emac->addrlo = 0xffffffff;
  emac->addrhi = 0x0000ffff;
  emac->vlan1 = 0x0000ffff;
  emac->vlan2 = 0x0000ffff;
  emac->sysctl = 0x00003f00;
  emac->mmc_ctl = 0x0000000a;

  bfin_emac_tap_init (me);
}

const struct hw_descriptor dv_bfin_emac_descriptor[] =
{
  {"bfin_emac", bfin_emac_finish,},
  {NULL, NULL},
};
