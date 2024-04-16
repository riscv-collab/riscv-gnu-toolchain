/* mem.h --- interface to memory for RL78 simulator.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

   This file is part of the GNU simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIM_RL78_MEM_H_
#define SIM_RL78_MEM_H_

#define MEM_SIZE 0x100000

/* Only for cpu.c to use.  */
extern unsigned char memory[];

void init_mem (void);

void mem_set_mirror (int rom_base, int ram_base, int length);

/* Pass the amount of bytes, like 2560 for 2.5k  */
void mem_ram_size (int ram_bytes);
void mem_rom_size (int rom_bytes);

void mem_put_qi (int address, unsigned char value);
void mem_put_hi (int address, unsigned short value);
void mem_put_psi (int address, unsigned long value);
void mem_put_si (int address, unsigned long value);

void mem_put_blk (int address, const void *bufptr, int nbytes);

unsigned char mem_get_pc (int address);

unsigned char mem_get_qi (int address);
unsigned short mem_get_hi (int address);
unsigned long mem_get_psi (int address);
unsigned long mem_get_si (int address);

void mem_get_blk (int address, void *bufptr, int nbytes);

int sign_ext (int v, int bits);

#endif
