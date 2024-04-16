/* Low level support for s390, shared between gdbserver and IPA.

   Copyright (C) 2016-2024 Free Software Foundation, Inc.

   This file is part of GDB.

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

#ifndef GDBSERVER_LINUX_S390_TDESC_H
#define GDBSERVER_LINUX_S390_TDESC_H

/* Note: since IPA obviously knows what ABI it's running on (s390 vs s390x),
   it's sufficient to pass only the register set here.  This, together with
   the ABI known at IPA compile time, maps to a tdesc.  */

enum s390_linux_tdesc {
  S390_TDESC_32,
  S390_TDESC_32V1,
  S390_TDESC_32V2,
  S390_TDESC_64,
  S390_TDESC_64V1,
  S390_TDESC_64V2,
  S390_TDESC_TE,
  S390_TDESC_VX,
  S390_TDESC_TEVX,
  S390_TDESC_GS,
};

#ifdef __s390x__

/* Defined in auto-generated file s390x-linux64.c.  */
void init_registers_s390x_linux64 (void);
extern const struct target_desc *tdesc_s390x_linux64;

/* Defined in auto-generated file s390x-linux64v1.c.  */
void init_registers_s390x_linux64v1 (void);
extern const struct target_desc *tdesc_s390x_linux64v1;

/* Defined in auto-generated file s390x-linux64v2.c.  */
void init_registers_s390x_linux64v2 (void);
extern const struct target_desc *tdesc_s390x_linux64v2;

/* Defined in auto-generated file s390x-te-linux64.c.  */
void init_registers_s390x_te_linux64 (void);
extern const struct target_desc *tdesc_s390x_te_linux64;

/* Defined in auto-generated file s390x-vx-linux64.c.  */
void init_registers_s390x_vx_linux64 (void);
extern const struct target_desc *tdesc_s390x_vx_linux64;

/* Defined in auto-generated file s390x-tevx-linux64.c.  */
void init_registers_s390x_tevx_linux64 (void);
extern const struct target_desc *tdesc_s390x_tevx_linux64;

/* Defined in auto-generated file s390x-gs-linux64.c.  */
void init_registers_s390x_gs_linux64 (void);
extern const struct target_desc *tdesc_s390x_gs_linux64;

#endif

#if !defined __s390x__ || !defined IN_PROCESS_AGENT

/* Defined in auto-generated file s390-linux32.c.  */
void init_registers_s390_linux32 (void);
extern const struct target_desc *tdesc_s390_linux32;

/* Defined in auto-generated file s390-linux32v1.c.  */
void init_registers_s390_linux32v1 (void);
extern const struct target_desc *tdesc_s390_linux32v1;

/* Defined in auto-generated file s390-linux32v2.c.  */
void init_registers_s390_linux32v2 (void);
extern const struct target_desc *tdesc_s390_linux32v2;

/* Defined in auto-generated file s390-linux64.c.  */
void init_registers_s390_linux64 (void);
extern const struct target_desc *tdesc_s390_linux64;

/* Defined in auto-generated file s390-linux64v1.c.  */
void init_registers_s390_linux64v1 (void);
extern const struct target_desc *tdesc_s390_linux64v1;

/* Defined in auto-generated file s390-linux64v2.c.  */
void init_registers_s390_linux64v2 (void);
extern const struct target_desc *tdesc_s390_linux64v2;

/* Defined in auto-generated file s390-te-linux64.c.  */
void init_registers_s390_te_linux64 (void);
extern const struct target_desc *tdesc_s390_te_linux64;

/* Defined in auto-generated file s390-vx-linux64.c.  */
void init_registers_s390_vx_linux64 (void);
extern const struct target_desc *tdesc_s390_vx_linux64;

/* Defined in auto-generated file s390-tevx-linux64.c.  */
void init_registers_s390_tevx_linux64 (void);
extern const struct target_desc *tdesc_s390_tevx_linux64;

/* Defined in auto-generated file s390-gs-linux64.c.  */
void init_registers_s390_gs_linux64 (void);
extern const struct target_desc *tdesc_s390_gs_linux64;

#endif

#endif /* GDBSERVER_LINUX_S390_TDESC_H */
