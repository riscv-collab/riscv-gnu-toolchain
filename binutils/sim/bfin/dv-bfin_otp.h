/* Blackfin One-Time Programmable Memory (OTP) model

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

#ifndef DV_BFIN_OTP_H
#define DV_BFIN_OTP_H

/* OTP Defined Pages.  */
#define FPS00		0x004
#define FPS01		0x005
#define FPS02		0x006
#define FPS03		0x007
#define FPS04		0x008
#define FPS05		0x009
#define FPS06		0x00A
#define FPS07		0x00B
#define FPS08		0x00C
#define FPS09		0x00D
#define FPS10		0x00E
#define FPS11		0x00F
#define CPS00		0x010
#define CPS01		0x011
#define CPS02		0x012
#define CPS03		0x013
#define CPS04		0x014
#define CPS05		0x015
#define CPS06		0x016
#define CPS07		0x017
#define PBS00		0x018
#define PBS01		0x019
#define PBS02		0x01A
#define PBS03		0x01B
#define PUB000		0x01C
#define PUBCRC000	0x0E0
#define PRIV000		0x110
#define PRIVCRC000	0x1E0

/* FPS03 Part values.  */
#define FPS03_BF51XF(n)	(FPS03_BF##n | 0xF000)
#define FPS03_BF512	0x0200
#define FPS03_BF512F	FPS03_BF51XF(512)
#define FPS03_BF514	0x0202
#define FPS03_BF514F	FPS03_BF51XF(514)
#define FPS03_BF516	0x0204
#define FPS03_BF516F	FPS03_BF51XF(516)
#define FPS03_BF518	0x0206
#define FPS03_BF518F	FPS03_BF51XF(518)
#define FPS03_BF52X_C1(n)	(FPS03_BF##n | 0x8000)
#define FPS03_BF52X_C2(n)	(FPS03_BF##n | 0x4000)
#define FPS03_BF522	0x020A
#define FPS03_BF522_C1	FPS03_BF52X_C1(522)
#define FPS03_BF522_C2	FPS03_BF52X_C2(522)
#define FPS03_BF523	0x020B
#define FPS03_BF523_C1	FPS03_BF52X_C1(523)
#define FPS03_BF523_C2	FPS03_BF52X_C2(523)
#define FPS03_BF524	0x020C
#define FPS03_BF524_C1	FPS03_BF52X_C1(524)
#define FPS03_BF524_C2	FPS03_BF52X_C2(524)
#define FPS03_BF525	0x020D
#define FPS03_BF525_C1	FPS03_BF52X_C1(525)
#define FPS03_BF525_C2	FPS03_BF52X_C2(525)
#define FPS03_BF526	0x020E
#define FPS03_BF526_C1	FPS03_BF52X_C1(526)
#define FPS03_BF526_C2	FPS03_BF52X_C2(526)
#define FPS03_BF527	0x020F
#define FPS03_BF527_C1	FPS03_BF52X_C1(527)
#define FPS03_BF527_C2	FPS03_BF52X_C2(527)

/* OTP_CONTROL masks.  */
#define PAGE_ADDR	(0x1FF)
#define DO_READ		(1 << 14)
#define DO_WRITE	(1 << 15)

/* OTP_STATUS masks.  */
#define STATUS_DONE	(1 << 0)
#define STATUS_ERR	(1 << 1)

#endif
