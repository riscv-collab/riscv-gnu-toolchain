/* Common native Linux definitions for the AArch64 scalable
   extensions: SVE and SME.

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef NAT_AARCH64_SCALABLE_LINUX_PTRACE_H
#define NAT_AARCH64_SCALABLE_LINUX_PTRACE_H

#include <signal.h>
#include <sys/utsname.h>

/* The order in which <sys/ptrace.h> and <asm/ptrace.h> are included
   can be important.  <sys/ptrace.h> often declares various PTRACE_*
   enums.  <asm/ptrace.h> often defines preprocessor constants for
   these very same symbols.  When that's the case, build errors will
   result when <asm/ptrace.h> is included before <sys/ptrace.h>.  */
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <stdarg.h>
#include "aarch64-scalable-linux-sigcontext.h"

/* Indicates whether a SVE ptrace header is followed by SVE registers or a
   fpsimd structure.  */
#define HAS_SVE_STATE(header) ((header).flags & SVE_PT_REGS_SVE)

/* Return true if there is an active SVE state in TID.
   Return false otherwise.  */
bool aarch64_has_sve_state (int tid);

/* Return true if there is an active SSVE state in TID.
   Return false otherwise.  */
bool aarch64_has_ssve_state (int tid);

/* Return true if there is an active ZA state in TID.
   Return false otherwise.  */
bool aarch64_has_za_state (int tid);

/* Given TID, read the SVE header into HEADER.

   Return true if successful, false otherwise.  */
bool read_sve_header (int tid, struct user_sve_header &header);

/* Given TID, store the SVE HEADER.

   Return true if successful, false otherwise.  */
bool write_sve_header (int tid, const struct user_sve_header &header);

/* Given TID, read the SSVE header into HEADER.

   Return true if successful, false otherwise.  */
bool read_ssve_header (int tid, struct user_sve_header &header);

/* Given TID, store the SSVE HEADER.

   Return true if successful, false otherwise.  */
bool write_ssve_header (int tid, const struct user_sve_header &header);

/* Given TID, read the ZA header into HEADER.

   Return true if successful, false otherwise.  */
bool read_za_header (int tid, struct user_za_header &header);

/* Given TID, store the ZA HEADER.

   Return true if successful, false otherwise.  */
bool write_za_header (int tid, const struct user_za_header &header);

/* Read VQ for the given tid using ptrace.  If SVE is not supported then zero
   is returned (on a system that supports SVE, then VQ cannot be zero).  */
uint64_t aarch64_sve_get_vq (int tid);

/* Set VQ in the kernel for the given tid, using either the value VQ or
   reading from the register VG in the register buffer.  */

bool aarch64_sve_set_vq (int tid, uint64_t vq);
bool aarch64_sve_set_vq (int tid, struct reg_buffer_common *reg_buf);

/* Read the streaming mode vq (svq) for the given TID.  If the ZA state is not
   supported or active, return 0.  */
uint64_t aarch64_za_get_svq (int tid);

/* Set the vector quotient (vq) in the kernel for the given TID using the
   value VQ.

   Return true if successful, false otherwise.  */
bool aarch64_za_set_svq (int tid, uint64_t vq);
bool aarch64_za_set_svq (int tid, const struct reg_buffer_common *reg_buf,
			 int svg_regnum);

/* Given TID, return the SVE/SSVE data as a vector of bytes.  */
extern gdb::byte_vector aarch64_fetch_sve_regset (int tid);

/* Write the SVE/SSVE contents from SVE_STATE to TID.  */
extern void aarch64_store_sve_regset (int tid,
				      const gdb::byte_vector &sve_state);

/* Given TID, return the ZA data as a vector of bytes.  */
extern gdb::byte_vector aarch64_fetch_za_regset (int tid);

/* Write ZA_STATE for TID.  */
extern void aarch64_store_za_regset (int tid, const gdb::byte_vector &za_state);

/* Given TID, initialize the ZA register set so the header contains the right
   size.  The bytes of the ZA register are initialized to zero.  */
extern void aarch64_initialize_za_regset (int tid);

/* Given TID, return the NT_ARM_ZT register set data as a vector of bytes.  */
extern gdb::byte_vector aarch64_fetch_zt_regset (int tid);

/* Write ZT_STATE for TID.  */
extern void aarch64_store_zt_regset (int tid, const gdb::byte_vector &zt_state);

/* Return TRUE if thread TID supports the NT_ARM_ZT register set.
   Return FALSE otherwise.  */
extern bool supports_zt_registers (int tid);

/* Given a register buffer REG_BUF, update it with SVE/SSVE register data
   from SVE_STATE.  */
extern void
aarch64_sve_regs_copy_to_reg_buf (int tid, struct reg_buffer_common *reg_buf);

/* Given a thread id TID and a register buffer REG_BUF containing SVE/SSVE
   register data, write the SVE data to thread TID.  */
extern void
aarch64_sve_regs_copy_from_reg_buf (int tid,
				    struct reg_buffer_common *reg_buf);

/* Given a thread id TID and a register buffer REG_BUF, update the register
   buffer with the ZA state from thread TID.

   ZA_REGNUM, SVG_REGNUM and SVCR_REGNUM are the register numbers for ZA,
   SVG and SVCR registers.  */
extern void aarch64_za_regs_copy_to_reg_buf (int tid,
					     struct reg_buffer_common *reg_buf,
					     int za_regnum, int svg_regnum,
					     int svcr_regnum);

/* Given a thread id TID and a register buffer REG_BUF containing ZA register
   data, write the ZA data to thread TID.

   ZA_REGNUM, SVG_REGNUM and SVCR_REGNUM are the register numbers for ZA,
   SVG and SVCR registers.  */
extern void
aarch64_za_regs_copy_from_reg_buf (int tid,
				   struct reg_buffer_common *reg_buf,
				   int za_regnum, int svg_regnum,
				   int svcr_regnum);

/* Given a thread id TID and a register buffer REG_BUF, update the register
   buffer with the ZT register set state from thread TID.

   ZT_REGNUM is the register number for ZT0.  */
extern void
aarch64_zt_regs_copy_to_reg_buf (int tid, struct reg_buffer_common *reg_buf,
				 int zt_regnum);

/* Given a thread id TID and a register buffer REG_BUF containing the ZT
   register set state, write the ZT data to thread TID.

   ZT_REGNUM is the register number for ZT0.  */
extern void
aarch64_zt_regs_copy_from_reg_buf (int tid, struct reg_buffer_common *reg_buf,
				   int zt_regnum);
#endif /* NAT_AARCH64_SCALABLE_LINUX_PTRACE_H */
