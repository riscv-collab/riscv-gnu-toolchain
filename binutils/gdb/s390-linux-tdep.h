/* Target-dependent code for GNU/Linux on s390.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

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

#ifndef S390_LINUX_TDEP_H
#define S390_LINUX_TDEP_H

#define S390_IS_GREGSET_REGNUM(i)					\
  (((i) >= S390_PSWM_REGNUM && (i) <= S390_A15_REGNUM)			\
   || ((i) >= S390_R0_UPPER_REGNUM && (i) <= S390_R15_UPPER_REGNUM)	\
   || (i) == S390_ORIG_R2_REGNUM)

#define S390_IS_FPREGSET_REGNUM(i)			\
  ((i) >= S390_FPC_REGNUM && (i) <= S390_F15_REGNUM)

#define S390_IS_TDBREGSET_REGNUM(i)				\
  ((i) >= S390_TDB_DWORD0_REGNUM && (i) <= S390_TDB_R15_REGNUM)

/* Core file register sets, defined in s390-linux-tdep.c.  */
#define s390_sizeof_gregset 0x90
#define s390x_sizeof_gregset 0xd8
extern const struct regset s390_gregset;
#define s390_sizeof_fpregset 0x88
extern const struct regset s390_fpregset;
extern const struct regset s390_last_break_regset;
extern const struct regset s390x_last_break_regset;
extern const struct regset s390_system_call_regset;
extern const struct regset s390_tdb_regset;
#define s390_sizeof_tdbregset 0x100
extern const struct regset s390_vxrs_low_regset;
extern const struct regset s390_vxrs_high_regset;
extern const struct regset s390_gs_regset;
extern const struct regset s390_gsbc_regset;

/* GNU/Linux target descriptions.  */
extern const struct target_desc *tdesc_s390_linux32v1;
extern const struct target_desc *tdesc_s390_linux32v2;
extern const struct target_desc *tdesc_s390_linux64;
extern const struct target_desc *tdesc_s390_linux64v1;
extern const struct target_desc *tdesc_s390_linux64v2;
extern const struct target_desc *tdesc_s390_te_linux64;
extern const struct target_desc *tdesc_s390_vx_linux64;
extern const struct target_desc *tdesc_s390_tevx_linux64;
extern const struct target_desc *tdesc_s390_gs_linux64;
extern const struct target_desc *tdesc_s390x_linux64v1;
extern const struct target_desc *tdesc_s390x_linux64v2;
extern const struct target_desc *tdesc_s390x_te_linux64;
extern const struct target_desc *tdesc_s390x_vx_linux64;
extern const struct target_desc *tdesc_s390x_tevx_linux64;
extern const struct target_desc *tdesc_s390x_gs_linux64;

#endif /* S390_LINUX_TDEP_H */
