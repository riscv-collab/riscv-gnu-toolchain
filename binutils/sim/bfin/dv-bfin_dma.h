/* Blackfin Direct Memory Access (DMA) Channel model.

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

#ifndef DV_BFIN_DMA_H
#define DV_BFIN_DMA_H

/* DMA_CONFIG Masks */
#define DMAEN		0x0001	/* DMA Channel Enable */
#define WNR		0x0002	/* Channel Direction (W/R*) */
#define WDSIZE_8	0x0000	/* Transfer Word Size = 8 */
#define WDSIZE_16	0x0004	/* Transfer Word Size = 16 */
#define WDSIZE_32	0x0008	/* Transfer Word Size = 32 */
#define WDSIZE		0x000c	/* Transfer Word Size */
#define DMA2D		0x0010	/* DMA Mode (2D/1D*) */
#define RESTART		0x0020	/* DMA Buffer Clear */
#define DI_SEL		0x0040	/* Data Interrupt Timing Select */
#define DI_EN		0x0080	/* Data Interrupt Enable */
#define NDSIZE_0	0x0000	/* Next Descriptor Size = 0 (Stop/Autobuffer) */
#define NDSIZE_1	0x0100	/* Next Descriptor Size = 1 */
#define NDSIZE_2	0x0200	/* Next Descriptor Size = 2 */
#define NDSIZE_3	0x0300	/* Next Descriptor Size = 3 */
#define NDSIZE_4	0x0400	/* Next Descriptor Size = 4 */
#define NDSIZE_5	0x0500	/* Next Descriptor Size = 5 */
#define NDSIZE_6	0x0600	/* Next Descriptor Size = 6 */
#define NDSIZE_7	0x0700	/* Next Descriptor Size = 7 */
#define NDSIZE_8	0x0800	/* Next Descriptor Size = 8 */
#define NDSIZE_9	0x0900	/* Next Descriptor Size = 9 */
#define NDSIZE		0x0f00	/* Next Descriptor Size */
#define NDSIZE_SHIFT	8
#define DMAFLOW		0x7000	/* Flow Control */
#define DMAFLOW_STOP	0x0000	/* Stop Mode */
#define DMAFLOW_AUTO	0x1000	/* Autobuffer Mode */
#define DMAFLOW_ARRAY	0x4000	/* Descriptor Array Mode */
#define DMAFLOW_SMALL	0x6000	/* Small Model Descriptor List Mode */
#define DMAFLOW_LARGE	0x7000	/* Large Model Descriptor List Mode */

/* DMA_IRQ_STATUS Masks */
#define DMA_DONE	0x0001	/* DMA Completion Interrupt Status */
#define DMA_ERR		0x0002	/* DMA Error Interrupt Status */
#define DFETCH		0x0004	/* DMA Descriptor Fetch Indicator */
#define DMA_RUN		0x0008	/* DMA Channel Running Indicator */

/* DMA_PERIPHERAL_MAP Masks */
#define CTYPE		(1 << 6)

#endif
