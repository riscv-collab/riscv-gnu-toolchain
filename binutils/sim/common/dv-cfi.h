/* Common Flash Memory Interface (CFI) model.

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

#ifndef DV_CFI_H
#define DV_CFI_H

/* CFI standard.  */
#define CFI_CMD_CFI_QUERY		0x98
#define CFI_ADDR_CFI_QUERY_START	0x55
#define CFI_ADDR_CFI_QUERY_RESULT	0x10

#define CFI_CMD_READ			0xFF
#define CFI_CMD_RESET			0xF0
#define CFI_CMD_READ_ID			0x90

/* Intel specific.  */
#define CFI_CMDSET_INTEL		0x0001
#define INTEL_CMD_STATUS_CLEAR		0x50
#define INTEL_CMD_STATUS_READ		0x70
#define INTEL_CMD_WRITE			0x40
#define INTEL_CMD_WRITE_ALT		0x10
#define INTEL_CMD_WRITE_BUFFER		0xE8
#define INTEL_CMD_WRITE_BUFFER_CONFIRM	0xD0
#define INTEL_CMD_LOCK_SETUP		0x60
#define INTEL_CMD_LOCK_BLOCK		0x01
#define INTEL_CMD_UNLOCK_BLOCK		0xD0
#define INTEL_CMD_LOCK_DOWN_BLOCK	0x2F
#define INTEL_CMD_ERASE_BLOCK		0x20
#define INTEL_CMD_ERASE_CONFIRM		0xD0

/* Intel Status Register bits.  */
#define INTEL_SR_BWS		(1 << 0)	/* BEFP Write.  */
#define INTEL_SR_BLS		(1 << 1)	/* Block Locked.  */
#define INTEL_SR_PSS		(1 << 2)	/* Program Suspend.  */
#define INTEL_SR_VPPS		(1 << 3)	/* Vpp.  */
#define INTEL_SR_PS		(1 << 4)	/* Program.  */
#define INTEL_SR_ES		(1 << 5)	/* Erase.  */
#define INTEL_SR_ESS		(1 << 6)	/* Erase Suspend.  */
#define INTEL_SR_DWS		(1 << 7)	/* Device Write.  */

#define INTEL_ID_MANU		0x89

#endif
