/* Target-dependent code for the RISC-V architecture, for GDB.

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

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "language.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "target.h"
#include "arch-utils.h"
#include "regcache.h"
#include "osabi.h"
#include "riscv-tdep.h"
#include "reggroups.h"
#include "opcode/riscv.h"
#include "elf/riscv.h"
#include "elf-bfd.h"
#include "symcat.h"
#include "dis-asm.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"
#include "infcall.h"
#include "floatformat.h"
#include "remote.h"
#include "target-descriptions.h"
#include "dwarf2/frame.h"
#include "user-regs.h"
#include "valprint.h"
#include "gdbsupport/common-defs.h"
#include "opcode/riscv-opc.h"
#include "cli/cli-decode.h"
#include "observable.h"
#include "prologue-value.h"
#include "arch/riscv.h"
#include "riscv-ravenscar-thread.h"
#include "gdbsupport/gdb-safe-ctype.h"

/* The stack must be 16-byte aligned.  */
#define SP_ALIGNMENT 16

/* The biggest alignment that the target supports.  */
#define BIGGEST_ALIGNMENT 16

/* Define a series of is_XXX_insn functions to check if the value INSN
   is an instance of instruction XXX.  */
#define DECLARE_INSN(INSN_NAME, INSN_MATCH, INSN_MASK) \
static inline bool is_ ## INSN_NAME ## _insn (long insn) \
{ \
  return (insn & INSN_MASK) == INSN_MATCH; \
}
#include "opcode/riscv-opc.h"
#undef DECLARE_INSN

/* When this is true debugging information about breakpoint kinds will be
   printed.  */

static bool riscv_debug_breakpoints = false;

/* Print a "riscv-breakpoints" debug statement.  */

#define riscv_breakpoints_debug_printf(fmt, ...)	\
  debug_prefixed_printf_cond (riscv_debug_breakpoints,	\
			      "riscv-breakpoints",	\
			      fmt, ##__VA_ARGS__)

/* When this is true debugging information about inferior calls will be
   printed.  */

static bool riscv_debug_infcall = false;

/* Print a "riscv-infcall" debug statement.  */

#define riscv_infcall_debug_printf(fmt, ...)				\
  debug_prefixed_printf_cond (riscv_debug_infcall, "riscv-infcall",	\
			      fmt, ##__VA_ARGS__)

/* Print "riscv-infcall" start/end debug statements.  */

#define RISCV_INFCALL_SCOPED_DEBUG_START_END(fmt, ...)		\
  scoped_debug_start_end (riscv_debug_infcall, "riscv-infcall", \
			  fmt, ##__VA_ARGS__)

/* When this is true debugging information about stack unwinding will be
   printed.  */

static bool riscv_debug_unwinder = false;

/* Print a "riscv-unwinder" debug statement.  */

#define riscv_unwinder_debug_printf(fmt, ...)				\
  debug_prefixed_printf_cond (riscv_debug_unwinder, "riscv-unwinder",	\
			      fmt, ##__VA_ARGS__)

/* When this is true debugging information about gdbarch initialisation
   will be printed.  */

static bool riscv_debug_gdbarch = false;

/* Print a "riscv-gdbarch" debug statement.  */

#define riscv_gdbarch_debug_printf(fmt, ...)				\
  debug_prefixed_printf_cond (riscv_debug_gdbarch, "riscv-gdbarch",	\
			      fmt, ##__VA_ARGS__)

/* The names of the RISC-V target description features.  */
const char *riscv_feature_name_csr = "org.gnu.gdb.riscv.csr";
static const char *riscv_feature_name_cpu = "org.gnu.gdb.riscv.cpu";
static const char *riscv_feature_name_fpu = "org.gnu.gdb.riscv.fpu";
static const char *riscv_feature_name_virtual = "org.gnu.gdb.riscv.virtual";
static const char *riscv_feature_name_vector = "org.gnu.gdb.riscv.vector";

/* The current set of options to be passed to the disassembler.  */
static char *riscv_disassembler_options;

/* Cached information about a frame.  */

struct riscv_unwind_cache
{
  /* The register from which we can calculate the frame base.  This is
     usually $sp or $fp.  */
  int frame_base_reg;

  /* The offset from the current value in register FRAME_BASE_REG to the
     actual frame base address.  */
  int frame_base_offset;

  /* Information about previous register values.  */
  trad_frame_saved_reg *regs;

  /* The id for this frame.  */
  struct frame_id this_id;

  /* The base (stack) address for this frame.  This is the stack pointer
     value on entry to this frame before any adjustments are made.  */
  CORE_ADDR frame_base;
};

/* RISC-V specific register group for CSRs.  */

static const reggroup *csr_reggroup = nullptr;

/* Callback function for user_reg_add.  */

static struct value *
value_of_riscv_user_reg (frame_info_ptr frame, const void *baton)
{
  const int *reg_p = (const int *) baton;
  return value_of_register (*reg_p, get_next_frame_sentinel_okay (frame));
}

/* Information about a register alias that needs to be set up for this
   target.  These are collected when the target's XML description is
   analysed, and then processed later, once the gdbarch has been created.  */

class riscv_pending_register_alias
{
public:
  /* Constructor.  */

  riscv_pending_register_alias (const char *name, const void *baton)
    : m_name (name),
      m_baton (baton)
  { /* Nothing.  */ }

  /* Convert this into a user register for GDBARCH.  */

  void create (struct gdbarch *gdbarch) const
  {
    user_reg_add (gdbarch, m_name, value_of_riscv_user_reg, m_baton);
  }

private:
  /* The name for this alias.  */
  const char *m_name;

  /* The baton value for passing to user_reg_add.  This must point to some
     data that will live for at least as long as the gdbarch object to
     which the user register is attached.  */
  const void *m_baton;
};

/* A set of registers that we expect to find in a tdesc_feature.  These
   are use in RISCV_GDBARCH_INIT when processing the target description.  */

struct riscv_register_feature
{
  explicit riscv_register_feature (const char *feature_name)
    : m_feature_name (feature_name)
  { /* Delete.  */ }

  riscv_register_feature () = delete;
  DISABLE_COPY_AND_ASSIGN (riscv_register_feature);

  /* Information for a single register.  */
  struct register_info
  {
    /* The GDB register number for this register.  */
    int regnum;

    /* List of names for this register.  The first name in this list is the
       preferred name, the name GDB should use when describing this
       register.  */
    std::vector<const char *> names;

    /* Look in FEATURE for a register with a name from this classes names
       list.  If the register is found then register its number with
       TDESC_DATA and add all its aliases to the ALIASES list.
       PREFER_FIRST_NAME_P is used when deciding which aliases to create.  */
    bool check (struct tdesc_arch_data *tdesc_data,
		const struct tdesc_feature *feature,
		bool prefer_first_name_p,
		std::vector<riscv_pending_register_alias> *aliases) const;
  };

  /* Return the name of this feature.  */
  const char *name () const
  { return m_feature_name; }

protected:

  /* Return a target description feature extracted from TDESC for this
     register feature.  Will return nullptr if there is no feature in TDESC
     with the name M_FEATURE_NAME.  */
  const struct tdesc_feature *tdesc_feature (const struct target_desc *tdesc) const
  {
    return tdesc_find_feature (tdesc, name ());
  }

  /* List of all the registers that we expect that we might find in this
     register set.  */
  std::vector<struct register_info> m_registers;

private:

  /* The name for this feature.  This is the name used to find this feature
     within the target description.  */
  const char *m_feature_name;
};

/* See description in the class declaration above.  */

bool
riscv_register_feature::register_info::check
	(struct tdesc_arch_data *tdesc_data,
	 const struct tdesc_feature *feature,
	 bool prefer_first_name_p,
	 std::vector<riscv_pending_register_alias> *aliases) const
{
  for (const char *name : this->names)
    {
      bool found = tdesc_numbered_register (feature, tdesc_data,
					    this->regnum, name);
      if (found)
	{
	  /* We know that the target description mentions this
	     register.  In RISCV_REGISTER_NAME we ensure that GDB
	     always uses the first name for each register, so here we
	     add aliases for all of the remaining names.  */
	  int start_index = prefer_first_name_p ? 1 : 0;
	  for (int i = start_index; i < this->names.size (); ++i)
	    {
	      const char *alias = this->names[i];
	      if (alias == name && !prefer_first_name_p)
		continue;
	      aliases->emplace_back (alias, (void *) &this->regnum);
	    }
	  return true;
	}
    }
  return false;
}

/* Class representing the x-registers feature set.  */

struct riscv_xreg_feature : public riscv_register_feature
{
  riscv_xreg_feature ()
    : riscv_register_feature (riscv_feature_name_cpu)
  {
    m_registers =  {
      { RISCV_ZERO_REGNUM + 0, { "zero", "x0" } },
      { RISCV_ZERO_REGNUM + 1, { "ra", "x1" } },
      { RISCV_ZERO_REGNUM + 2, { "sp", "x2" } },
      { RISCV_ZERO_REGNUM + 3, { "gp", "x3" } },
      { RISCV_ZERO_REGNUM + 4, { "tp", "x4" } },
      { RISCV_ZERO_REGNUM + 5, { "t0", "x5" } },
      { RISCV_ZERO_REGNUM + 6, { "t1", "x6" } },
      { RISCV_ZERO_REGNUM + 7, { "t2", "x7" } },
      { RISCV_ZERO_REGNUM + 8, { "fp", "x8", "s0" } },
      { RISCV_ZERO_REGNUM + 9, { "s1", "x9" } },
      { RISCV_ZERO_REGNUM + 10, { "a0", "x10" } },
      { RISCV_ZERO_REGNUM + 11, { "a1", "x11" } },
      { RISCV_ZERO_REGNUM + 12, { "a2", "x12" } },
      { RISCV_ZERO_REGNUM + 13, { "a3", "x13" } },
      { RISCV_ZERO_REGNUM + 14, { "a4", "x14" } },
      { RISCV_ZERO_REGNUM + 15, { "a5", "x15" } },
      { RISCV_ZERO_REGNUM + 16, { "a6", "x16" } },
      { RISCV_ZERO_REGNUM + 17, { "a7", "x17" } },
      { RISCV_ZERO_REGNUM + 18, { "s2", "x18" } },
      { RISCV_ZERO_REGNUM + 19, { "s3", "x19" } },
      { RISCV_ZERO_REGNUM + 20, { "s4", "x20" } },
      { RISCV_ZERO_REGNUM + 21, { "s5", "x21" } },
      { RISCV_ZERO_REGNUM + 22, { "s6", "x22" } },
      { RISCV_ZERO_REGNUM + 23, { "s7", "x23" } },
      { RISCV_ZERO_REGNUM + 24, { "s8", "x24" } },
      { RISCV_ZERO_REGNUM + 25, { "s9", "x25" } },
      { RISCV_ZERO_REGNUM + 26, { "s10", "x26" } },
      { RISCV_ZERO_REGNUM + 27, { "s11", "x27" } },
      { RISCV_ZERO_REGNUM + 28, { "t3", "x28" } },
      { RISCV_ZERO_REGNUM + 29, { "t4", "x29" } },
      { RISCV_ZERO_REGNUM + 30, { "t5", "x30" } },
      { RISCV_ZERO_REGNUM + 31, { "t6", "x31" } },
      { RISCV_ZERO_REGNUM + 32, { "pc" } }
    };
  }

  /* Return the preferred name for the register with gdb register number
     REGNUM, which must be in the inclusive range RISCV_ZERO_REGNUM to
     RISCV_PC_REGNUM.  */
  const char *register_name (int regnum) const
  {
    gdb_assert (regnum >= RISCV_ZERO_REGNUM && regnum <= m_registers.size ());
    return m_registers[regnum].names[0];
  }

  /* Check this feature within TDESC, record the registers from this
     feature into TDESC_DATA and update ALIASES and FEATURES.  */
  bool check (const struct target_desc *tdesc,
	      struct tdesc_arch_data *tdesc_data,
	      std::vector<riscv_pending_register_alias> *aliases,
	      struct riscv_gdbarch_features *features) const
  {
    const struct tdesc_feature *feature_cpu = tdesc_feature (tdesc);

    if (feature_cpu == nullptr)
      return false;

    bool seen_an_optional_reg_p = false;
    for (const auto &reg : m_registers)
      {
	bool found = reg.check (tdesc_data, feature_cpu, true, aliases);

	bool is_optional_reg_p = (reg.regnum >= RISCV_ZERO_REGNUM + 16
				  && reg.regnum < RISCV_ZERO_REGNUM + 32);

	if (!found && (!is_optional_reg_p || seen_an_optional_reg_p))
	  return false;
	else if (found && is_optional_reg_p)
	  seen_an_optional_reg_p = true;
      }

    /* Check that all of the core cpu registers have the same bitsize.  */
    int xlen_bitsize = tdesc_register_bitsize (feature_cpu, "pc");

    bool valid_p = true;
    for (auto &tdesc_reg : feature_cpu->registers)
      valid_p &= (tdesc_reg->bitsize == xlen_bitsize);

    features->xlen = (xlen_bitsize / 8);
    features->embedded = !seen_an_optional_reg_p;

    return valid_p;
  }
};

/* An instance of the x-register feature set.  */

static const struct riscv_xreg_feature riscv_xreg_feature;

/* Class representing the f-registers feature set.  */

struct riscv_freg_feature : public riscv_register_feature
{
  riscv_freg_feature ()
    : riscv_register_feature (riscv_feature_name_fpu)
  {
    m_registers =  {
      { RISCV_FIRST_FP_REGNUM + 0, { "ft0", "f0" } },
      { RISCV_FIRST_FP_REGNUM + 1, { "ft1", "f1" } },
      { RISCV_FIRST_FP_REGNUM + 2, { "ft2", "f2" } },
      { RISCV_FIRST_FP_REGNUM + 3, { "ft3", "f3" } },
      { RISCV_FIRST_FP_REGNUM + 4, { "ft4", "f4" } },
      { RISCV_FIRST_FP_REGNUM + 5, { "ft5", "f5" } },
      { RISCV_FIRST_FP_REGNUM + 6, { "ft6", "f6" } },
      { RISCV_FIRST_FP_REGNUM + 7, { "ft7", "f7" } },
      { RISCV_FIRST_FP_REGNUM + 8, { "fs0", "f8" } },
      { RISCV_FIRST_FP_REGNUM + 9, { "fs1", "f9" } },
      { RISCV_FIRST_FP_REGNUM + 10, { "fa0", "f10" } },
      { RISCV_FIRST_FP_REGNUM + 11, { "fa1", "f11" } },
      { RISCV_FIRST_FP_REGNUM + 12, { "fa2", "f12" } },
      { RISCV_FIRST_FP_REGNUM + 13, { "fa3", "f13" } },
      { RISCV_FIRST_FP_REGNUM + 14, { "fa4", "f14" } },
      { RISCV_FIRST_FP_REGNUM + 15, { "fa5", "f15" } },
      { RISCV_FIRST_FP_REGNUM + 16, { "fa6", "f16" } },
      { RISCV_FIRST_FP_REGNUM + 17, { "fa7", "f17" } },
      { RISCV_FIRST_FP_REGNUM + 18, { "fs2", "f18" } },
      { RISCV_FIRST_FP_REGNUM + 19, { "fs3", "f19" } },
      { RISCV_FIRST_FP_REGNUM + 20, { "fs4", "f20" } },
      { RISCV_FIRST_FP_REGNUM + 21, { "fs5", "f21" } },
      { RISCV_FIRST_FP_REGNUM + 22, { "fs6", "f22" } },
      { RISCV_FIRST_FP_REGNUM + 23, { "fs7", "f23" } },
      { RISCV_FIRST_FP_REGNUM + 24, { "fs8", "f24" } },
      { RISCV_FIRST_FP_REGNUM + 25, { "fs9", "f25" } },
      { RISCV_FIRST_FP_REGNUM + 26, { "fs10", "f26" } },
      { RISCV_FIRST_FP_REGNUM + 27, { "fs11", "f27" } },
      { RISCV_FIRST_FP_REGNUM + 28, { "ft8", "f28" } },
      { RISCV_FIRST_FP_REGNUM + 29, { "ft9", "f29" } },
      { RISCV_FIRST_FP_REGNUM + 30, { "ft10", "f30" } },
      { RISCV_FIRST_FP_REGNUM + 31, { "ft11", "f31" } },
      { RISCV_CSR_FFLAGS_REGNUM, { "fflags", "csr1" } },
      { RISCV_CSR_FRM_REGNUM, { "frm", "csr2" } },
      { RISCV_CSR_FCSR_REGNUM, { "fcsr", "csr3" } },
    };
  }

  /* Return the preferred name for the register with gdb register number
     REGNUM, which must be in the inclusive range RISCV_FIRST_FP_REGNUM to
     RISCV_LAST_FP_REGNUM.  */
  const char *register_name (int regnum) const
  {
    static_assert (RISCV_LAST_FP_REGNUM == RISCV_FIRST_FP_REGNUM + 31);
    gdb_assert (regnum >= RISCV_FIRST_FP_REGNUM
		&& regnum <= RISCV_LAST_FP_REGNUM);
    regnum -= RISCV_FIRST_FP_REGNUM;
    return m_registers[regnum].names[0];
  }

  /* Check this feature within TDESC, record the registers from this
     feature into TDESC_DATA and update ALIASES and FEATURES.  */
  bool check (const struct target_desc *tdesc,
	      struct tdesc_arch_data *tdesc_data,
	      std::vector<riscv_pending_register_alias> *aliases,
	      struct riscv_gdbarch_features *features) const
  {
    const struct tdesc_feature *feature_fpu = tdesc_feature (tdesc);

    /* It's fine if this feature is missing.  Update the architecture
       feature set and return.  */
    if (feature_fpu == nullptr)
      {
	features->flen = 0;
	return true;
      }

    /* Check all of the floating pointer registers are present.  We also
       check that the floating point CSRs are present too, though if these
       are missing this is not fatal.  */
    for (const auto &reg : m_registers)
      {
	bool found = reg.check (tdesc_data, feature_fpu, true, aliases);

	bool is_ctrl_reg_p = reg.regnum > RISCV_LAST_FP_REGNUM;

	if (!found && !is_ctrl_reg_p)
	  return false;
      }

    /* Look through all of the floating point registers (not the FP CSRs
       though), and check they all have the same bitsize.  Use this bitsize
       to update the feature set for this gdbarch.  */
    int fp_bitsize = -1;
    for (const auto &reg : m_registers)
      {
	/* Stop once we get to the CSRs which are at the end of the
	   M_REGISTERS list.  */
	if (reg.regnum > RISCV_LAST_FP_REGNUM)
	  break;

	int reg_bitsize = -1;
	for (const char *name : reg.names)
	  {
	    if (tdesc_unnumbered_register (feature_fpu, name))
	      {
		reg_bitsize = tdesc_register_bitsize (feature_fpu, name);
		break;
	      }
	  }
	gdb_assert (reg_bitsize != -1);
	if (fp_bitsize == -1)
	  fp_bitsize = reg_bitsize;
	else if (fp_bitsize != reg_bitsize)
	  return false;
      }

    features->flen = (fp_bitsize / 8);
    return true;
  }
};

/* An instance of the f-register feature set.  */

static const struct riscv_freg_feature riscv_freg_feature;

/* Class representing the virtual registers.  These are not physical
   registers on the hardware, but might be available from the target.
   These are not pseudo registers, reading these really does result in a
   register read from the target, it is just that there might not be a
   physical register backing the result.  */

struct riscv_virtual_feature : public riscv_register_feature
{
  riscv_virtual_feature ()
    : riscv_register_feature (riscv_feature_name_virtual)
  {
    m_registers =  {
      { RISCV_PRIV_REGNUM, { "priv" } }
    };
  }

  bool check (const struct target_desc *tdesc,
	      struct tdesc_arch_data *tdesc_data,
	      std::vector<riscv_pending_register_alias> *aliases,
	      struct riscv_gdbarch_features *features) const
  {
    const struct tdesc_feature *feature_virtual = tdesc_feature (tdesc);

    /* It's fine if this feature is missing.  */
    if (feature_virtual == nullptr)
      return true;

    /* We don't check the return value from the call to check here, all the
       registers in this feature are optional.  */
    for (const auto &reg : m_registers)
      reg.check (tdesc_data, feature_virtual, true, aliases);

    return true;
  }
};

/* An instance of the virtual register feature.  */

static const struct riscv_virtual_feature riscv_virtual_feature;

/* Class representing the CSR feature.  */

struct riscv_csr_feature : public riscv_register_feature
{
  riscv_csr_feature ()
    : riscv_register_feature (riscv_feature_name_csr)
  {
    m_registers = {
#define DECLARE_CSR(NAME,VALUE,CLASS,DEFINE_VER,ABORT_VER)		\
      { RISCV_ ## VALUE ## _REGNUM, { # NAME } },
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
    };
    riscv_create_csr_aliases ();
  }

  bool check (const struct target_desc *tdesc,
	      struct tdesc_arch_data *tdesc_data,
	      std::vector<riscv_pending_register_alias> *aliases,
	      struct riscv_gdbarch_features *features) const
  {
    const struct tdesc_feature *feature_csr = tdesc_feature (tdesc);

    /* It's fine if this feature is missing.  */
    if (feature_csr == nullptr)
      return true;

    /* We don't check the return value from the call to check here, all the
       registers in this feature are optional.  */
    for (const auto &reg : m_registers)
      reg.check (tdesc_data, feature_csr, true, aliases);

    return true;
  }

private:

  /* Complete RISCV_CSR_FEATURE, building the CSR alias names and adding them
     to the name list for each register.  */

  void
  riscv_create_csr_aliases ()
  {
    for (auto &reg : m_registers)
      {
	int csr_num = reg.regnum - RISCV_FIRST_CSR_REGNUM;
	gdb::unique_xmalloc_ptr<char> alias = xstrprintf ("csr%d", csr_num);
	reg.names.push_back (alias.release ());
      }
  }
};

/* An instance of the csr register feature.  */

static const struct riscv_csr_feature riscv_csr_feature;

/* Class representing the v-registers feature set.  */

struct riscv_vector_feature : public riscv_register_feature
{
  riscv_vector_feature ()
    : riscv_register_feature (riscv_feature_name_vector)
  {
    m_registers =  {
      { RISCV_V0_REGNUM + 0, { "v0" } },
      { RISCV_V0_REGNUM + 1, { "v1" } },
      { RISCV_V0_REGNUM + 2, { "v2" } },
      { RISCV_V0_REGNUM + 3, { "v3" } },
      { RISCV_V0_REGNUM + 4, { "v4" } },
      { RISCV_V0_REGNUM + 5, { "v5" } },
      { RISCV_V0_REGNUM + 6, { "v6" } },
      { RISCV_V0_REGNUM + 7, { "v7" } },
      { RISCV_V0_REGNUM + 8, { "v8" } },
      { RISCV_V0_REGNUM + 9, { "v9" } },
      { RISCV_V0_REGNUM + 10, { "v10" } },
      { RISCV_V0_REGNUM + 11, { "v11" } },
      { RISCV_V0_REGNUM + 12, { "v12" } },
      { RISCV_V0_REGNUM + 13, { "v13" } },
      { RISCV_V0_REGNUM + 14, { "v14" } },
      { RISCV_V0_REGNUM + 15, { "v15" } },
      { RISCV_V0_REGNUM + 16, { "v16" } },
      { RISCV_V0_REGNUM + 17, { "v17" } },
      { RISCV_V0_REGNUM + 18, { "v18" } },
      { RISCV_V0_REGNUM + 19, { "v19" } },
      { RISCV_V0_REGNUM + 20, { "v20" } },
      { RISCV_V0_REGNUM + 21, { "v21" } },
      { RISCV_V0_REGNUM + 22, { "v22" } },
      { RISCV_V0_REGNUM + 23, { "v23" } },
      { RISCV_V0_REGNUM + 24, { "v24" } },
      { RISCV_V0_REGNUM + 25, { "v25" } },
      { RISCV_V0_REGNUM + 26, { "v26" } },
      { RISCV_V0_REGNUM + 27, { "v27" } },
      { RISCV_V0_REGNUM + 28, { "v28" } },
      { RISCV_V0_REGNUM + 29, { "v29" } },
      { RISCV_V0_REGNUM + 30, { "v30" } },
      { RISCV_V0_REGNUM + 31, { "v31" } },
    };
  }

  /* Return the preferred name for the register with gdb register number
     REGNUM, which must be in the inclusive range RISCV_V0_REGNUM to
     RISCV_V0_REGNUM + 31.  */
  const char *register_name (int regnum) const
  {
    gdb_assert (regnum >= RISCV_V0_REGNUM
		&& regnum <= RISCV_V0_REGNUM + 31);
    regnum -= RISCV_V0_REGNUM;
    return m_registers[regnum].names[0];
  }

  /* Check this feature within TDESC, record the registers from this
     feature into TDESC_DATA and update ALIASES and FEATURES.  */
  bool check (const struct target_desc *tdesc,
	      struct tdesc_arch_data *tdesc_data,
	      std::vector<riscv_pending_register_alias> *aliases,
	      struct riscv_gdbarch_features *features) const
  {
    const struct tdesc_feature *feature_vector = tdesc_feature (tdesc);

    /* It's fine if this feature is missing.  Update the architecture
       feature set and return.  */
    if (feature_vector == nullptr)
      {
	features->vlen = 0;
	return true;
      }

    /* Check all of the vector registers are present.  */
    for (const auto &reg : m_registers)
      {
	if (!reg.check (tdesc_data, feature_vector, true, aliases))
	  return false;
      }

    /* Look through all of the vector registers and check they all have the
       same bitsize.  Use this bitsize to update the feature set for this
       gdbarch.  */
    int vector_bitsize = -1;
    for (const auto &reg : m_registers)
      {
	int reg_bitsize = -1;
	for (const char *name : reg.names)
	  {
	    if (tdesc_unnumbered_register (feature_vector, name))
	      {
		reg_bitsize = tdesc_register_bitsize (feature_vector, name);
		break;
	      }
	  }
	gdb_assert (reg_bitsize != -1);
	if (vector_bitsize == -1)
	  vector_bitsize = reg_bitsize;
	else if (vector_bitsize != reg_bitsize)
	  return false;
      }

    features->vlen = (vector_bitsize / 8);
    return true;
  }
};

/* An instance of the v-register feature set.  */

static const struct riscv_vector_feature riscv_vector_feature;

/* Controls whether we place compressed breakpoints or not.  When in auto
   mode GDB tries to determine if the target supports compressed
   breakpoints, and uses them if it does.  */

static enum auto_boolean use_compressed_breakpoints;

/* The show callback for 'show riscv use-compressed-breakpoints'.  */

static void
show_use_compressed_breakpoints (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  gdb_printf (file,
	      _("Debugger's use of compressed breakpoints is set "
		"to %s.\n"), value);
}

/* The set and show lists for 'set riscv' and 'show riscv' prefixes.  */

static struct cmd_list_element *setriscvcmdlist = NULL;
static struct cmd_list_element *showriscvcmdlist = NULL;

/* The set and show lists for 'set riscv' and 'show riscv' prefixes.  */

static struct cmd_list_element *setdebugriscvcmdlist = NULL;
static struct cmd_list_element *showdebugriscvcmdlist = NULL;

/* The show callback for all 'show debug riscv VARNAME' variables.  */

static void
show_riscv_debug_variable (struct ui_file *file, int from_tty,
			   struct cmd_list_element *c,
			   const char *value)
{
  gdb_printf (file,
	      _("RiscV debug variable `%s' is set to: %s\n"),
	      c->name, value);
}

/* See riscv-tdep.h.  */

int
riscv_isa_xlen (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return tdep->isa_features.xlen;
}

/* See riscv-tdep.h.  */

int
riscv_abi_xlen (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return tdep->abi_features.xlen;
}

/* See riscv-tdep.h.  */

int
riscv_isa_flen (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return tdep->isa_features.flen;
}

/* See riscv-tdep.h.  */

int
riscv_abi_flen (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return tdep->abi_features.flen;
}

/* See riscv-tdep.h.  */

bool
riscv_abi_embedded (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return tdep->abi_features.embedded;
}

/* Return true if the target for GDBARCH has floating point hardware.  */

static bool
riscv_has_fp_regs (struct gdbarch *gdbarch)
{
  return (riscv_isa_flen (gdbarch) > 0);
}

/* Return true if GDBARCH is using any of the floating point hardware ABIs.  */

static bool
riscv_has_fp_abi (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return tdep->abi_features.flen > 0;
}

/* Return true if REGNO is a floating pointer register.  */

static bool
riscv_is_fp_regno_p (int regno)
{
  return (regno >= RISCV_FIRST_FP_REGNUM
	  && regno <= RISCV_LAST_FP_REGNUM);
}

/* Implement the breakpoint_kind_from_pc gdbarch method.  */

static int
riscv_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
{
  if (use_compressed_breakpoints == AUTO_BOOLEAN_AUTO)
    {
      bool unaligned_p = false;
      gdb_byte buf[1];

      /* Some targets don't support unaligned reads.  The address can only
	 be unaligned if the C extension is supported.  So it is safe to
	 use a compressed breakpoint in this case.  */
      if (*pcptr & 0x2)
	unaligned_p = true;
      else
	{
	  /* Read the opcode byte to determine the instruction length.  If
	     the read fails this may be because we tried to set the
	     breakpoint at an invalid address, in this case we provide a
	     fake result which will give a breakpoint length of 4.
	     Hopefully when we try to actually insert the breakpoint we
	     will see a failure then too which will be reported to the
	     user.  */
	  if (target_read_code (*pcptr, buf, 1) == -1)
	    buf[0] = 0;
	}

      if (riscv_debug_breakpoints)
	{
	  const char *bp = (unaligned_p || riscv_insn_length (buf[0]) == 2
			    ? "C.EBREAK" : "EBREAK");

	  std::string suffix;
	  if (unaligned_p)
	    suffix = "(unaligned address)";
	  else
	    suffix = string_printf ("(instruction length %d)",
				    riscv_insn_length (buf[0]));
	  riscv_breakpoints_debug_printf ("Using %s for breakpoint at %s %s",
					  bp, paddress (gdbarch, *pcptr),
					  suffix.c_str ());
	}
      if (unaligned_p || riscv_insn_length (buf[0]) == 2)
	return 2;
      else
	return 4;
    }
  else if (use_compressed_breakpoints == AUTO_BOOLEAN_TRUE)
    return 2;
  else
    return 4;
}

/* Implement the sw_breakpoint_from_kind gdbarch method.  */

static const gdb_byte *
riscv_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size)
{
  static const gdb_byte ebreak[] = { 0x73, 0x00, 0x10, 0x00, };
  static const gdb_byte c_ebreak[] = { 0x02, 0x90 };

  *size = kind;
  switch (kind)
    {
    case 2:
      return c_ebreak;
    case 4:
      return ebreak;
    default:
      gdb_assert_not_reached ("unhandled breakpoint kind");
    }
}

/* Implement the register_name gdbarch method.  This is used instead of
   the function supplied by calling TDESC_USE_REGISTERS so that we can
   ensure the preferred names are offered for x-regs and f-regs.  */

static const char *
riscv_register_name (struct gdbarch *gdbarch, int regnum)
{
  /* Lookup the name through the target description.  If we get back NULL
     then this is an unknown register.  If we do get a name back then we
     look up the registers preferred name below.  */
  const char *name = tdesc_register_name (gdbarch, regnum);
  gdb_assert (name != nullptr);
  if (name[0] == '\0')
    return name;

  /* We want GDB to use the ABI names for registers even if the target
     gives us a target description with the architectural name.  For
     example we want to see 'ra' instead of 'x1' whatever the target
     description called it.  */
  if (regnum >= RISCV_ZERO_REGNUM && regnum < RISCV_FIRST_FP_REGNUM)
    return riscv_xreg_feature.register_name (regnum);

  /* Like with the x-regs we prefer the abi names for the floating point
     registers.  If the target doesn't have floating point registers then
     the tdesc_register_name call above should have returned an empty
     string.  */
  if (regnum >= RISCV_FIRST_FP_REGNUM && regnum <= RISCV_LAST_FP_REGNUM)
    {
      gdb_assert (riscv_has_fp_regs (gdbarch));
      return riscv_freg_feature.register_name (regnum);
    }

  /* Some targets (QEMU) are reporting these three registers twice, once
     in the FPU feature, and once in the CSR feature.  Both of these read
     the same underlying state inside the target, but naming the register
     twice in the target description results in GDB having two registers
     with the same name, only one of which can ever be accessed, but both
     will show up in 'info register all'.  Unless, we identify the
     duplicate copies of these registers (in riscv_tdesc_unknown_reg) and
     then hide the registers here by giving them no name.  */
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  if (tdep->duplicate_fflags_regnum == regnum
      || tdep->duplicate_frm_regnum == regnum
      || tdep->duplicate_fcsr_regnum == regnum)
    return "";

  /* The remaining registers are different.  For all other registers on the
     machine we prefer to see the names that the target description
     provides.  This is particularly important for CSRs which might be
     renamed over time.  If GDB keeps track of the "latest" name, but a
     particular target provides an older name then we don't want to force
     users to see the newer name in register output.

     The other case that reaches here are any registers that the target
     provided that GDB is completely unaware of.  For these we have no
     choice but to accept the target description name.

     Just accept whatever name TDESC_REGISTER_NAME returned.  */
  return name;
}

/* Implement gdbarch_pseudo_register_read.  Read pseudo-register REGNUM
   from REGCACHE and place the register value into BUF.  BUF is sized
   based on the type of register REGNUM, all of BUF should be written too,
   the result should be sign or zero extended as appropriate.  */

static enum register_status
riscv_pseudo_register_read (struct gdbarch *gdbarch,
			    readable_regcache *regcache,
			    int regnum, gdb_byte *buf)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  if (regnum == tdep->fflags_regnum || regnum == tdep->frm_regnum)
    {
      /* Clear BUF.  */
      memset (buf, 0, register_size (gdbarch, regnum));

      /* Read the first byte of the fcsr register, this contains both frm
	 and fflags.  */
      enum register_status status
	= regcache->raw_read_part (RISCV_CSR_FCSR_REGNUM, 0, 1, buf);

      if (status != REG_VALID)
	return status;

      /* Extract the appropriate parts.  */
      if (regnum == tdep->fflags_regnum)
	buf[0] &= 0x1f;
      else if (regnum == tdep->frm_regnum)
	buf[0] = (buf[0] >> 5) & 0x7;

      return REG_VALID;
    }

  return REG_UNKNOWN;
}

/* Implement gdbarch_deprecated_pseudo_register_write.  Write the contents of
   BUF into pseudo-register REGNUM in REGCACHE.  BUF is sized based on the type
   of register REGNUM.  */

static void
riscv_pseudo_register_write (struct gdbarch *gdbarch,
			     struct regcache *regcache, int regnum,
			     const gdb_byte *buf)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  if (regnum == tdep->fflags_regnum || regnum == tdep->frm_regnum)
    {
      int fcsr_regnum = RISCV_CSR_FCSR_REGNUM;
      gdb_byte raw_buf[register_size (gdbarch, fcsr_regnum)];

      regcache->raw_read (fcsr_regnum, raw_buf);

      if (regnum == tdep->fflags_regnum)
	raw_buf[0] = (raw_buf[0] & ~0x1f) | (buf[0] & 0x1f);
      else if (regnum == tdep->frm_regnum)
	raw_buf[0] = (raw_buf[0] & ~(0x7 << 5)) | ((buf[0] & 0x7) << 5);

      regcache->raw_write (fcsr_regnum, raw_buf);
    }
  else
    gdb_assert_not_reached ("unknown pseudo register %d", regnum);
}

/* Implement the cannot_store_register gdbarch method.  The zero register
   (x0) is read-only on RISC-V.  */

static int
riscv_cannot_store_register (struct gdbarch *gdbarch, int regnum)
{
  return regnum == RISCV_ZERO_REGNUM;
}

/* Construct a type for 64-bit FP registers.  */

static struct type *
riscv_fpreg_d_type (struct gdbarch *gdbarch)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  if (tdep->riscv_fpreg_d_type == nullptr)
    {
      const struct builtin_type *bt = builtin_type (gdbarch);

      /* The type we're building is this: */
#if 0
      union __gdb_builtin_type_fpreg_d
      {
	float f;
	double d;
      };
#endif

      struct type *t;

      t = arch_composite_type (gdbarch,
			       "__gdb_builtin_type_fpreg_d", TYPE_CODE_UNION);
      append_composite_type_field (t, "float", bt->builtin_float);
      append_composite_type_field (t, "double", bt->builtin_double);
      t->set_is_vector (true);
      t->set_name ("builtin_type_fpreg_d");
      tdep->riscv_fpreg_d_type = t;
    }

  return tdep->riscv_fpreg_d_type;
}

/* Implement the register_type gdbarch method.  This is installed as an
   for the override setup by TDESC_USE_REGISTERS, for most registers we
   delegate the type choice to the target description, but for a few
   registers we try to improve the types if the target description has
   taken a simplistic approach.  */

static struct type *
riscv_register_type (struct gdbarch *gdbarch, int regnum)
{
  struct type *type = tdesc_register_type (gdbarch, regnum);
  int xlen = riscv_isa_xlen (gdbarch);

  /* We want to perform some specific type "fixes" in cases where we feel
     that we really can do better than the target description.  For all
     other cases we just return what the target description says.  */
  if (riscv_is_fp_regno_p (regnum))
    {
      /* This spots the case for RV64 where the double is defined as
	 either 'ieee_double' or 'float' (which is the generic name that
	 converts to 'double' on 64-bit).  In these cases its better to
	 present the registers using a union type.  */
      int flen = riscv_isa_flen (gdbarch);
      if (flen == 8
	  && type->code () == TYPE_CODE_FLT
	  && type->length () == flen
	  && (strcmp (type->name (), "builtin_type_ieee_double") == 0
	      || strcmp (type->name (), "double") == 0))
	type = riscv_fpreg_d_type (gdbarch);
    }

  if ((regnum == gdbarch_pc_regnum (gdbarch)
       || regnum == RISCV_RA_REGNUM
       || regnum == RISCV_FP_REGNUM
       || regnum == RISCV_SP_REGNUM
       || regnum == RISCV_GP_REGNUM
       || regnum == RISCV_TP_REGNUM)
      && type->code () == TYPE_CODE_INT
      && type->length () == xlen)
    {
      /* This spots the case where some interesting registers are defined
	 as simple integers of the expected size, we force these registers
	 to be pointers as we believe that is more useful.  */
      if (regnum == gdbarch_pc_regnum (gdbarch)
	  || regnum == RISCV_RA_REGNUM)
	type = builtin_type (gdbarch)->builtin_func_ptr;
      else if (regnum == RISCV_FP_REGNUM
	       || regnum == RISCV_SP_REGNUM
	       || regnum == RISCV_GP_REGNUM
	       || regnum == RISCV_TP_REGNUM)
	type = builtin_type (gdbarch)->builtin_data_ptr;
    }

  return type;
}

/* Helper for riscv_print_registers_info, prints info for a single register
   REGNUM.  */

static void
riscv_print_one_register_info (struct gdbarch *gdbarch,
			       struct ui_file *file,
			       frame_info_ptr frame,
			       int regnum)
{
  const char *name = gdbarch_register_name (gdbarch, regnum);
  struct value *val;
  struct type *regtype;
  int print_raw_format;
  enum tab_stops { value_column_1 = 15 };

  gdb_puts (name, file);
  print_spaces (std::max<int> (1, value_column_1 - strlen (name)), file);

  try
    {
      val = value_of_register (regnum, get_next_frame_sentinel_okay (frame));
      regtype = val->type ();
    }
  catch (const gdb_exception_error &ex)
    {
      /* Handle failure to read a register without interrupting the entire
	 'info registers' flow.  */
      gdb_printf (file, "%s\n", ex.what ());
      return;
    }

  print_raw_format = (val->entirely_available ()
		      && !val->optimized_out ());

  if (regtype->code () == TYPE_CODE_FLT
      || (regtype->code () == TYPE_CODE_UNION
	  && regtype->num_fields () == 2
	  && regtype->field (0).type ()->code () == TYPE_CODE_FLT
	  && regtype->field (1).type ()->code () == TYPE_CODE_FLT)
      || (regtype->code () == TYPE_CODE_UNION
	  && regtype->num_fields () == 3
	  && regtype->field (0).type ()->code () == TYPE_CODE_FLT
	  && regtype->field (1).type ()->code () == TYPE_CODE_FLT
	  && regtype->field (2).type ()->code () == TYPE_CODE_FLT))
    {
      struct value_print_options opts;
      const gdb_byte *valaddr = val->contents_for_printing ().data ();
      enum bfd_endian byte_order = type_byte_order (regtype);

      get_user_print_options (&opts);
      opts.deref_ref = true;

      common_val_print (val, file, 0, &opts, current_language);

      if (print_raw_format)
	{
	  gdb_printf (file, "\t(raw ");
	  print_hex_chars (file, valaddr, regtype->length (), byte_order,
			   true);
	  gdb_printf (file, ")");
	}
    }
  else
    {
      struct value_print_options opts;
      riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

      /* Print the register in hex.  */
      get_formatted_print_options (&opts, 'x');
      opts.deref_ref = true;
      common_val_print (val, file, 0, &opts, current_language);

      if (print_raw_format)
	{
	  if (regnum == RISCV_CSR_MSTATUS_REGNUM)
	    {
	      LONGEST d;
	      int size = register_size (gdbarch, regnum);
	      unsigned xlen;

	      /* The SD field is always in the upper bit of MSTATUS, regardless
		 of the number of bits in MSTATUS.  */
	      d = value_as_long (val);
	      xlen = size * 8;
	      gdb_printf (file,
			  "\tSD:%X VM:%02X MXR:%X PUM:%X MPRV:%X XS:%X "
			  "FS:%X MPP:%x HPP:%X SPP:%X MPIE:%X HPIE:%X "
			  "SPIE:%X UPIE:%X MIE:%X HIE:%X SIE:%X UIE:%X",
			  (int) ((d >> (xlen - 1)) & 0x1),
			  (int) ((d >> 24) & 0x1f),
			  (int) ((d >> 19) & 0x1),
			  (int) ((d >> 18) & 0x1),
			  (int) ((d >> 17) & 0x1),
			  (int) ((d >> 15) & 0x3),
			  (int) ((d >> 13) & 0x3),
			  (int) ((d >> 11) & 0x3),
			  (int) ((d >> 9) & 0x3),
			  (int) ((d >> 8) & 0x1),
			  (int) ((d >> 7) & 0x1),
			  (int) ((d >> 6) & 0x1),
			  (int) ((d >> 5) & 0x1),
			  (int) ((d >> 4) & 0x1),
			  (int) ((d >> 3) & 0x1),
			  (int) ((d >> 2) & 0x1),
			  (int) ((d >> 1) & 0x1),
			  (int) ((d >> 0) & 0x1));
	    }
	  else if (regnum == RISCV_CSR_MISA_REGNUM)
	    {
	      int base;
	      unsigned xlen, i;
	      LONGEST d;
	      int size = register_size (gdbarch, regnum);

	      /* The MXL field is always in the upper two bits of MISA,
		 regardless of the number of bits in MISA.  Mask out other
		 bits to ensure we have a positive value.  */
	      d = value_as_long (val);
	      base = (d >> ((size * 8) - 2)) & 0x3;
	      xlen = 16;

	      for (; base > 0; base--)
		xlen *= 2;
	      gdb_printf (file, "\tRV%d", xlen);

	      for (i = 0; i < 26; i++)
		{
		  if (d & (1 << i))
		    gdb_printf (file, "%c", 'A' + i);
		}
	    }
	  else if (regnum == RISCV_CSR_FCSR_REGNUM
		   || regnum == tdep->fflags_regnum
		   || regnum == tdep->frm_regnum)
	    {
	      LONGEST d = value_as_long (val);

	      gdb_printf (file, "\t");
	      if (regnum != tdep->frm_regnum)
		gdb_printf (file,
			    "NV:%d DZ:%d OF:%d UF:%d NX:%d",
			    (int) ((d >> 4) & 0x1),
			    (int) ((d >> 3) & 0x1),
			    (int) ((d >> 2) & 0x1),
			    (int) ((d >> 1) & 0x1),
			    (int) ((d >> 0) & 0x1));

	      if (regnum != tdep->fflags_regnum)
		{
		  static const char * const sfrm[] =
		    {
		      _("RNE (round to nearest; ties to even)"),
		      _("RTZ (Round towards zero)"),
		      _("RDN (Round down towards -INF)"),
		      _("RUP (Round up towards +INF)"),
		      _("RMM (Round to nearest; ties to max magnitude)"),
		      _("INVALID[5]"),
		      _("INVALID[6]"),
		      /* A value of 0x7 indicates dynamic rounding mode when
			 used within an instructions rounding-mode field, but
			 is invalid within the FRM register.  */
		      _("INVALID[7] (Dynamic rounding mode)"),
		    };
		  int frm = ((regnum == RISCV_CSR_FCSR_REGNUM)
			     ? (d >> 5) : d) & 0x7;

		  gdb_printf (file, "%sFRM:%i [%s]",
			      (regnum == RISCV_CSR_FCSR_REGNUM
			       ? " " : ""),
			      frm, sfrm[frm]);
		}
	    }
	  else if (regnum == RISCV_PRIV_REGNUM)
	    {
	      LONGEST d;
	      uint8_t priv;

	      d = value_as_long (val);
	      priv = d & 0xff;

	      if (priv < 4)
		{
		  static const char * const sprv[] =
		    {
		      "User/Application",
		      "Supervisor",
		      "Hypervisor",
		      "Machine"
		    };
		  gdb_printf (file, "\tprv:%d [%s]",
			      priv, sprv[priv]);
		}
	      else
		gdb_printf (file, "\tprv:%d [INVALID]", priv);
	    }
	  else
	    {
	      /* If not a vector register, print it also according to its
		 natural format.  */
	      if (regtype->is_vector () == 0)
		{
		  get_user_print_options (&opts);
		  opts.deref_ref = true;
		  gdb_printf (file, "\t");
		  common_val_print (val, file, 0, &opts, current_language);
		}
	    }
	}
    }
  gdb_printf (file, "\n");
}

/* Return true if REGNUM is a valid CSR register.  The CSR register space
   is sparsely populated, so not every number is a named CSR.  */

static bool
riscv_is_regnum_a_named_csr (int regnum)
{
  gdb_assert (regnum >= RISCV_FIRST_CSR_REGNUM
	      && regnum <= RISCV_LAST_CSR_REGNUM);

  switch (regnum)
    {
#define DECLARE_CSR(name, num, class, define_ver, abort_ver) case RISCV_ ## num ## _REGNUM:
#include "opcode/riscv-opc.h"
#undef DECLARE_CSR
      return true;

    default:
      return false;
    }
}

/* Return true if REGNUM is an unknown CSR identified in
   riscv_tdesc_unknown_reg for GDBARCH.  */

static bool
riscv_is_unknown_csr (struct gdbarch *gdbarch, int regnum)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  return (regnum >= tdep->unknown_csrs_first_regnum
	  && regnum < (tdep->unknown_csrs_first_regnum
		       + tdep->unknown_csrs_count));
}

/* Implement the register_reggroup_p gdbarch method.  Is REGNUM a member
   of REGGROUP?  */

static int
riscv_register_reggroup_p (struct gdbarch  *gdbarch, int regnum,
			   const struct reggroup *reggroup)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  /* Used by 'info registers' and 'info registers <groupname>'.  */

  if (gdbarch_register_name (gdbarch, regnum)[0] == '\0')
    return 0;

  if (regnum > RISCV_LAST_REGNUM && regnum < gdbarch_num_regs (gdbarch))
    {
      /* Any extra registers from the CSR tdesc_feature (identified in
	 riscv_tdesc_unknown_reg) are removed from the save/restore groups
	 as some targets (QEMU) report CSRs which then can't be read and
	 having unreadable registers in the save/restore group breaks
	 things like inferior calls.

	 The unknown CSRs are also removed from the general group, and
	 added into both the csr and system group.  This is inline with the
	 known CSRs (see below).  */
      if (riscv_is_unknown_csr (gdbarch, regnum))
	{
	  if (reggroup == restore_reggroup || reggroup == save_reggroup
	       || reggroup == general_reggroup)
	    return 0;
	  else if (reggroup == system_reggroup || reggroup == csr_reggroup)
	    return 1;
	}

      /* This is some other unknown register from the target description.
	 In this case we trust whatever the target description says about
	 which groups this register should be in.  */
      int ret = tdesc_register_in_reggroup_p (gdbarch, regnum, reggroup);
      if (ret != -1)
	return ret;

      return default_register_reggroup_p (gdbarch, regnum, reggroup);
    }

  if (reggroup == all_reggroup)
    {
      if (regnum < RISCV_FIRST_CSR_REGNUM || regnum >= RISCV_PRIV_REGNUM)
	return 1;
      if (riscv_is_regnum_a_named_csr (regnum))
	return 1;
      return 0;
    }
  else if (reggroup == float_reggroup)
    return (riscv_is_fp_regno_p (regnum)
	    || regnum == RISCV_CSR_FCSR_REGNUM
	    || regnum == tdep->fflags_regnum
	    || regnum == tdep->frm_regnum);
  else if (reggroup == general_reggroup)
    return regnum < RISCV_FIRST_FP_REGNUM;
  else if (reggroup == restore_reggroup || reggroup == save_reggroup)
    {
      if (riscv_has_fp_regs (gdbarch))
	return (regnum <= RISCV_LAST_FP_REGNUM
		|| regnum == RISCV_CSR_FCSR_REGNUM
		|| regnum == tdep->fflags_regnum
		|| regnum == tdep->frm_regnum);
      else
	return regnum < RISCV_FIRST_FP_REGNUM;
    }
  else if (reggroup == system_reggroup || reggroup == csr_reggroup)
    {
      if (regnum == RISCV_PRIV_REGNUM)
	return 1;
      if (regnum < RISCV_FIRST_CSR_REGNUM || regnum > RISCV_LAST_CSR_REGNUM)
	return 0;
      if (riscv_is_regnum_a_named_csr (regnum))
	return 1;
      return 0;
    }
  else if (reggroup == vector_reggroup)
    return (regnum >= RISCV_V0_REGNUM && regnum <= RISCV_V31_REGNUM);
  else
    return 0;
}

/* Return the name for pseudo-register REGNUM for GDBARCH.  */

static const char *
riscv_pseudo_register_name (struct gdbarch *gdbarch, int regnum)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  if (regnum == tdep->fflags_regnum)
    return "fflags";
  else if (regnum == tdep->frm_regnum)
    return "frm";
  else
    gdb_assert_not_reached ("unknown pseudo register number %d", regnum);
}

/* Return the type for pseudo-register REGNUM for GDBARCH.  */

static struct type *
riscv_pseudo_register_type (struct gdbarch *gdbarch, int regnum)
{
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  if (regnum == tdep->fflags_regnum || regnum == tdep->frm_regnum)
   return builtin_type (gdbarch)->builtin_int32;
  else
    gdb_assert_not_reached ("unknown pseudo register number %d", regnum);
}

/* Return true (non-zero) if pseudo-register REGNUM from GDBARCH is a
   member of REGGROUP, otherwise return false (zero).  */

static int
riscv_pseudo_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
				  const struct reggroup *reggroup)
{
  /* The standard function will also work for pseudo-registers.  */
  return riscv_register_reggroup_p (gdbarch, regnum, reggroup);
}

/* Implement the print_registers_info gdbarch method.  This is used by
   'info registers' and 'info all-registers'.  */

static void
riscv_print_registers_info (struct gdbarch *gdbarch,
			    struct ui_file *file,
			    frame_info_ptr frame,
			    int regnum, int print_all)
{
  if (regnum != -1)
    {
      /* Print one specified register.  */
      if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	error (_("Not a valid register for the current processor type"));
      riscv_print_one_register_info (gdbarch, file, frame, regnum);
    }
  else
    {
      const struct reggroup *reggroup;

      if (print_all)
	reggroup = all_reggroup;
      else
	reggroup = general_reggroup;

      for (regnum = 0; regnum < gdbarch_num_cooked_regs (gdbarch); ++regnum)
	{
	  /* Zero never changes, so might as well hide by default.  */
	  if (regnum == RISCV_ZERO_REGNUM && !print_all)
	    continue;

	  /* Registers with no name are not valid on this ISA.  */
	  if (*(gdbarch_register_name (gdbarch, regnum)) == '\0')
	    continue;

	  /* Is the register in the group we're interested in?  */
	  if (!gdbarch_register_reggroup_p (gdbarch, regnum, reggroup))
	    continue;

	  riscv_print_one_register_info (gdbarch, file, frame, regnum);
	}
    }
}

/* Class that handles one decoded RiscV instruction.  */

class riscv_insn
{
public:

  /* Enum of all the opcodes that GDB cares about during the prologue scan.  */
  enum opcode
    {
      /* Unknown value is used at initialisation time.  */
      UNKNOWN = 0,

      /* These instructions are all the ones we are interested in during the
	 prologue scan.  */
      ADD,
      ADDI,
      ADDIW,
      ADDW,
      AUIPC,
      LUI,
      LI,
      SD,
      SW,
      LD,
      LW,
      MV,
      /* These are needed for software breakpoint support.  */
      JAL,
      JALR,
      BEQ,
      BNE,
      BLT,
      BGE,
      BLTU,
      BGEU,
      /* These are needed for stepping over atomic sequences.  */
      SLTI,
      SLTIU,
      XORI,
      ORI,
      ANDI,
      SLLI,
      SLLIW,
      SRLI,
      SRLIW,
      SRAI,
      SRAIW,
      SUB,
      SUBW,
      SLL,
      SLLW,
      SLT,
      SLTU,
      XOR,
      SRL,
      SRLW,
      SRA,
      SRAW,
      OR,
      AND,
      LR_W,
      LR_D,
      SC_W,
      SC_D,
      /* This instruction is used to do a syscall.  */
      ECALL,

      /* Other instructions are not interesting during the prologue scan, and
	 are ignored.  */
      OTHER
    };

  riscv_insn ()
    : m_length (0),
      m_opcode (OTHER),
      m_rd (0),
      m_rs1 (0),
      m_rs2 (0)
  {
    /* Nothing.  */
  }

  void decode (struct gdbarch *gdbarch, CORE_ADDR pc);

  /* Get the length of the instruction in bytes.  */
  int length () const
  { return m_length; }

  /* Get the opcode for this instruction.  */
  enum opcode opcode () const
  { return m_opcode; }

  /* Get destination register field for this instruction.  This is only
     valid if the OPCODE implies there is such a field for this
     instruction.  */
  int rd () const
  { return m_rd; }

  /* Get the RS1 register field for this instruction.  This is only valid
     if the OPCODE implies there is such a field for this instruction.  */
  int rs1 () const
  { return m_rs1; }

  /* Get the RS2 register field for this instruction.  This is only valid
     if the OPCODE implies there is such a field for this instruction.  */
  int rs2 () const
  { return m_rs2; }

  /* Get the immediate for this instruction in signed form.  This is only
     valid if the OPCODE implies there is such a field for this
     instruction.  */
  int imm_signed () const
  { return m_imm.s; }

private:

  /* Extract 5 bit register field at OFFSET from instruction OPCODE.  */
  int decode_register_index (unsigned long opcode, int offset)
  {
    return (opcode >> offset) & 0x1F;
  }

  /* Extract 5 bit register field at OFFSET from instruction OPCODE.  */
  int decode_register_index_short (unsigned long opcode, int offset)
  {
    return ((opcode >> offset) & 0x7) + 8;
  }

  /* Helper for DECODE, decode 32-bit R-type instruction.  */
  void decode_r_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_rs2 = decode_register_index (ival, OP_SH_RS2);
  }

  /* Helper for DECODE, decode 16-bit compressed R-type instruction.  */
  void decode_cr_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = m_rs1 = decode_register_index (ival, OP_SH_CRS1S);
    m_rs2 = decode_register_index (ival, OP_SH_CRS2);
  }

  /* Helper for DECODE, decode 32-bit I-type instruction.  */
  void decode_i_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_imm.s = EXTRACT_ITYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 16-bit compressed I-type instruction.  Some
     of the CI instruction have a hard-coded rs1 register, while others
     just use rd for both the source and destination.  RS1_REGNUM, if
     passed, is the value to place in rs1, otherwise rd is duplicated into
     rs1.  */
  void decode_ci_type_insn (enum opcode opcode, ULONGEST ival,
			    std::optional<int> rs1_regnum = {})
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_CRS1S);
    if (rs1_regnum.has_value ())
      m_rs1 = *rs1_regnum;
    else
      m_rs1 = m_rd;
    m_imm.s = EXTRACT_CITYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 16-bit compressed CL-type instruction.  */
  void decode_cl_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index_short (ival, OP_SH_CRS2S);
    m_rs1 = decode_register_index_short (ival, OP_SH_CRS1S);
    m_imm.s = EXTRACT_CLTYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 32-bit S-type instruction.  */
  void decode_s_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_rs2 = decode_register_index (ival, OP_SH_RS2);
    m_imm.s = EXTRACT_STYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 16-bit CS-type instruction.  The immediate
     encoding is different for each CS format instruction, so extracting
     the immediate is left up to the caller, who should pass the extracted
     immediate value through in IMM.  */
  void decode_cs_type_insn (enum opcode opcode, ULONGEST ival, int imm)
  {
    m_opcode = opcode;
    m_imm.s = imm;
    m_rs1 = decode_register_index_short (ival, OP_SH_CRS1S);
    m_rs2 = decode_register_index_short (ival, OP_SH_CRS2S);
  }

  /* Helper for DECODE, decode 16-bit CSS-type instruction.  The immediate
     encoding is different for each CSS format instruction, so extracting
     the immediate is left up to the caller, who should pass the extracted
     immediate value through in IMM.  */
  void decode_css_type_insn (enum opcode opcode, ULONGEST ival, int imm)
  {
    m_opcode = opcode;
    m_imm.s = imm;
    m_rs1 = RISCV_SP_REGNUM;
    /* Not a compressed register number in this case.  */
    m_rs2 = decode_register_index (ival, OP_SH_CRS2);
  }

  /* Helper for DECODE, decode 32-bit U-type instruction.  */
  void decode_u_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_imm.s = EXTRACT_UTYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 32-bit J-type instruction.  */
  void decode_j_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rd = decode_register_index (ival, OP_SH_RD);
    m_imm.s = EXTRACT_JTYPE_IMM (ival);
  }

  /* Helper for DECODE, decode 32-bit J-type instruction.  */
  void decode_cj_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_imm.s = EXTRACT_CJTYPE_IMM (ival);
  }

  void decode_b_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index (ival, OP_SH_RS1);
    m_rs2 = decode_register_index (ival, OP_SH_RS2);
    m_imm.s = EXTRACT_BTYPE_IMM (ival);
  }

  void decode_cb_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index_short (ival, OP_SH_CRS1S);
    m_imm.s = EXTRACT_CBTYPE_IMM (ival);
  }

  void decode_ca_type_insn (enum opcode opcode, ULONGEST ival)
  {
    m_opcode = opcode;
    m_rs1 = decode_register_index_short (ival, OP_SH_CRS1S);
    m_rs2 = decode_register_index_short (ival, OP_SH_CRS2S);
  }

  /* Fetch instruction from target memory at ADDR, return the content of
     the instruction, and update LEN with the instruction length.  */
  static ULONGEST fetch_instruction (struct gdbarch *gdbarch,
				     CORE_ADDR addr, int *len);

  /* The length of the instruction in bytes.  Should be 2 or 4.  */
  int m_length;

  /* The instruction opcode.  */
  enum opcode m_opcode;

  /* The three possible registers an instruction might reference.  Not
     every instruction fills in all of these registers.  Which fields are
     valid depends on the opcode.  The naming of these fields matches the
     naming in the riscv isa manual.  */
  int m_rd;
  int m_rs1;
  int m_rs2;

  /* Possible instruction immediate.  This is only valid if the instruction
     format contains an immediate, not all instruction, whether this is
     valid depends on the opcode.  Despite only having one format for now
     the immediate is packed into a union, later instructions might require
     an unsigned formatted immediate, having the union in place now will
     reduce the need for code churn later.  */
  union riscv_insn_immediate
  {
    riscv_insn_immediate ()
      : s (0)
    {
      /* Nothing.  */
    }

    int s;
  } m_imm;
};

/* Fetch instruction from target memory at ADDR, return the content of the
   instruction, and update LEN with the instruction length.  */

ULONGEST
riscv_insn::fetch_instruction (struct gdbarch *gdbarch,
			       CORE_ADDR addr, int *len)
{
  gdb_byte buf[RISCV_MAX_INSN_LEN];
  int instlen, status;

  /* All insns are at least 16 bits.  */
  status = target_read_memory (addr, buf, 2);
  if (status)
    memory_error (TARGET_XFER_E_IO, addr);

  /* If we need more, grab it now.  */
  instlen = riscv_insn_length (buf[0]);
  gdb_assert (instlen <= sizeof (buf));
  *len = instlen;

  if (instlen > 2)
    {
      status = target_read_memory (addr + 2, buf + 2, instlen - 2);
      if (status)
	memory_error (TARGET_XFER_E_IO, addr + 2);
    }

  /* RISC-V Specification states instructions are always little endian */
  return extract_unsigned_integer (buf, instlen, BFD_ENDIAN_LITTLE);
}

/* Fetch from target memory an instruction at PC and decode it.  This can
   throw an error if the memory access fails, callers are responsible for
   handling this error if that is appropriate.  */

void
riscv_insn::decode (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  ULONGEST ival;

  /* Fetch the instruction, and the instructions length.  */
  ival = fetch_instruction (gdbarch, pc, &m_length);

  if (m_length == 4)
    {
      if (is_add_insn (ival))
	decode_r_type_insn (ADD, ival);
      else if (is_addw_insn (ival))
	decode_r_type_insn (ADDW, ival);
      else if (is_addi_insn (ival))
	decode_i_type_insn (ADDI, ival);
      else if (is_addiw_insn (ival))
	decode_i_type_insn (ADDIW, ival);
      else if (is_auipc_insn (ival))
	decode_u_type_insn (AUIPC, ival);
      else if (is_lui_insn (ival))
	decode_u_type_insn (LUI, ival);
      else if (is_sd_insn (ival))
	decode_s_type_insn (SD, ival);
      else if (is_sw_insn (ival))
	decode_s_type_insn (SW, ival);
      else if (is_jal_insn (ival))
	decode_j_type_insn (JAL, ival);
      else if (is_jalr_insn (ival))
	decode_i_type_insn (JALR, ival);
      else if (is_beq_insn (ival))
	decode_b_type_insn (BEQ, ival);
      else if (is_bne_insn (ival))
	decode_b_type_insn (BNE, ival);
      else if (is_blt_insn (ival))
	decode_b_type_insn (BLT, ival);
      else if (is_bge_insn (ival))
	decode_b_type_insn (BGE, ival);
      else if (is_bltu_insn (ival))
	decode_b_type_insn (BLTU, ival);
      else if (is_bgeu_insn (ival))
	decode_b_type_insn (BGEU, ival);
      else if (is_slti_insn(ival))
	decode_i_type_insn (SLTI, ival);
      else if (is_sltiu_insn(ival))
	decode_i_type_insn (SLTIU, ival);
      else if (is_xori_insn(ival))
	decode_i_type_insn (XORI, ival);
      else if (is_ori_insn(ival))
	decode_i_type_insn (ORI, ival);
      else if (is_andi_insn(ival))
	decode_i_type_insn (ANDI, ival);
      else if (is_slli_insn(ival))
	decode_i_type_insn (SLLI, ival);
      else if (is_slliw_insn(ival))
	decode_i_type_insn (SLLIW, ival);
      else if (is_srli_insn(ival))
	decode_i_type_insn (SRLI, ival);
      else if (is_srliw_insn(ival))
	decode_i_type_insn (SRLIW, ival);
      else if (is_srai_insn(ival))
	decode_i_type_insn (SRAI, ival);
      else if (is_sraiw_insn(ival))
	decode_i_type_insn (SRAIW, ival);
      else if (is_sub_insn(ival))
	decode_r_type_insn (SUB, ival);
      else if (is_subw_insn(ival))
	decode_r_type_insn (SUBW, ival);
      else if (is_sll_insn(ival))
	decode_r_type_insn (SLL, ival);
      else if (is_sllw_insn(ival))
	decode_r_type_insn (SLLW, ival);
      else if (is_slt_insn(ival))
	decode_r_type_insn (SLT, ival);
      else if (is_sltu_insn(ival))
	decode_r_type_insn (SLTU, ival);
      else if (is_xor_insn(ival))
	decode_r_type_insn (XOR, ival);
      else if (is_srl_insn(ival))
	decode_r_type_insn (SRL, ival);
      else if (is_srlw_insn(ival))
	decode_r_type_insn (SRLW, ival);
      else if (is_sra_insn(ival))
	decode_r_type_insn (SRA, ival);
      else if (is_sraw_insn(ival))
	decode_r_type_insn (SRAW, ival);
      else if (is_or_insn(ival))
	decode_r_type_insn (OR, ival);
      else if (is_and_insn(ival))
	decode_r_type_insn (AND, ival);
      else if (is_lr_w_insn (ival))
	decode_r_type_insn (LR_W, ival);
      else if (is_lr_d_insn (ival))
	decode_r_type_insn (LR_D, ival);
      else if (is_sc_w_insn (ival))
	decode_r_type_insn (SC_W, ival);
      else if (is_sc_d_insn (ival))
	decode_r_type_insn (SC_D, ival);
      else if (is_ecall_insn (ival))
	decode_i_type_insn (ECALL, ival);
      else if (is_ld_insn (ival))
	decode_i_type_insn (LD, ival);
      else if (is_lw_insn (ival))
	decode_i_type_insn (LW, ival);
      else
	/* None of the other fields are valid in this case.  */
	m_opcode = OTHER;
    }
  else if (m_length == 2)
    {
      int xlen = riscv_isa_xlen (gdbarch);

      /* C_ADD and C_JALR have the same opcode.  If RS2 is 0, then this is a
	 C_JALR.  So must try to match C_JALR first as it has more bits in
	 mask.  */
      if (is_c_jalr_insn (ival))
	decode_cr_type_insn (JALR, ival);
      else if (is_c_add_insn (ival))
	decode_cr_type_insn (ADD, ival);
      /* C_ADDW is RV64 and RV128 only.  */
      else if (xlen != 4 && is_c_addw_insn (ival))
	decode_cr_type_insn (ADDW, ival);
      else if (is_c_addi_insn (ival))
	decode_ci_type_insn (ADDI, ival);
      /* C_ADDIW and C_JAL have the same opcode.  C_ADDIW is RV64 and RV128
	 only and C_JAL is RV32 only.  */
      else if (xlen != 4 && is_c_addiw_insn (ival))
	decode_ci_type_insn (ADDIW, ival);
      else if (xlen == 4 && is_c_jal_insn (ival))
	decode_cj_type_insn (JAL, ival);
      /* C_ADDI16SP and C_LUI have the same opcode.  If RD is 2, then this is a
	 C_ADDI16SP.  So must try to match C_ADDI16SP first as it has more bits
	 in mask.  */
      else if (is_c_addi16sp_insn (ival))
	{
	  m_opcode = ADDI;
	  m_rd = m_rs1 = decode_register_index (ival, OP_SH_RD);
	  m_imm.s = EXTRACT_CITYPE_ADDI16SP_IMM (ival);
	}
      else if (is_c_addi4spn_insn (ival))
	{
	  m_opcode = ADDI;
	  m_rd = decode_register_index_short (ival, OP_SH_CRS2S);
	  m_rs1 = RISCV_SP_REGNUM;
	  m_imm.s = EXTRACT_CIWTYPE_ADDI4SPN_IMM (ival);
	}
      else if (is_c_lui_insn (ival))
	{
	  m_opcode = LUI;
	  m_rd = decode_register_index (ival, OP_SH_CRS1S);
	  m_imm.s = EXTRACT_CITYPE_LUI_IMM (ival);
	}
      else if (is_c_srli_insn (ival))
	decode_cb_type_insn (SRLI, ival);
      else if (is_c_srai_insn (ival))
	decode_cb_type_insn (SRAI, ival);
      else if (is_c_andi_insn (ival))
	decode_cb_type_insn (ANDI, ival);
      else if (is_c_sub_insn (ival))
	decode_ca_type_insn (SUB, ival);
      else if (is_c_xor_insn (ival))
	decode_ca_type_insn (XOR, ival);
      else if (is_c_or_insn (ival))
	decode_ca_type_insn (OR, ival);
      else if (is_c_and_insn (ival))
	decode_ca_type_insn (AND, ival);
      else if (is_c_subw_insn (ival))
	decode_ca_type_insn (SUBW, ival);
      else if (is_c_addw_insn (ival))
	decode_ca_type_insn (ADDW, ival);
      else if (is_c_li_insn (ival))
	decode_ci_type_insn (LI, ival);
      /* C_SD and C_FSW have the same opcode.  C_SD is RV64 and RV128 only,
	 and C_FSW is RV32 only.  */
      else if (xlen != 4 && is_c_sd_insn (ival))
	decode_cs_type_insn (SD, ival, EXTRACT_CLTYPE_LD_IMM (ival));
      else if (is_c_sw_insn (ival))
	decode_cs_type_insn (SW, ival, EXTRACT_CLTYPE_LW_IMM (ival));
      else if (is_c_swsp_insn (ival))
	decode_css_type_insn (SW, ival, EXTRACT_CSSTYPE_SWSP_IMM (ival));
      else if (xlen != 4 && is_c_sdsp_insn (ival))
	decode_css_type_insn (SD, ival, EXTRACT_CSSTYPE_SDSP_IMM (ival));
      /* C_JR and C_MV have the same opcode.  If RS2 is 0, then this is a C_JR.
	 So must try to match C_JR first as it has more bits in mask.  */
      else if (is_c_jr_insn (ival))
	decode_cr_type_insn (JALR, ival);
      else if (is_c_mv_insn (ival))
	decode_cr_type_insn (MV, ival);
      else if (is_c_j_insn (ival))
	decode_cj_type_insn (JAL, ival);
      else if (is_c_beqz_insn (ival))
	decode_cb_type_insn (BEQ, ival);
      else if (is_c_bnez_insn (ival))
	decode_cb_type_insn (BNE, ival);
      else if (is_c_ld_insn (ival))
	decode_cl_type_insn (LD, ival);
      else if (is_c_lw_insn (ival))
	decode_cl_type_insn (LW, ival);
      else if (is_c_ldsp_insn (ival))
	decode_ci_type_insn (LD, ival, RISCV_SP_REGNUM);
      else if (is_c_lwsp_insn (ival))
	decode_ci_type_insn (LW, ival, RISCV_SP_REGNUM);
      else
	/* None of the other fields of INSN are valid in this case.  */
	m_opcode = OTHER;
    }
  else
    {
      /* 6 bytes or more.  If the instruction is longer than 8 bytes, we don't
	 have full instruction bits in ival.  At least, such long instructions
	 are not defined yet, so just ignore it.  */
      gdb_assert (m_length > 0 && m_length % 2 == 0);
      m_opcode = OTHER;
    }
}

/* Return true if INSN represents an instruction something like:

   ld fp,IMMEDIATE(sp)

   That is, a load from stack-pointer plus some immediate offset, with the
   result stored into the frame pointer.  We also accept 'lw' as well as
   'ld'.  */

static bool
is_insn_load_of_fp_from_sp (const struct riscv_insn &insn)
{
  return ((insn.opcode () == riscv_insn::LD
	   || insn.opcode () == riscv_insn::LW)
	  && insn.rd () == RISCV_FP_REGNUM
	  && insn.rs1 () == RISCV_SP_REGNUM);
}

/* Return true if INSN represents an instruction something like:

   add sp,sp,IMMEDIATE

   That is, an add of an immediate to the value in the stack pointer
   register, with the result stored back to the stack pointer register.  */

static bool
is_insn_addi_of_sp_to_sp (const struct riscv_insn &insn)
{
  return ((insn.opcode () == riscv_insn::ADDI
	   || insn.opcode () == riscv_insn::ADDIW)
	  && insn.rd () == RISCV_SP_REGNUM
	  && insn.rs1 () == RISCV_SP_REGNUM);
}

/* Is the instruction in code memory prior to address PC a load from stack
   instruction?  Return true if it is, otherwise, return false.

   This is a best effort that is used as part of the function prologue
   scanning logic.  With compressed instructions and arbitrary control
   flow in the inferior, we can never be certain what the instruction
   prior to PC is.

   This function first looks for a compressed instruction, then looks for
   a 32-bit non-compressed instruction.  */

static bool
previous_insn_is_load_fp_from_stack (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  struct riscv_insn insn;
  insn.decode (gdbarch, pc - 2);
  gdb_assert (insn.length () > 0);

  if (insn.length () != 2 || !is_insn_load_of_fp_from_sp (insn))
    {
      insn.decode (gdbarch, pc - 4);
      gdb_assert (insn.length () > 0);

      if (insn.length () != 4 || !is_insn_load_of_fp_from_sp (insn))
	return false;
    }

  riscv_unwinder_debug_printf
    ("previous instruction at %s (length %d) was 'ld'",
     core_addr_to_string (pc - insn.length ()), insn.length ());
  return true;
}

/* Is the instruction in code memory prior to address PC an add of an
   immediate to the stack pointer, with the result being written back into
   the stack pointer?  Return true and set *PREV_PC to the address of the
   previous instruction if we believe the previous instruction is such an
   add, otherwise return false and *PREV_PC is undefined.

   This is a best effort that is used as part of the function prologue
   scanning logic.  With compressed instructions and arbitrary control
   flow in the inferior, we can never be certain what the instruction
   prior to PC is.

   This function first looks for a compressed instruction, then looks for
   a 32-bit non-compressed instruction.  */

static bool
previous_insn_is_add_imm_to_sp (struct gdbarch *gdbarch, CORE_ADDR pc,
				CORE_ADDR *prev_pc)
{
  struct riscv_insn insn;
  insn.decode (gdbarch, pc - 2);
  gdb_assert (insn.length () > 0);

  if (insn.length () != 2 || !is_insn_addi_of_sp_to_sp (insn))
    {
      insn.decode (gdbarch, pc - 4);
      gdb_assert (insn.length () > 0);

      if (insn.length () != 4 || !is_insn_addi_of_sp_to_sp (insn))
	return false;
    }

  riscv_unwinder_debug_printf
    ("previous instruction at %s (length %d) was 'add'",
     core_addr_to_string (pc - insn.length ()), insn.length ());
  *prev_pc = pc - insn.length ();
  return true;
}

/* Try to spot when PC is located in an exit sequence for a particular
   function.  Detecting an exit sequence involves a limited amount of
   scanning backwards through the disassembly, and so, when considering
   compressed instructions, we can never be certain that we have
   disassembled the preceding instructions correctly.  On top of that, we
   can't be certain that the inferior arrived at PC by passing through the
   preceding instructions.

   With all that said, we know that using prologue scanning to figure a
   functions unwind information starts to fail when we consider returns
   from an instruction -- we must pass through some instructions that
   restore the previous state prior to the final return instruction, and
   with state partially restored, our prologue derived unwind information
   is no longer valid.

   This function then, aims to spot instruction sequences like this:

     ld     fp, IMM_1(sp)
     add    sp, sp, IMM_2
     ret

   The first instruction restores the previous frame-pointer value, the
   second restores the previous stack pointer value, and the final
   instruction is the actual return.

   We need to consider that some or all of these instructions might be
   compressed.

   This function makes the assumption that, when the inferior reaches the
   'ret' instruction the stack pointer will have been restored to its value
   on entry to this function.  This assumption will be true in most well
   formed programs.

   Return true if we detect that we are in such an instruction sequence,
   that is PC points at one of the three instructions given above.  In this
   case, set *OFFSET to IMM_2 if PC points to either of the first
   two instructions (the 'ld' or 'add'), otherwise set *OFFSET to 0.

   Otherwise, this function returns false, and the contents of *OFFSET are
   undefined.  */

static bool
riscv_detect_end_of_function (struct gdbarch *gdbarch, CORE_ADDR pc,
			      int *offset)
{
  *offset = 0;

  /* We only want to scan a maximum of 3 instructions.  */
  for (int i = 0; i < 3; ++i)
    {
      struct riscv_insn insn;
      insn.decode (gdbarch, pc);
      gdb_assert (insn.length () > 0);

      if (is_insn_load_of_fp_from_sp (insn))
	{
	  riscv_unwinder_debug_printf ("found 'ld' instruction at %s",
				       core_addr_to_string (pc));
	  if (i > 0)
	    return false;
	  pc += insn.length ();
	}
      else if (is_insn_addi_of_sp_to_sp (insn))
	{
	  riscv_unwinder_debug_printf ("found 'add' instruction at %s",
				       core_addr_to_string (pc));
	  if (i > 1)
	    return false;
	  if (i == 0)
	    {
	      if (!previous_insn_is_load_fp_from_stack (gdbarch, pc))
		return false;

	      i = 1;
	    }
	  *offset = insn.imm_signed ();
	  pc += insn.length ();
	}
      else if (insn.opcode () == riscv_insn::JALR
	       && insn.rs1 () == RISCV_RA_REGNUM
	       && insn.rs2 () == RISCV_ZERO_REGNUM)
	{
	  riscv_unwinder_debug_printf ("found 'ret' instruction at %s",
				       core_addr_to_string (pc));
	  gdb_assert (i != 1);
	  if (i == 0)
	    {
	      CORE_ADDR prev_pc;
	      if (!previous_insn_is_add_imm_to_sp (gdbarch, pc, &prev_pc))
		return false;
	      if (!previous_insn_is_load_fp_from_stack (gdbarch, prev_pc))
		return false;
	      i = 2;
	    }

	  pc += insn.length ();
	}
      else
	return false;
    }

  return true;
}

/* The prologue scanner.  This is currently only used for skipping the
   prologue of a function when the DWARF information is not sufficient.
   However, it is written with filling of the frame cache in mind, which
   is why different groups of stack setup instructions are split apart
   during the core of the inner loop.  In the future, the intention is to
   extend this function to fully support building up a frame cache that
   can unwind register values when there is no DWARF information.  */

static CORE_ADDR
riscv_scan_prologue (struct gdbarch *gdbarch,
		     CORE_ADDR start_pc, CORE_ADDR end_pc,
		     struct riscv_unwind_cache *cache)
{
  CORE_ADDR cur_pc, next_pc, after_prologue_pc;
  CORE_ADDR original_end_pc = end_pc;
  CORE_ADDR end_prologue_addr = 0;

  /* Find an upper limit on the function prologue using the debug
     information.  If the debug information could not be used to provide
     that bound, then use an arbitrary large number as the upper bound.  */
  after_prologue_pc = skip_prologue_using_sal (gdbarch, start_pc);
  if (after_prologue_pc == 0)
    after_prologue_pc = start_pc + 100;   /* Arbitrary large number.  */
  if (after_prologue_pc < end_pc)
    end_pc = after_prologue_pc;

  pv_t regs[RISCV_NUM_INTEGER_REGS]; /* Number of GPR.  */
  for (int regno = 0; regno < RISCV_NUM_INTEGER_REGS; regno++)
    regs[regno] = pv_register (regno, 0);
  pv_area stack (RISCV_SP_REGNUM, gdbarch_addr_bit (gdbarch));

  riscv_unwinder_debug_printf ("function starting at %s (limit %s)",
			       core_addr_to_string (start_pc),
			       core_addr_to_string (end_pc));

  for (next_pc = cur_pc = start_pc; cur_pc < end_pc; cur_pc = next_pc)
    {
      struct riscv_insn insn;

      /* Decode the current instruction, and decide where the next
	 instruction lives based on the size of this instruction.  */
      insn.decode (gdbarch, cur_pc);
      gdb_assert (insn.length () > 0);
      next_pc = cur_pc + insn.length ();

      /* Look for common stack adjustment insns.  */
      if (is_insn_addi_of_sp_to_sp (insn))
	{
	  /* Handle: addi sp, sp, -i
	     or:     addiw sp, sp, -i  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()]
	    = pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ());
	}
      else if ((insn.opcode () == riscv_insn::SW
		|| insn.opcode () == riscv_insn::SD)
	       && (insn.rs1 () == RISCV_SP_REGNUM
		   || insn.rs1 () == RISCV_FP_REGNUM))
	{
	  /* Handle: sw reg, offset(sp)
	     or:     sd reg, offset(sp)
	     or:     sw reg, offset(s0)
	     or:     sd reg, offset(s0)  */
	  /* Instruction storing a register onto the stack.  */
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs2 () < RISCV_NUM_INTEGER_REGS);
	  stack.store (pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ()),
			(insn.opcode () == riscv_insn::SW ? 4 : 8),
			regs[insn.rs2 ()]);
	}
      else if (insn.opcode () == riscv_insn::ADDI
	       && insn.rd () == RISCV_FP_REGNUM
	       && insn.rs1 () == RISCV_SP_REGNUM)
	{
	  /* Handle: addi s0, sp, size  */
	  /* Instructions setting up the frame pointer.  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()]
	    = pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ());
	}
      else if ((insn.opcode () == riscv_insn::ADD
		|| insn.opcode () == riscv_insn::ADDW)
	       && insn.rd () == RISCV_FP_REGNUM
	       && insn.rs1 () == RISCV_SP_REGNUM
	       && insn.rs2 () == RISCV_ZERO_REGNUM)
	{
	  /* Handle: add s0, sp, 0
	     or:     addw s0, sp, 0  */
	  /* Instructions setting up the frame pointer.  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()] = pv_add_constant (regs[insn.rs1 ()], 0);
	}
      else if ((insn.opcode () == riscv_insn::ADDI
		&& insn.rd () == RISCV_ZERO_REGNUM
		&& insn.rs1 () == RISCV_ZERO_REGNUM
		&& insn.imm_signed () == 0))
	{
	  /* Handle: add x0, x0, 0   (NOP)  */
	}
      else if (insn.opcode () == riscv_insn::AUIPC)
	{
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()] = pv_constant (cur_pc + insn.imm_signed ());
	}
      else if (insn.opcode () == riscv_insn::LUI
	       || insn.opcode () == riscv_insn::LI)
	{
	  /* Handle: lui REG, n
	     or:     li  REG, n  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()] = pv_constant (insn.imm_signed ());
	}
      else if (insn.opcode () == riscv_insn::ADDI)
	{
	  /* Handle: addi REG1, REG2, IMM  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()]
	    = pv_add_constant (regs[insn.rs1 ()], insn.imm_signed ());
	}
      else if (insn.opcode () == riscv_insn::ADD)
	{
	  /* Handle: add REG1, REG2, REG3  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs2 () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()] = pv_add (regs[insn.rs1 ()], regs[insn.rs2 ()]);
	}
      else if (insn.opcode () == riscv_insn::LD
	       || insn.opcode () == riscv_insn::LW)
	{
	  /* Handle: ld reg, offset(rs1)
	     or:     c.ld reg, offset(rs1)
	     or:     lw reg, offset(rs1)
	     or:     c.lw reg, offset(rs1)  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs1 () < RISCV_NUM_INTEGER_REGS);
	  regs[insn.rd ()]
	    = stack.fetch (pv_add_constant (regs[insn.rs1 ()],
					    insn.imm_signed ()),
			   (insn.opcode () == riscv_insn::LW ? 4 : 8));
	}
      else if (insn.opcode () == riscv_insn::MV)
	{
	  /* Handle: c.mv RD, RS2  */
	  gdb_assert (insn.rd () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs2 () < RISCV_NUM_INTEGER_REGS);
	  gdb_assert (insn.rs2 () > 0);
	  regs[insn.rd ()] = regs[insn.rs2 ()];
	}
      else
	{
	  end_prologue_addr = cur_pc;
	  break;
	}
    }

  if (end_prologue_addr == 0)
    end_prologue_addr = cur_pc;

  riscv_unwinder_debug_printf ("end of prologue at %s",
			       core_addr_to_string (end_prologue_addr));

  if (cache != NULL)
    {
      /* Figure out if it is a frame pointer or just a stack pointer.  Also
	 the offset held in the pv_t is from the original register value to
	 the current value, which for a grows down stack means a negative
	 value.  The FRAME_BASE_OFFSET is the negation of this, how to get
	 from the current value to the original value.  */
      if (pv_is_register (regs[RISCV_FP_REGNUM], RISCV_SP_REGNUM))
	{
	  cache->frame_base_reg = RISCV_FP_REGNUM;
	  cache->frame_base_offset = -regs[RISCV_FP_REGNUM].k;
	}
      else
	{
	  cache->frame_base_reg = RISCV_SP_REGNUM;
	  cache->frame_base_offset = -regs[RISCV_SP_REGNUM].k;
	}

      /* Check to see if we are located near to a return instruction in
	 this function.  If we are then the one or both of the stack
	 pointer and frame pointer may have been restored to their previous
	 value.  If we can spot this situation then we can adjust which
	 register and offset we use for the frame base.  */
      if (cache->frame_base_reg != RISCV_SP_REGNUM
	  || cache->frame_base_offset != 0)
	{
	  int sp_offset;

	  if (riscv_detect_end_of_function (gdbarch, original_end_pc,
					    &sp_offset))
	    {
	      riscv_unwinder_debug_printf
		("in function epilogue at %s, stack offset is %d",
		 core_addr_to_string (original_end_pc), sp_offset);
	      cache->frame_base_reg= RISCV_SP_REGNUM;
	      cache->frame_base_offset = sp_offset;
	    }
	}

      /* Assign offset from old SP to all saved registers.  As we don't
	 have the previous value for the frame base register at this
	 point, we store the offset as the address in the trad_frame, and
	 then convert this to an actual address later.  */
      for (int i = 0; i <= RISCV_NUM_INTEGER_REGS; i++)
	{
	  CORE_ADDR offset;
	  if (stack.find_reg (gdbarch, i, &offset))
	    {
	      /* Display OFFSET as a signed value, the offsets are from the
		 frame base address to the registers location on the stack,
		 with a descending stack this means the offsets are always
		 negative.  */
	      riscv_unwinder_debug_printf ("register $%s at stack offset %s",
					   gdbarch_register_name (gdbarch, i),
					   plongest ((LONGEST) offset));
	      cache->regs[i].set_addr (offset);
	    }
	}
    }

  return end_prologue_addr;
}

/* Implement the riscv_skip_prologue gdbarch method.  */

static CORE_ADDR
riscv_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR func_addr;

  /* See if we can determine the end of the prologue via the symbol
     table.  If so, then return either PC, or the PC after the
     prologue, whichever is greater.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, NULL))
    {
      CORE_ADDR post_prologue_pc
	= skip_prologue_using_sal (gdbarch, func_addr);

      if (post_prologue_pc != 0)
	return std::max (pc, post_prologue_pc);
    }

  /* Can't determine prologue from the symbol table, need to examine
     instructions.  Pass -1 for the end address to indicate the prologue
     scanner can scan as far as it needs to find the end of the prologue.  */
  return riscv_scan_prologue (gdbarch, pc, ((CORE_ADDR) -1), NULL);
}

/* Implement the gdbarch push dummy code callback.  */

static CORE_ADDR
riscv_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp,
		       CORE_ADDR funaddr, struct value **args, int nargs,
		       struct type *value_type, CORE_ADDR *real_pc,
		       CORE_ADDR *bp_addr, struct regcache *regcache)
{
  /* A nop instruction is 'add x0, x0, 0'.  */
  static const gdb_byte nop_insn[] = { 0x13, 0x00, 0x00, 0x00 };

  /* Allocate space for a breakpoint, and keep the stack correctly
     aligned.  The space allocated here must be at least big enough to
     accommodate the NOP_INSN defined above.  */
  sp -= 16;
  *bp_addr = sp;
  *real_pc = funaddr;

  /* When we insert a breakpoint we select whether to use a compressed
     breakpoint or not based on the existing contents of the memory.

     If the breakpoint is being placed onto the stack as part of setting up
     for an inferior call from GDB, then the existing stack contents may
     randomly appear to be a compressed instruction, causing GDB to insert
     a compressed breakpoint.  If this happens on a target that does not
     support compressed instructions then this could cause problems.

     To prevent this issue we write an uncompressed nop onto the stack at
     the location where the breakpoint will be inserted.  In this way we
     ensure that we always use an uncompressed breakpoint, which should
     work on all targets.

     We call TARGET_WRITE_MEMORY here so that if the write fails we don't
     throw an exception.  Instead we ignore the error and move on.  The
     assumption is that either GDB will error later when actually trying to
     insert a software breakpoint, or GDB will use hardware breakpoints and
     there will be no need to write to memory later.  */
  int status = target_write_memory (*bp_addr, nop_insn, sizeof (nop_insn));

  riscv_infcall_debug_printf ("writing %s-byte nop instruction to %s: %s",
			      plongest (sizeof (nop_insn)),
			      paddress (gdbarch, *bp_addr),
			      (status == 0 ? "success" : "failed"));

  return sp;
}

/* Implement the gdbarch type alignment method, overrides the generic
   alignment algorithm for anything that is RISC-V specific.  */

static ULONGEST
riscv_type_align (gdbarch *gdbarch, type *type)
{
  type = check_typedef (type);
  if (type->code () == TYPE_CODE_ARRAY && type->is_vector ())
    return std::min (type->length (), (ULONGEST) BIGGEST_ALIGNMENT);

  /* Anything else will be aligned by the generic code.  */
  return 0;
}

/* Holds information about a single argument either being passed to an
   inferior function, or returned from an inferior function.  This includes
   information about the size, type, etc of the argument, and also
   information about how the argument will be passed (or returned).  */

struct riscv_arg_info
{
  /* Contents of the argument.  */
  const gdb_byte *contents;

  /* Length of argument.  */
  int length;

  /* Alignment required for an argument of this type.  */
  int align;

  /* The type for this argument.  */
  struct type *type;

  /* Each argument can have either 1 or 2 locations assigned to it.  Each
     location describes where part of the argument will be placed.  The
     second location is valid based on the LOC_TYPE and C_LENGTH fields
     of the first location (which is always valid).  */
  struct location
  {
    /* What type of location this is.  */
    enum location_type
      {
       /* Argument passed in a register.  */
       in_reg,

       /* Argument passed as an on stack argument.  */
       on_stack,

       /* Argument passed by reference.  The second location is always
	  valid for a BY_REF argument, and describes where the address
	  of the BY_REF argument should be placed.  */
       by_ref
      } loc_type;

    /* Information that depends on the location type.  */
    union
    {
      /* Which register number to use.  */
      int regno;

      /* The offset into the stack region.  */
      int offset;
    } loc_data;

    /* The length of contents covered by this location.  If this is less
       than the total length of the argument, then the second location
       will be valid, and will describe where the rest of the argument
       will go.  */
    int c_length;

    /* The offset within CONTENTS for this part of the argument.  This can
       be non-zero even for the first part (the first field of a struct can
       have a non-zero offset due to padding).  For the second part of the
       argument, this might be the C_LENGTH value of the first part,
       however, if we are passing a structure in two registers, and there's
       is padding between the first and second field, then this offset
       might be greater than the length of the first argument part.  When
       the second argument location is not holding part of the argument
       value, but is instead holding the address of a reference argument,
       then this offset will be set to 0.  */
    int c_offset;
  } argloc[2];

  /* TRUE if this is an unnamed argument.  */
  bool is_unnamed;
};

/* Information about a set of registers being used for passing arguments as
   part of a function call.  The register set must be numerically
   sequential from NEXT_REGNUM to LAST_REGNUM.  The register set can be
   disabled from use by setting NEXT_REGNUM greater than LAST_REGNUM.  */

struct riscv_arg_reg
{
  riscv_arg_reg (int first, int last)
    : next_regnum (first),
      last_regnum (last)
  {
    /* Nothing.  */
  }

  /* The GDB register number to use in this set.  */
  int next_regnum;

  /* The last GDB register number to use in this set.  */
  int last_regnum;
};

/* Arguments can be passed as on stack arguments, or by reference.  The
   on stack arguments must be in a continuous region starting from $sp,
   while the by reference arguments can be anywhere, but we'll put them
   on the stack after (at higher address) the on stack arguments.

   This might not be the right approach to take.  The ABI is clear that
   an argument passed by reference can be modified by the callee, which
   us placing the argument (temporarily) onto the stack will not achieve
   (changes will be lost).  There's also the possibility that very large
   arguments could overflow the stack.

   This struct is used to track offset into these two areas for where
   arguments are to be placed.  */
struct riscv_memory_offsets
{
  riscv_memory_offsets ()
    : arg_offset (0),
      ref_offset (0)
  {
    /* Nothing.  */
  }

  /* Offset into on stack argument area.  */
  int arg_offset;

  /* Offset into the pass by reference area.  */
  int ref_offset;
};

/* Holds information about where arguments to a call will be placed.  This
   is updated as arguments are added onto the call, and can be used to
   figure out where the next argument should be placed.  */

struct riscv_call_info
{
  riscv_call_info (struct gdbarch *gdbarch)
    : int_regs (RISCV_A0_REGNUM, RISCV_A0_REGNUM + 7),
      float_regs (RISCV_FA0_REGNUM, RISCV_FA0_REGNUM + 7)
  {
    xlen = riscv_abi_xlen (gdbarch);
    flen = riscv_abi_flen (gdbarch);

    /* Reduce the number of integer argument registers when using the
       embedded abi (i.e. rv32e).  */
    if (riscv_abi_embedded (gdbarch))
      int_regs.last_regnum = RISCV_A0_REGNUM + 5;

    /* Disable use of floating point registers if needed.  */
    if (!riscv_has_fp_abi (gdbarch))
      float_regs.next_regnum = float_regs.last_regnum + 1;
  }

  /* Track the memory areas used for holding in-memory arguments to a
     call.  */
  struct riscv_memory_offsets memory;

  /* Holds information about the next integer register to use for passing
     an argument.  */
  struct riscv_arg_reg int_regs;

  /* Holds information about the next floating point register to use for
     passing an argument.  */
  struct riscv_arg_reg float_regs;

  /* The XLEN and FLEN are copied in to this structure for convenience, and
     are just the results of calling RISCV_ABI_XLEN and RISCV_ABI_FLEN.  */
  int xlen;
  int flen;
};

/* Return the number of registers available for use as parameters in the
   register set REG.  Returned value can be 0 or more.  */

static int
riscv_arg_regs_available (struct riscv_arg_reg *reg)
{
  if (reg->next_regnum > reg->last_regnum)
    return 0;

  return (reg->last_regnum - reg->next_regnum + 1);
}

/* If there is at least one register available in the register set REG then
   the next register from REG is assigned to LOC and the length field of
   LOC is updated to LENGTH.  The register set REG is updated to indicate
   that the assigned register is no longer available and the function
   returns true.

   If there are no registers available in REG then the function returns
   false, and LOC and REG are unchanged.  */

static bool
riscv_assign_reg_location (struct riscv_arg_info::location *loc,
			   struct riscv_arg_reg *reg,
			   int length, int offset)
{
  if (reg->next_regnum <= reg->last_regnum)
    {
      loc->loc_type = riscv_arg_info::location::in_reg;
      loc->loc_data.regno = reg->next_regnum;
      reg->next_regnum++;
      loc->c_length = length;
      loc->c_offset = offset;
      return true;
    }

  return false;
}

/* Assign LOC a location as the next stack parameter, and update MEMORY to
   record that an area of stack has been used to hold the parameter
   described by LOC.

   The length field of LOC is updated to LENGTH, the length of the
   parameter being stored, and ALIGN is the alignment required by the
   parameter, which will affect how memory is allocated out of MEMORY.  */

static void
riscv_assign_stack_location (struct riscv_arg_info::location *loc,
			     struct riscv_memory_offsets *memory,
			     int length, int align)
{
  loc->loc_type = riscv_arg_info::location::on_stack;
  memory->arg_offset
    = align_up (memory->arg_offset, align);
  loc->loc_data.offset = memory->arg_offset;
  memory->arg_offset += length;
  loc->c_length = length;

  /* Offset is always 0, either we're the first location part, in which
     case we're reading content from the start of the argument, or we're
     passing the address of a reference argument, so 0.  */
  loc->c_offset = 0;
}

/* Update AINFO, which describes an argument that should be passed or
   returned using the integer ABI.  The argloc fields within AINFO are
   updated to describe the location in which the argument will be passed to
   a function, or returned from a function.

   The CINFO structure contains the ongoing call information, the holds
   information such as which argument registers are remaining to be
   assigned to parameter, and how much memory has been used by parameters
   so far.

   By examining the state of CINFO a suitable location can be selected,
   and assigned to AINFO.  */

static void
riscv_call_arg_scalar_int (struct riscv_arg_info *ainfo,
			   struct riscv_call_info *cinfo)
{
  if (TYPE_HAS_DYNAMIC_LENGTH (ainfo->type)
      || ainfo->length > (2 * cinfo->xlen))
    {
      /* Argument is going to be passed by reference.  */
      ainfo->argloc[0].loc_type
	= riscv_arg_info::location::by_ref;
      cinfo->memory.ref_offset
	= align_up (cinfo->memory.ref_offset, ainfo->align);
      ainfo->argloc[0].loc_data.offset = cinfo->memory.ref_offset;
      cinfo->memory.ref_offset += ainfo->length;
      ainfo->argloc[0].c_length = ainfo->length;

      /* The second location for this argument is given over to holding the
	 address of the by-reference data.  Pass 0 for the offset as this
	 is not part of the actual argument value.  */
      if (!riscv_assign_reg_location (&ainfo->argloc[1],
				      &cinfo->int_regs,
				      cinfo->xlen, 0))
	riscv_assign_stack_location (&ainfo->argloc[1],
				     &cinfo->memory, cinfo->xlen,
				     cinfo->xlen);
    }
  else
    {
      int len = std::min (ainfo->length, cinfo->xlen);
      int align = std::max (ainfo->align, cinfo->xlen);

      /* Unnamed arguments in registers that require 2*XLEN alignment are
	 passed in an aligned register pair.  */
      if (ainfo->is_unnamed && (align == cinfo->xlen * 2)
	  && cinfo->int_regs.next_regnum & 1)
	cinfo->int_regs.next_regnum++;

      if (!riscv_assign_reg_location (&ainfo->argloc[0],
				      &cinfo->int_regs, len, 0))
	riscv_assign_stack_location (&ainfo->argloc[0],
				     &cinfo->memory, len, align);

      if (len < ainfo->length)
	{
	  len = ainfo->length - len;
	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->int_regs, len,
					  cinfo->xlen))
	    riscv_assign_stack_location (&ainfo->argloc[1],
					 &cinfo->memory, len, cinfo->xlen);
	}
    }
}

/* Like RISCV_CALL_ARG_SCALAR_INT, except the argument described by AINFO
   is being passed with the floating point ABI.  */

static void
riscv_call_arg_scalar_float (struct riscv_arg_info *ainfo,
			     struct riscv_call_info *cinfo)
{
  if (ainfo->length > cinfo->flen || ainfo->is_unnamed)
    return riscv_call_arg_scalar_int (ainfo, cinfo);
  else
    {
      if (!riscv_assign_reg_location (&ainfo->argloc[0],
				      &cinfo->float_regs,
				      ainfo->length, 0))
	return riscv_call_arg_scalar_int (ainfo, cinfo);
    }
}

/* Like RISCV_CALL_ARG_SCALAR_INT, except the argument described by AINFO
   is a complex floating point argument, and is therefore handled
   differently to other argument types.  */

static void
riscv_call_arg_complex_float (struct riscv_arg_info *ainfo,
			      struct riscv_call_info *cinfo)
{
  if (ainfo->length <= (2 * cinfo->flen)
      && riscv_arg_regs_available (&cinfo->float_regs) >= 2
      && !ainfo->is_unnamed)
    {
      bool result;
      int len = ainfo->length / 2;

      result = riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->float_regs, len, 0);
      gdb_assert (result);

      result = riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->float_regs, len, len);
      gdb_assert (result);
    }
  else
    return riscv_call_arg_scalar_int (ainfo, cinfo);
}

/* A structure used for holding information about a structure type within
   the inferior program.  The RiscV ABI has special rules for handling some
   structures with a single field or with two fields.  The counting of
   fields here is done after flattening out all nested structures.  */

class riscv_struct_info
{
public:
  riscv_struct_info ()
    : m_number_of_fields (0),
      m_types { nullptr, nullptr },
      m_offsets { 0, 0 }
  {
    /* Nothing.  */
  }

  /* Analyse TYPE descending into nested structures, count the number of
     scalar fields and record the types of the first two fields found.  */
  void analyse (struct type *type)
  {
    analyse_inner (type, 0);
  }

  /* The number of scalar fields found in the analysed type.  This is
     currently only accurate if the value returned is 0, 1, or 2 as the
     analysis stops counting when the number of fields is 3.  This is
     because the RiscV ABI only has special cases for 1 or 2 fields,
     anything else we just don't care about.  */
  int number_of_fields () const
  { return m_number_of_fields; }

  /* Return the type for scalar field INDEX within the analysed type.  Will
     return nullptr if there is no field at that index.  Only INDEX values
     0 and 1 can be requested as the RiscV ABI only has special cases for
     structures with 1 or 2 fields.  */
  struct type *field_type (int index) const
  {
    gdb_assert (index < (sizeof (m_types) / sizeof (m_types[0])));
    return m_types[index];
  }

  /* Return the offset of scalar field INDEX within the analysed type. Will
     return 0 if there is no field at that index.  Only INDEX values 0 and
     1 can be requested as the RiscV ABI only has special cases for
     structures with 1 or 2 fields.  */
  int field_offset (int index) const
  {
    gdb_assert (index < (sizeof (m_offsets) / sizeof (m_offsets[0])));
    return m_offsets[index];
  }

private:
  /* The number of scalar fields found within the structure after recursing
     into nested structures.  */
  int m_number_of_fields;

  /* The types of the first two scalar fields found within the structure
     after recursing into nested structures.  */
  struct type *m_types[2];

  /* The offsets of the first two scalar fields found within the structure
     after recursing into nested structures.  */
  int m_offsets[2];

  /* Recursive core for ANALYSE, the OFFSET parameter tracks the byte
     offset from the start of the top level structure being analysed.  */
  void analyse_inner (struct type *type, int offset);
};

/* See description in class declaration.  */

void
riscv_struct_info::analyse_inner (struct type *type, int offset)
{
  unsigned int count = type->num_fields ();
  unsigned int i;

  for (i = 0; i < count; ++i)
    {
      if (type->field (i).loc_kind () != FIELD_LOC_KIND_BITPOS)
	continue;

      struct type *field_type = type->field (i).type ();
      field_type = check_typedef (field_type);
      int field_offset
	= offset + type->field (i).loc_bitpos () / TARGET_CHAR_BIT;

      switch (field_type->code ())
	{
	case TYPE_CODE_STRUCT:
	  analyse_inner (field_type, field_offset);
	  break;

	default:
	  /* RiscV only flattens out structures.  Anything else does not
	     need to be flattened, we just record the type, and when we
	     look at the analysis results we'll realise this is not a
	     structure we can special case, and pass the structure in
	     memory.  */
	  if (m_number_of_fields < 2)
	    {
	      m_types[m_number_of_fields] = field_type;
	      m_offsets[m_number_of_fields] = field_offset;
	    }
	  m_number_of_fields++;
	  break;
	}

      /* RiscV only has special handling for structures with 1 or 2 scalar
	 fields, any more than that and the structure is just passed in
	 memory.  We can safely drop out early when we find 3 or more
	 fields then.  */

      if (m_number_of_fields > 2)
	return;
    }
}

/* Like RISCV_CALL_ARG_SCALAR_INT, except the argument described by AINFO
   is a structure.  Small structures on RiscV have some special case
   handling in order that the structure might be passed in register.
   Larger structures are passed in memory.  After assigning location
   information to AINFO, CINFO will have been updated.  */

static void
riscv_call_arg_struct (struct riscv_arg_info *ainfo,
		       struct riscv_call_info *cinfo)
{
  if (riscv_arg_regs_available (&cinfo->float_regs) >= 1)
    {
      struct riscv_struct_info sinfo;

      sinfo.analyse (ainfo->type);
      if (sinfo.number_of_fields () == 1
	  && sinfo.field_type(0)->code () == TYPE_CODE_COMPLEX)
	{
	  /* The following is similar to RISCV_CALL_ARG_COMPLEX_FLOAT,
	     except we use the type of the complex field instead of the
	     type from AINFO, and the first location might be at a non-zero
	     offset.  */
	  if (sinfo.field_type (0)->length () <= (2 * cinfo->flen)
	      && riscv_arg_regs_available (&cinfo->float_regs) >= 2
	      && !ainfo->is_unnamed)
	    {
	      bool result;
	      int len = sinfo.field_type (0)->length () / 2;
	      int offset = sinfo.field_offset (0);

	      result = riscv_assign_reg_location (&ainfo->argloc[0],
						  &cinfo->float_regs, len,
						  offset);
	      gdb_assert (result);

	      result = riscv_assign_reg_location (&ainfo->argloc[1],
						  &cinfo->float_regs, len,
						  (offset + len));
	      gdb_assert (result);
	    }
	  else
	    riscv_call_arg_scalar_int (ainfo, cinfo);
	  return;
	}

      if (sinfo.number_of_fields () == 1
	  && sinfo.field_type(0)->code () == TYPE_CODE_FLT)
	{
	  /* The following is similar to RISCV_CALL_ARG_SCALAR_FLOAT,
	     except we use the type of the first scalar field instead of
	     the type from AINFO.  Also the location might be at a non-zero
	     offset.  */
	  if (sinfo.field_type (0)->length () > cinfo->flen
	      || ainfo->is_unnamed)
	    riscv_call_arg_scalar_int (ainfo, cinfo);
	  else
	    {
	      int offset = sinfo.field_offset (0);
	      int len = sinfo.field_type (0)->length ();

	      if (!riscv_assign_reg_location (&ainfo->argloc[0],
					      &cinfo->float_regs,
					      len, offset))
		riscv_call_arg_scalar_int (ainfo, cinfo);
	    }
	  return;
	}

      if (sinfo.number_of_fields () == 2
	  && sinfo.field_type(0)->code () == TYPE_CODE_FLT
	  && sinfo.field_type (0)->length () <= cinfo->flen
	  && sinfo.field_type(1)->code () == TYPE_CODE_FLT
	  && sinfo.field_type (1)->length () <= cinfo->flen
	  && riscv_arg_regs_available (&cinfo->float_regs) >= 2)
	{
	  int len0 = sinfo.field_type (0)->length ();
	  int offset = sinfo.field_offset (0);
	  if (!riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->float_regs, len0, offset))
	    error (_("failed during argument setup"));

	  int len1 = sinfo.field_type (1)->length ();
	  offset = sinfo.field_offset (1);
	  gdb_assert (len1 <= (ainfo->type->length ()
			       - sinfo.field_type (0)->length ()));

	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->float_regs,
					  len1, offset))
	    error (_("failed during argument setup"));
	  return;
	}

      if (sinfo.number_of_fields () == 2
	  && riscv_arg_regs_available (&cinfo->int_regs) >= 1
	  && (sinfo.field_type(0)->code () == TYPE_CODE_FLT
	      && sinfo.field_type (0)->length () <= cinfo->flen
	      && is_integral_type (sinfo.field_type (1))
	      && sinfo.field_type (1)->length () <= cinfo->xlen))
	{
	  int  len0 = sinfo.field_type (0)->length ();
	  int offset = sinfo.field_offset (0);
	  if (!riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->float_regs, len0, offset))
	    error (_("failed during argument setup"));

	  int len1 = sinfo.field_type (1)->length ();
	  offset = sinfo.field_offset (1);
	  gdb_assert (len1 <= cinfo->xlen);
	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->int_regs, len1, offset))
	    error (_("failed during argument setup"));
	  return;
	}

      if (sinfo.number_of_fields () == 2
	  && riscv_arg_regs_available (&cinfo->int_regs) >= 1
	  && (is_integral_type (sinfo.field_type (0))
	      && sinfo.field_type (0)->length () <= cinfo->xlen
	      && sinfo.field_type(1)->code () == TYPE_CODE_FLT
	      && sinfo.field_type (1)->length () <= cinfo->flen))
	{
	  int len0 = sinfo.field_type (0)->length ();
	  int len1 = sinfo.field_type (1)->length ();

	  gdb_assert (len0 <= cinfo->xlen);
	  gdb_assert (len1 <= cinfo->flen);

	  int offset = sinfo.field_offset (0);
	  if (!riscv_assign_reg_location (&ainfo->argloc[0],
					  &cinfo->int_regs, len0, offset))
	    error (_("failed during argument setup"));

	  offset = sinfo.field_offset (1);
	  if (!riscv_assign_reg_location (&ainfo->argloc[1],
					  &cinfo->float_regs,
					  len1, offset))
	    error (_("failed during argument setup"));

	  return;
	}
    }

  /* Non of the structure flattening cases apply, so we just pass using
     the integer ABI.  */
  riscv_call_arg_scalar_int (ainfo, cinfo);
}

/* Assign a location to call (or return) argument AINFO, the location is
   selected from CINFO which holds information about what call argument
   locations are available for use next.  The TYPE is the type of the
   argument being passed, this information is recorded into AINFO (along
   with some additional information derived from the type).  IS_UNNAMED
   is true if this is an unnamed (stdarg) argument, this info is also
   recorded into AINFO.

   After assigning a location to AINFO, CINFO will have been updated.  */

static void
riscv_arg_location (struct gdbarch *gdbarch,
		    struct riscv_arg_info *ainfo,
		    struct riscv_call_info *cinfo,
		    struct type *type, bool is_unnamed)
{
  ainfo->type = type;
  ainfo->length = ainfo->type->length ();
  ainfo->align = type_align (ainfo->type);
  ainfo->is_unnamed = is_unnamed;
  ainfo->contents = nullptr;
  ainfo->argloc[0].c_length = 0;
  ainfo->argloc[1].c_length = 0;

  switch (ainfo->type->code ())
    {
    case TYPE_CODE_INT:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_PTR:
    case TYPE_CODE_FIXED_POINT:
      if (ainfo->length <= cinfo->xlen)
	{
	  ainfo->type = builtin_type (gdbarch)->builtin_long;
	  ainfo->length = cinfo->xlen;
	}
      else if (ainfo->length <= (2 * cinfo->xlen))
	{
	  ainfo->type = builtin_type (gdbarch)->builtin_long_long;
	  ainfo->length = 2 * cinfo->xlen;
	}

      /* Recalculate the alignment requirement.  */
      ainfo->align = type_align (ainfo->type);
      riscv_call_arg_scalar_int (ainfo, cinfo);
      break;

    case TYPE_CODE_FLT:
      riscv_call_arg_scalar_float (ainfo, cinfo);
      break;

    case TYPE_CODE_COMPLEX:
      riscv_call_arg_complex_float (ainfo, cinfo);
      break;

    case TYPE_CODE_STRUCT:
      if (!TYPE_HAS_DYNAMIC_LENGTH (ainfo->type))
	{
	  riscv_call_arg_struct (ainfo, cinfo);
	  break;
	}
      [[fallthrough]];

    default:
      riscv_call_arg_scalar_int (ainfo, cinfo);
      break;
    }
}

/* Used for printing debug information about the call argument location in
   INFO to STREAM.  The addresses in SP_REFS and SP_ARGS are the base
   addresses for the location of pass-by-reference and
   arguments-on-the-stack memory areas.  */

static void
riscv_print_arg_location (ui_file *stream, struct gdbarch *gdbarch,
			  struct riscv_arg_info *info,
			  CORE_ADDR sp_refs, CORE_ADDR sp_args)
{
  gdb_printf (stream, "type: '%s', length: 0x%x, alignment: 0x%x",
	      TYPE_SAFE_NAME (info->type), info->length, info->align);
  switch (info->argloc[0].loc_type)
    {
    case riscv_arg_info::location::in_reg:
      gdb_printf
	(stream, ", register %s",
	 gdbarch_register_name (gdbarch, info->argloc[0].loc_data.regno));
      if (info->argloc[0].c_length < info->length)
	{
	  switch (info->argloc[1].loc_type)
	    {
	    case riscv_arg_info::location::in_reg:
	      gdb_printf
		(stream, ", register %s",
		 gdbarch_register_name (gdbarch,
					info->argloc[1].loc_data.regno));
	      break;

	    case riscv_arg_info::location::on_stack:
	      gdb_printf (stream, ", on stack at offset 0x%x",
			  info->argloc[1].loc_data.offset);
	      break;

	    case riscv_arg_info::location::by_ref:
	    default:
	      /* The second location should never be a reference, any
		 argument being passed by reference just places its address
		 in the first location and is done.  */
	      error (_("invalid argument location"));
	      break;
	    }

	  if (info->argloc[1].c_offset > info->argloc[0].c_length)
	    gdb_printf (stream, " (offset 0x%x)",
			info->argloc[1].c_offset);
	}
      break;

    case riscv_arg_info::location::on_stack:
      gdb_printf (stream, ", on stack at offset 0x%x",
		  info->argloc[0].loc_data.offset);
      break;

    case riscv_arg_info::location::by_ref:
      gdb_printf
	(stream, ", by reference, data at offset 0x%x (%s)",
	 info->argloc[0].loc_data.offset,
	 core_addr_to_string (sp_refs + info->argloc[0].loc_data.offset));
      if (info->argloc[1].loc_type
	  == riscv_arg_info::location::in_reg)
	gdb_printf
	  (stream, ", address in register %s",
	   gdbarch_register_name (gdbarch, info->argloc[1].loc_data.regno));
      else
	{
	  gdb_assert (info->argloc[1].loc_type
		      == riscv_arg_info::location::on_stack);
	  gdb_printf
	    (stream, ", address on stack at offset 0x%x (%s)",
	     info->argloc[1].loc_data.offset,
	     core_addr_to_string (sp_args + info->argloc[1].loc_data.offset));
	}
      break;

    default:
      gdb_assert_not_reached ("unknown argument location type");
    }
}

/* Wrapper around REGCACHE->cooked_write.  Places the LEN bytes of DATA
   into a buffer that is at least as big as the register REGNUM, padding
   out the DATA with either 0x00, or 0xff.  For floating point registers
   0xff is used, for everyone else 0x00 is used.  */

static void
riscv_regcache_cooked_write (int regnum, const gdb_byte *data, int len,
			     struct regcache *regcache, int flen)
{
  gdb_byte tmp [sizeof (ULONGEST)];

  /* FP values in FP registers must be NaN-boxed.  */
  if (riscv_is_fp_regno_p (regnum) && len < flen)
    memset (tmp, -1, sizeof (tmp));
  else
    memset (tmp, 0, sizeof (tmp));
  memcpy (tmp, data, len);
  regcache->cooked_write (regnum, tmp);
}

/* Implement the push dummy call gdbarch callback.  */

static CORE_ADDR
riscv_push_dummy_call (struct gdbarch *gdbarch,
		       struct value *function,
		       struct regcache *regcache,
		       CORE_ADDR bp_addr,
		       int nargs,
		       struct value **args,
		       CORE_ADDR sp,
		       function_call_return_method return_method,
		       CORE_ADDR struct_addr)
{
  int i;
  CORE_ADDR sp_args, sp_refs;
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);

  struct riscv_arg_info *arg_info =
    (struct riscv_arg_info *) alloca (nargs * sizeof (struct riscv_arg_info));

  struct riscv_call_info call_info (gdbarch);

  CORE_ADDR osp = sp;

  struct type *ftype = check_typedef (function->type ());

  if (ftype->code () == TYPE_CODE_PTR)
    ftype = check_typedef (ftype->target_type ());

  /* We'll use register $a0 if we're returning a struct.  */
  if (return_method == return_method_struct)
    ++call_info.int_regs.next_regnum;

  for (i = 0; i < nargs; ++i)
    {
      struct value *arg_value;
      struct type *arg_type;
      struct riscv_arg_info *info = &arg_info[i];

      arg_value = args[i];
      arg_type = check_typedef (arg_value->type ());

      riscv_arg_location (gdbarch, info, &call_info, arg_type,
			  ftype->has_varargs () && i >= ftype->num_fields ());

      if (info->type != arg_type)
	arg_value = value_cast (info->type, arg_value);
      info->contents = arg_value->contents ().data ();
    }

  /* Adjust the stack pointer and align it.  */
  sp = sp_refs = align_down (sp - call_info.memory.ref_offset, SP_ALIGNMENT);
  sp = sp_args = align_down (sp - call_info.memory.arg_offset, SP_ALIGNMENT);

  if (riscv_debug_infcall)
    {
      RISCV_INFCALL_SCOPED_DEBUG_START_END ("dummy call args");
      riscv_infcall_debug_printf ("floating point ABI %s in use",
				  (riscv_has_fp_abi (gdbarch)
				   ? "is" : "is not"));
      riscv_infcall_debug_printf ("xlen: %d", call_info.xlen);
      riscv_infcall_debug_printf ("flen: %d", call_info.flen);
      if (return_method == return_method_struct)
	riscv_infcall_debug_printf
	  ("[**] struct return pointer in register $A0");
      for (i = 0; i < nargs; ++i)
	{
	  struct riscv_arg_info *info = &arg_info [i];
	  string_file tmp;

	  riscv_print_arg_location (&tmp, gdbarch, info, sp_refs, sp_args);
	  riscv_infcall_debug_printf ("[%2d] %s", i, tmp.string ().c_str ());
	}
      if (call_info.memory.arg_offset > 0
	  || call_info.memory.ref_offset > 0)
	{
	  riscv_infcall_debug_printf ("              Original sp: %s",
				      core_addr_to_string (osp));
	  riscv_infcall_debug_printf ("Stack required (for args): 0x%x",
				      call_info.memory.arg_offset);
	  riscv_infcall_debug_printf ("Stack required (for refs): 0x%x",
				      call_info.memory.ref_offset);
	  riscv_infcall_debug_printf ("          Stack allocated: %s",
				      core_addr_to_string_nz (osp - sp));
	}
    }

  /* Now load the argument into registers, or onto the stack.  */

  if (return_method == return_method_struct)
    {
      gdb_byte buf[sizeof (LONGEST)];

      store_unsigned_integer (buf, call_info.xlen, byte_order, struct_addr);
      regcache->cooked_write (RISCV_A0_REGNUM, buf);
    }

  for (i = 0; i < nargs; ++i)
    {
      CORE_ADDR dst;
      int second_arg_length = 0;
      const gdb_byte *second_arg_data;
      struct riscv_arg_info *info = &arg_info [i];

      gdb_assert (info->length > 0);

      switch (info->argloc[0].loc_type)
	{
	case riscv_arg_info::location::in_reg:
	  {
	    gdb_assert (info->argloc[0].c_length <= info->length);

	    riscv_regcache_cooked_write (info->argloc[0].loc_data.regno,
					 (info->contents
					  + info->argloc[0].c_offset),
					 info->argloc[0].c_length,
					 regcache, call_info.flen);
	    second_arg_length =
	      (((info->argloc[0].c_length + info->argloc[0].c_offset) < info->length)
	       ? info->argloc[1].c_length : 0);
	    second_arg_data = info->contents + info->argloc[1].c_offset;
	  }
	  break;

	case riscv_arg_info::location::on_stack:
	  dst = sp_args + info->argloc[0].loc_data.offset;
	  write_memory (dst, info->contents, info->length);
	  second_arg_length = 0;
	  break;

	case riscv_arg_info::location::by_ref:
	  dst = sp_refs + info->argloc[0].loc_data.offset;
	  write_memory (dst, info->contents, info->length);

	  second_arg_length = call_info.xlen;
	  second_arg_data = (gdb_byte *) &dst;
	  break;

	default:
	  gdb_assert_not_reached ("unknown argument location type");
	}

      if (second_arg_length > 0)
	{
	  switch (info->argloc[1].loc_type)
	    {
	    case riscv_arg_info::location::in_reg:
	      {
		gdb_assert ((riscv_is_fp_regno_p (info->argloc[1].loc_data.regno)
			     && second_arg_length <= call_info.flen)
			    || second_arg_length <= call_info.xlen);
		riscv_regcache_cooked_write (info->argloc[1].loc_data.regno,
					     second_arg_data,
					     second_arg_length,
					     regcache, call_info.flen);
	      }
	      break;

	    case riscv_arg_info::location::on_stack:
	      {
		CORE_ADDR arg_addr;

		arg_addr = sp_args + info->argloc[1].loc_data.offset;
		write_memory (arg_addr, second_arg_data, second_arg_length);
		break;
	      }

	    case riscv_arg_info::location::by_ref:
	    default:
	      /* The second location should never be a reference, any
		 argument being passed by reference just places its address
		 in the first location and is done.  */
	      error (_("invalid argument location"));
	      break;
	    }
	}
    }

  /* Set the dummy return value to bp_addr.
     A dummy breakpoint will be setup to execute the call.  */

  riscv_infcall_debug_printf ("writing $ra = %s",
			      core_addr_to_string (bp_addr));
  regcache_cooked_write_unsigned (regcache, RISCV_RA_REGNUM, bp_addr);

  /* Finally, update the stack pointer.  */

  riscv_infcall_debug_printf ("writing $sp = %s", core_addr_to_string (sp));
  regcache_cooked_write_unsigned (regcache, RISCV_SP_REGNUM, sp);

  return sp;
}

/* Implement the return_value gdbarch method.  */

static enum return_value_convention
riscv_return_value (struct gdbarch  *gdbarch,
		    struct value *function,
		    struct type *type,
		    struct regcache *regcache,
		    struct value **read_value,
		    const gdb_byte *writebuf)
{
  struct riscv_call_info call_info (gdbarch);
  struct riscv_arg_info info;
  struct type *arg_type;

  arg_type = check_typedef (type);
  riscv_arg_location (gdbarch, &info, &call_info, arg_type, false);

  if (riscv_debug_infcall)
    {
      string_file tmp;
      riscv_print_arg_location (&tmp, gdbarch, &info, 0, 0);
      riscv_infcall_debug_printf ("[R] %s", tmp.string ().c_str ());
    }

  if (read_value != nullptr || writebuf != nullptr)
    {
      unsigned int arg_len;
      struct value *abi_val;
      gdb_byte *readbuf = nullptr;
      int regnum;

      /* We only do one thing at a time.  */
      gdb_assert (read_value == nullptr || writebuf == nullptr);

      /* In some cases the argument is not returned as the declared type,
	 and we need to cast to or from the ABI type in order to
	 correctly access the argument.  When writing to the machine we
	 do the cast here, when reading from the machine the cast occurs
	 later, after extracting the value.  As the ABI type can be
	 larger than the declared type, then the read or write buffers
	 passed in might be too small.  Here we ensure that we are using
	 buffers of sufficient size.  */
      if (writebuf != nullptr)
	{
	  struct value *arg_val;

	  if (is_fixed_point_type (arg_type))
	    {
	      /* Convert the argument to the type used to pass
		 the return value, but being careful to preserve
		 the fact that the value needs to be returned
		 unscaled.  */
	      gdb_mpz unscaled;

	      unscaled.read (gdb::make_array_view (writebuf,
						   arg_type->length ()),
			     type_byte_order (arg_type),
			     arg_type->is_unsigned ());
	      abi_val = value::allocate (info.type);
	      unscaled.write (abi_val->contents_raw (),
			      type_byte_order (info.type),
			      info.type->is_unsigned ());
	    }
	  else
	    {
	      arg_val = value_from_contents (arg_type, writebuf);
	      abi_val = value_cast (info.type, arg_val);
	    }
	  writebuf = abi_val->contents_raw ().data ();
	}
      else
	{
	  abi_val = value::allocate (info.type);
	  readbuf = abi_val->contents_raw ().data ();
	}
      arg_len = info.type->length ();

      switch (info.argloc[0].loc_type)
	{
	  /* Return value in register(s).  */
	case riscv_arg_info::location::in_reg:
	  {
	    regnum = info.argloc[0].loc_data.regno;
	    gdb_assert (info.argloc[0].c_length <= arg_len);
	    gdb_assert (info.argloc[0].c_length
			<= register_size (gdbarch, regnum));

	    if (readbuf)
	      {
		gdb_byte *ptr = readbuf + info.argloc[0].c_offset;
		regcache->cooked_read_part (regnum, 0,
					    info.argloc[0].c_length,
					    ptr);
	      }

	    if (writebuf)
	      {
		const gdb_byte *ptr = writebuf + info.argloc[0].c_offset;
		riscv_regcache_cooked_write (regnum, ptr,
					     info.argloc[0].c_length,
					     regcache, call_info.flen);
	      }

	    /* A return value in register can have a second part in a
	       second register.  */
	    if (info.argloc[1].c_length > 0)
	      {
		switch (info.argloc[1].loc_type)
		  {
		  case riscv_arg_info::location::in_reg:
		    regnum = info.argloc[1].loc_data.regno;

		    gdb_assert ((info.argloc[0].c_length
				 + info.argloc[1].c_length) <= arg_len);
		    gdb_assert (info.argloc[1].c_length
				<= register_size (gdbarch, regnum));

		    if (readbuf)
		      {
			readbuf += info.argloc[1].c_offset;
			regcache->cooked_read_part (regnum, 0,
						    info.argloc[1].c_length,
						    readbuf);
		      }

		    if (writebuf)
		      {
			const gdb_byte *ptr
			  = writebuf + info.argloc[1].c_offset;
			riscv_regcache_cooked_write
			  (regnum, ptr, info.argloc[1].c_length,
			   regcache, call_info.flen);
		      }
		    break;

		  case riscv_arg_info::location::by_ref:
		  case riscv_arg_info::location::on_stack:
		  default:
		    error (_("invalid argument location"));
		    break;
		  }
	      }
	  }
	  break;

	  /* Return value by reference will have its address in A0.  */
	case riscv_arg_info::location::by_ref:
	  {
	    ULONGEST addr;

	    regcache_cooked_read_unsigned (regcache, RISCV_A0_REGNUM,
					   &addr);
	    if (read_value != nullptr)
	      {
		abi_val = value_at_non_lval (type, addr);
		/* Also reset the expected type, so that the cast
		   later on is a no-op.  If the cast is not a no-op,
		   and if the return type is variably-sized, then the
		   type of ABI_VAL will differ from ARG_TYPE due to
		   dynamic type resolution, and so will most likely
		   fail.  */
		arg_type = abi_val->type ();
	      }
	    if (writebuf != nullptr)
	      write_memory (addr, writebuf, info.length);
	  }
	  break;

	case riscv_arg_info::location::on_stack:
	default:
	  error (_("invalid argument location"));
	  break;
	}

      /* This completes the cast from abi type back to the declared type
	 in the case that we are reading from the machine.  See the
	 comment at the head of this block for more details.  */
      if (read_value != nullptr)
	{
	  if (is_fixed_point_type (arg_type))
	    {
	      /* Convert abi_val to the actual return type, but
		 being careful to preserve the fact that abi_val
		 is unscaled.  */
	      gdb_mpz unscaled;

	      unscaled.read (abi_val->contents (),
			     type_byte_order (info.type),
			     info.type->is_unsigned ());
	      *read_value = value::allocate (arg_type);
	      unscaled.write ((*read_value)->contents_raw (),
			      type_byte_order (arg_type),
			      arg_type->is_unsigned ());
	    }
	  else
	    *read_value = value_cast (arg_type, abi_val);
	}
    }

  switch (info.argloc[0].loc_type)
    {
    case riscv_arg_info::location::in_reg:
      return RETURN_VALUE_REGISTER_CONVENTION;
    case riscv_arg_info::location::by_ref:
      return RETURN_VALUE_ABI_PRESERVES_ADDRESS;
    case riscv_arg_info::location::on_stack:
    default:
      error (_("invalid argument location"));
    }
}

/* Implement the frame_align gdbarch method.  */

static CORE_ADDR
riscv_frame_align (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return align_down (addr, 16);
}

/* Generate, or return the cached frame cache for the RiscV frame
   unwinder.  */

static struct riscv_unwind_cache *
riscv_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  CORE_ADDR pc, start_addr;
  struct riscv_unwind_cache *cache;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int numregs, regno;

  if ((*this_cache) != NULL)
    return (struct riscv_unwind_cache *) *this_cache;

  cache = FRAME_OBSTACK_ZALLOC (struct riscv_unwind_cache);
  cache->regs = trad_frame_alloc_saved_regs (this_frame);
  (*this_cache) = cache;

  /* Scan the prologue, filling in the cache.  */
  start_addr = get_frame_func (this_frame);
  pc = get_frame_pc (this_frame);
  riscv_scan_prologue (gdbarch, start_addr, pc, cache);

  /* We can now calculate the frame base address.  */
  cache->frame_base
    = (get_frame_register_unsigned (this_frame, cache->frame_base_reg)
       + cache->frame_base_offset);
  riscv_unwinder_debug_printf ("frame base is %s ($%s + 0x%x)",
			       core_addr_to_string (cache->frame_base),
			       gdbarch_register_name (gdbarch,
						      cache->frame_base_reg),
			       cache->frame_base_offset);

  /* The prologue scanner sets the address of registers stored to the stack
     as the offset of that register from the frame base.  The prologue
     scanner doesn't know the actual frame base value, and so is unable to
     compute the exact address.  We do now know the frame base value, so
     update the address of registers stored to the stack.  */
  numregs = gdbarch_num_regs (gdbarch) + gdbarch_num_pseudo_regs (gdbarch);
  for (regno = 0; regno < numregs; ++regno)
    {
      if (cache->regs[regno].is_addr ())
	cache->regs[regno].set_addr (cache->regs[regno].addr ()
				     + cache->frame_base);
    }

  /* The previous $pc can be found wherever the $ra value can be found.
     The previous $ra value is gone, this would have been stored be the
     previous frame if required.  */
  cache->regs[gdbarch_pc_regnum (gdbarch)] = cache->regs[RISCV_RA_REGNUM];
  cache->regs[RISCV_RA_REGNUM].set_unknown ();

  /* Build the frame id.  */
  cache->this_id = frame_id_build (cache->frame_base, start_addr);

  /* The previous $sp value is the frame base value.  */
  cache->regs[gdbarch_sp_regnum (gdbarch)].set_value (cache->frame_base);

  return cache;
}

/* Implement the this_id callback for RiscV frame unwinder.  */

static void
riscv_frame_this_id (frame_info_ptr this_frame,
		     void **prologue_cache,
		     struct frame_id *this_id)
{
  struct riscv_unwind_cache *cache;

  try
    {
      cache = riscv_frame_cache (this_frame, prologue_cache);
      *this_id = cache->this_id;
    }
  catch (const gdb_exception_error &ex)
    {
      /* Ignore errors, this leaves the frame id as the predefined outer
	 frame id which terminates the backtrace at this point.  */
    }
}

/* Implement the prev_register callback for RiscV frame unwinder.  */

static struct value *
riscv_frame_prev_register (frame_info_ptr this_frame,
			   void **prologue_cache,
			   int regnum)
{
  struct riscv_unwind_cache *cache;

  cache = riscv_frame_cache (this_frame, prologue_cache);
  return trad_frame_get_prev_register (this_frame, cache->regs, regnum);
}

/* Structure defining the RiscV normal frame unwind functions.  Since we
   are the fallback unwinder (DWARF unwinder is used first), we use the
   default frame sniffer, which always accepts the frame.  */

static const struct frame_unwind riscv_frame_unwind =
{
  /*.name          =*/ "riscv prologue",
  /*.type          =*/ NORMAL_FRAME,
  /*.stop_reason   =*/ default_frame_unwind_stop_reason,
  /*.this_id       =*/ riscv_frame_this_id,
  /*.prev_register =*/ riscv_frame_prev_register,
  /*.unwind_data   =*/ NULL,
  /*.sniffer       =*/ default_frame_sniffer,
  /*.dealloc_cache =*/ NULL,
  /*.prev_arch     =*/ NULL,
};

/* Extract a set of required target features out of ABFD.  If ABFD is
   nullptr then a RISCV_GDBARCH_FEATURES is returned in its default state.  */

static struct riscv_gdbarch_features
riscv_features_from_bfd (const bfd *abfd)
{
  struct riscv_gdbarch_features features;

  /* Now try to improve on the defaults by looking at the binary we are
     going to execute.  We assume the user knows what they are doing and
     that the target will match the binary.  Remember, this code path is
     only used at all if the target hasn't given us a description, so this
     is really a last ditched effort to do something sane before giving
     up.  */
  if (abfd != nullptr && bfd_get_flavour (abfd) == bfd_target_elf_flavour)
    {
      unsigned char eclass = elf_elfheader (abfd)->e_ident[EI_CLASS];
      int e_flags = elf_elfheader (abfd)->e_flags;

      if (eclass == ELFCLASS32)
	features.xlen = 4;
      else if (eclass == ELFCLASS64)
	features.xlen = 8;
      else
	internal_error (_("unknown ELF header class %d"), eclass);

      if (e_flags & EF_RISCV_FLOAT_ABI_DOUBLE)
	features.flen = 8;
      else if (e_flags & EF_RISCV_FLOAT_ABI_SINGLE)
	features.flen = 4;

      if (e_flags & EF_RISCV_RVE)
	{
	  if (features.xlen == 8)
	    {
	      warning (_("64-bit ELF with RV32E flag set!  Assuming 32-bit"));
	      features.xlen = 4;
	    }
	  features.embedded = true;
	}
    }

  return features;
}

/* Find a suitable default target description.  Use the contents of INFO,
   specifically the bfd object being executed, to guide the selection of a
   suitable default target description.  */

static const struct target_desc *
riscv_find_default_target_description (const struct gdbarch_info info)
{
  /* Extract desired feature set from INFO.  */
  struct riscv_gdbarch_features features
    = riscv_features_from_bfd (info.abfd);

  /* If the XLEN field is still 0 then we got nothing useful from INFO.BFD,
     maybe there was no bfd object.  In this case we fall back to a minimal
     useful target with no floating point, the x-register size is selected
     based on the architecture from INFO.  */
  if (features.xlen == 0)
    features.xlen = info.bfd_arch_info->bits_per_word == 32 ? 4 : 8;

  /* Now build a target description based on the feature set.  */
  return riscv_lookup_target_description (features);
}

/* Add all the RISC-V specific register groups into GDBARCH.  */

static void
riscv_add_reggroups (struct gdbarch *gdbarch)
{
  reggroup_add (gdbarch, csr_reggroup);
}

/* Implement the "dwarf2_reg_to_regnum" gdbarch method.  */

static int
riscv_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  if (reg <= RISCV_DWARF_REGNUM_X31)
    return RISCV_ZERO_REGNUM + (reg - RISCV_DWARF_REGNUM_X0);

  else if (reg <= RISCV_DWARF_REGNUM_F31)
    return RISCV_FIRST_FP_REGNUM + (reg - RISCV_DWARF_REGNUM_F0);

  else if (reg >= RISCV_DWARF_FIRST_CSR && reg <= RISCV_DWARF_LAST_CSR)
    return RISCV_FIRST_CSR_REGNUM + (reg - RISCV_DWARF_FIRST_CSR);

  else if (reg >= RISCV_DWARF_REGNUM_V0 && reg <= RISCV_DWARF_REGNUM_V31)
    return RISCV_V0_REGNUM + (reg - RISCV_DWARF_REGNUM_V0);

  return -1;
}

/* Implement the gcc_target_options method.  We have to select the arch and abi
   from the feature info.  We have enough feature info to select the abi, but
   not enough info for the arch given all of the possible architecture
   extensions.  So choose reasonable defaults for now.  */

static std::string
riscv_gcc_target_options (struct gdbarch *gdbarch)
{
  int isa_xlen = riscv_isa_xlen (gdbarch);
  int isa_flen = riscv_isa_flen (gdbarch);
  int abi_xlen = riscv_abi_xlen (gdbarch);
  int abi_flen = riscv_abi_flen (gdbarch);
  std::string target_options;

  target_options = "-march=rv";
  if (isa_xlen == 8)
    target_options += "64";
  else
    target_options += "32";
  if (isa_flen == 8)
    target_options += "gc";
  else if (isa_flen == 4)
    target_options += "imafc";
  else
    target_options += "imac";

  target_options += " -mabi=";
  if (abi_xlen == 8)
    target_options += "lp64";
  else
    target_options += "ilp32";
  if (abi_flen == 8)
    target_options += "d";
  else if (abi_flen == 4)
    target_options += "f";

  /* The gdb loader doesn't handle link-time relaxation relocations.  */
  target_options += " -mno-relax";

  return target_options;
}

/* Call back from tdesc_use_registers, called for each unknown register
   found in the target description.

   See target-description.h (typedef tdesc_unknown_register_ftype) for a
   discussion of the arguments and return values.  */

static int
riscv_tdesc_unknown_reg (struct gdbarch *gdbarch, tdesc_feature *feature,
			 const char *reg_name, int possible_regnum)
{
  /* At one point in time GDB had an incorrect default target description
     that duplicated the fflags, frm, and fcsr registers in both the FPU
     and CSR register sets.

     Some targets (QEMU) copied these target descriptions into their source
     tree, and so we're now stuck working with some versions of QEMU that
     declare the same registers twice.

     To make matters worse, if GDB tries to read or write to these
     registers using the register number assigned in the FPU feature set,
     then QEMU will fail to read the register, so we must use the register
     number declared in the CSR feature set.

     Luckily, GDB scans the FPU feature first, and then the CSR feature,
     which means that the CSR feature will be the one we end up using, the
     versions of these registers in the FPU feature will appear as unknown
     registers and will be passed through to this code.

     To prevent these duplicate registers showing up in any of the register
     lists, and to prevent GDB every trying to access the FPU feature copies,
     we spot the three problematic registers here, and record the register
     number that GDB has assigned them.  Then in riscv_register_name we will
     return no name for the three duplicates, this hides the duplicates from
     the user.  */
  if (strcmp (tdesc_feature_name (feature), riscv_freg_feature.name ()) == 0)
    {
      riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
      int *regnum_ptr = nullptr;

      if (strcmp (reg_name, "fflags") == 0)
	regnum_ptr = &tdep->duplicate_fflags_regnum;
      else if (strcmp (reg_name, "frm") == 0)
	regnum_ptr = &tdep->duplicate_frm_regnum;
      else if (strcmp (reg_name, "fcsr") == 0)
	regnum_ptr = &tdep->duplicate_fcsr_regnum;

      if (regnum_ptr != nullptr)
	{
	  /* This means the register appears more than twice in the target
	     description.  Just let GDB add this as another register.
	     We'll have duplicates in the register name list, but there's
	     not much more we can do.  */
	  if (*regnum_ptr != -1)
	    return -1;

	  /* Record the number assigned to this register, then return the
	     number (so it actually gets assigned to this register).  */
	  *regnum_ptr = possible_regnum;
	  return possible_regnum;
	}
    }

  /* Any unknown registers in the CSR feature are recorded within a single
     block so we can easily identify these registers when making choices
     about register groups in riscv_register_reggroup_p.  */
  if (strcmp (tdesc_feature_name (feature), riscv_csr_feature.name ()) == 0)
    {
      riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
      if (tdep->unknown_csrs_first_regnum == -1)
	tdep->unknown_csrs_first_regnum = possible_regnum;
      gdb_assert (tdep->unknown_csrs_first_regnum
		  + tdep->unknown_csrs_count == possible_regnum);
      tdep->unknown_csrs_count++;
      return possible_regnum;
    }

  /* Some other unknown register.  Don't assign this a number now, it will
     be assigned a number automatically later by the target description
     handling code.  */
  return -1;
}

/* Implement the gnu_triplet_regexp method.  A single compiler supports both
   32-bit and 64-bit code, and may be named riscv32 or riscv64 or (not
   recommended) riscv.  */

static const char *
riscv_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  return "riscv(32|64)?";
}

/* Implementation of `gdbarch_stap_is_single_operand', as defined in
   gdbarch.h.  */

static int
riscv_stap_is_single_operand (struct gdbarch *gdbarch, const char *s)
{
  return (ISDIGIT (*s) /* Literal number.  */
	  || *s == '(' /* Register indirection.  */
	  || ISALPHA (*s)); /* Register value.  */
}

/* String that appears before a register name in a SystemTap register
   indirect expression.  */

static const char *const stap_register_indirection_prefixes[] =
{
  "(", nullptr
};

/* String that appears after a register name in a SystemTap register
   indirect expression.  */

static const char *const stap_register_indirection_suffixes[] =
{
  ")", nullptr
};

/* Initialize the current architecture based on INFO.  If possible,
   re-use an architecture from ARCHES, which is a list of
   architectures already created during this debugging session.

   Called e.g. at program startup, when reading a core file, and when
   reading a binary file.  */

static struct gdbarch *
riscv_gdbarch_init (struct gdbarch_info info,
		    struct gdbarch_list *arches)
{
  struct riscv_gdbarch_features features;
  const struct target_desc *tdesc = info.target_desc;

  /* Ensure we always have a target description.  */
  if (!tdesc_has_registers (tdesc))
    tdesc = riscv_find_default_target_description (info);
  gdb_assert (tdesc != nullptr);

  riscv_gdbarch_debug_printf ("have got a target description");

  tdesc_arch_data_up tdesc_data = tdesc_data_alloc ();
  std::vector<riscv_pending_register_alias> pending_aliases;

  bool valid_p = (riscv_xreg_feature.check (tdesc, tdesc_data.get (),
					    &pending_aliases, &features)
		  && riscv_freg_feature.check (tdesc, tdesc_data.get (),
					       &pending_aliases, &features)
		  && riscv_virtual_feature.check (tdesc, tdesc_data.get (),
						  &pending_aliases, &features)
		  && riscv_csr_feature.check (tdesc, tdesc_data.get (),
					      &pending_aliases, &features)
		  && riscv_vector_feature.check (tdesc, tdesc_data.get (),
						 &pending_aliases, &features));
  if (!valid_p)
    {
      riscv_gdbarch_debug_printf ("target description is not valid");
      return NULL;
    }

  if (tdesc_found_register (tdesc_data.get (), RISCV_CSR_FFLAGS_REGNUM))
    features.has_fflags_reg = true;
  if (tdesc_found_register (tdesc_data.get (), RISCV_CSR_FRM_REGNUM))
    features.has_frm_reg = true;
  if (tdesc_found_register (tdesc_data.get (), RISCV_CSR_FCSR_REGNUM))
    features.has_fcsr_reg = true;

  /* Have a look at what the supplied (if any) bfd object requires of the
     target, then check that this matches with what the target is
     providing.  */
  struct riscv_gdbarch_features abi_features
    = riscv_features_from_bfd (info.abfd);

  /* If the ABI_FEATURES xlen is 0 then this indicates we got no useful abi
     features from the INFO object.  In this case we just treat the
     hardware features as defining the abi.  */
  if (abi_features.xlen == 0)
    abi_features = features;

  /* In theory a binary compiled for RV32 could run on an RV64 target,
     however, this has not been tested in GDB yet, so for now we require
     that the requested xlen match the targets xlen.  */
  if (abi_features.xlen != features.xlen)
    error (_("bfd requires xlen %d, but target has xlen %d"),
	    abi_features.xlen, features.xlen);
  /* We do support running binaries compiled for 32-bit float on targets
     with 64-bit float, so we only complain if the binary requires more
     than the target has available.  */
  if (abi_features.flen > features.flen)
    error (_("bfd requires flen %d, but target has flen %d"),
	    abi_features.flen, features.flen);

  /* Find a candidate among the list of pre-declared architectures.  */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      /* Check that the feature set of the ARCHES matches the feature set
	 we are looking for.  If it doesn't then we can't reuse this
	 gdbarch.  */
      riscv_gdbarch_tdep *other_tdep
	= gdbarch_tdep<riscv_gdbarch_tdep> (arches->gdbarch);

      if (other_tdep->isa_features != features
	  || other_tdep->abi_features != abi_features)
	continue;

      break;
    }

  if (arches != NULL)
    return arches->gdbarch;

  /* None found, so create a new architecture from the information provided.  */
  gdbarch *gdbarch
    = gdbarch_alloc (&info, gdbarch_tdep_up (new riscv_gdbarch_tdep));
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  tdep->isa_features = features;
  tdep->abi_features = abi_features;

  /* Target data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, riscv_isa_xlen (gdbarch) * 8);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 128);
  set_gdbarch_long_double_format (gdbarch, floatformats_ieee_quad);
  set_gdbarch_ptr_bit (gdbarch, riscv_isa_xlen (gdbarch) * 8);
  set_gdbarch_char_signed (gdbarch, 0);
  set_gdbarch_type_align (gdbarch, riscv_type_align);

  /* Information about the target architecture.  */
  set_gdbarch_return_value_as_value (gdbarch, riscv_return_value);
  set_gdbarch_breakpoint_kind_from_pc (gdbarch, riscv_breakpoint_kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch, riscv_sw_breakpoint_from_kind);
  set_gdbarch_have_nonsteppable_watchpoint (gdbarch, 1);

  /* Functions to analyze frames.  */
  set_gdbarch_skip_prologue (gdbarch, riscv_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_frame_align (gdbarch, riscv_frame_align);

  /* Functions handling dummy frames.  */
  set_gdbarch_call_dummy_location (gdbarch, ON_STACK);
  set_gdbarch_push_dummy_code (gdbarch, riscv_push_dummy_code);
  set_gdbarch_push_dummy_call (gdbarch, riscv_push_dummy_call);

  /* Frame unwinders.  Use DWARF debug info if available, otherwise use our own
     unwinder.  */
  dwarf2_append_unwinders (gdbarch);
  frame_unwind_append_unwinder (gdbarch, &riscv_frame_unwind);

  /* Register architecture.  */
  riscv_add_reggroups (gdbarch);

  /* Internal <-> external register number maps.  */
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, riscv_dwarf_reg_to_regnum);

  /* We reserve all possible register numbers for the known registers.
     This means the target description mechanism will add any target
     specific registers after this number.  This helps make debugging GDB
     just a little easier.  */
  set_gdbarch_num_regs (gdbarch, RISCV_LAST_REGNUM + 1);

  /* Some specific register numbers GDB likes to know about.  */
  set_gdbarch_sp_regnum (gdbarch, RISCV_SP_REGNUM);
  set_gdbarch_pc_regnum (gdbarch, RISCV_PC_REGNUM);

  set_gdbarch_print_registers_info (gdbarch, riscv_print_registers_info);

  set_tdesc_pseudo_register_name (gdbarch, riscv_pseudo_register_name);
  set_tdesc_pseudo_register_type (gdbarch, riscv_pseudo_register_type);
  set_tdesc_pseudo_register_reggroup_p (gdbarch,
					riscv_pseudo_register_reggroup_p);
  set_gdbarch_pseudo_register_read (gdbarch, riscv_pseudo_register_read);
  set_gdbarch_deprecated_pseudo_register_write (gdbarch,
						riscv_pseudo_register_write);

  /* Finalise the target description registers.  */
  tdesc_use_registers (gdbarch, tdesc, std::move (tdesc_data),
		       riscv_tdesc_unknown_reg);

  /* Calculate the number of pseudo registers we need.  The fflags and frm
     registers are sub-fields of the fcsr CSR register (csr3).  However,
     these registers can also be accessed directly as separate CSR
     registers (fflags is csr1, and frm is csr2).  And so, some targets
     might choose to offer direct access to all three registers in the
     target description, while other targets might choose to only offer
     access to fcsr.

     As we scan the target description we spot which of fcsr, fflags, and
     frm are available.  If fcsr is available but either of fflags and/or
     frm are not available, then we add pseudo-registers to provide the
     missing functionality.

     This has to be done after the call to tdesc_use_registers as we don't
     know the final register number until after that call, and the pseudo
     register numbers need to be after the physical registers.  */
  int num_pseudo_regs = 0;
  int next_pseudo_regnum = gdbarch_num_regs (gdbarch);

  if (features.has_fflags_reg)
    tdep->fflags_regnum = RISCV_CSR_FFLAGS_REGNUM;
  else if (features.has_fcsr_reg)
    {
      tdep->fflags_regnum = next_pseudo_regnum;
      pending_aliases.emplace_back ("csr1", (void *) &tdep->fflags_regnum);
      next_pseudo_regnum++;
      num_pseudo_regs++;
    }

  if (features.has_frm_reg)
    tdep->frm_regnum = RISCV_CSR_FRM_REGNUM;
  else if (features.has_fcsr_reg)
    {
      tdep->frm_regnum = next_pseudo_regnum;
      pending_aliases.emplace_back ("csr2", (void *) &tdep->frm_regnum);
      next_pseudo_regnum++;
      num_pseudo_regs++;
    }

  set_gdbarch_num_pseudo_regs (gdbarch, num_pseudo_regs);

  /* Override the register type callback setup by the target description
     mechanism.  This allows us to provide special type for floating point
     registers.  */
  set_gdbarch_register_type (gdbarch, riscv_register_type);

  /* Override the register name callback setup by the target description
     mechanism.  This allows us to force our preferred names for the
     registers, no matter what the target description called them.  */
  set_gdbarch_register_name (gdbarch, riscv_register_name);

  /* Tell GDB which RISC-V registers are read-only. */
  set_gdbarch_cannot_store_register (gdbarch, riscv_cannot_store_register);

  /* Override the register group callback setup by the target description
     mechanism.  This allows us to force registers into the groups we
     want, ignoring what the target tells us.  */
  set_gdbarch_register_reggroup_p (gdbarch, riscv_register_reggroup_p);

  /* Create register aliases for alternative register names.  We only
     create aliases for registers which were mentioned in the target
     description.  */
  for (const auto &alias : pending_aliases)
    alias.create (gdbarch);

  /* Compile command hooks.  */
  set_gdbarch_gcc_target_options (gdbarch, riscv_gcc_target_options);
  set_gdbarch_gnu_triplet_regexp (gdbarch, riscv_gnu_triplet_regexp);

  /* Disassembler options support.  */
  set_gdbarch_valid_disassembler_options (gdbarch,
					  disassembler_options_riscv ());
  set_gdbarch_disassembler_options (gdbarch, &riscv_disassembler_options);

  /* SystemTap Support.  */
  set_gdbarch_stap_is_single_operand (gdbarch, riscv_stap_is_single_operand);
  set_gdbarch_stap_register_indirection_prefixes
    (gdbarch, stap_register_indirection_prefixes);
  set_gdbarch_stap_register_indirection_suffixes
    (gdbarch, stap_register_indirection_suffixes);

  /* Hook in OS ABI-specific overrides, if they have been registered.  */
  gdbarch_init_osabi (info, gdbarch);

  register_riscv_ravenscar_ops (gdbarch);

  return gdbarch;
}

/* This decodes the current instruction and determines the address of the
   next instruction.  */

static CORE_ADDR
riscv_next_pc (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  const riscv_gdbarch_tdep *tdep
    = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);
  struct riscv_insn insn;
  CORE_ADDR next_pc;

  insn.decode (gdbarch, pc);
  next_pc = pc + insn.length ();

  if (insn.opcode () == riscv_insn::JAL)
    next_pc = pc + insn.imm_signed ();
  else if (insn.opcode () == riscv_insn::JALR)
    {
      LONGEST source;
      regcache->cooked_read (insn.rs1 (), &source);
      next_pc = (source + insn.imm_signed ()) & ~(CORE_ADDR) 0x1;
    }
  else if (insn.opcode () == riscv_insn::BEQ)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 == src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BNE)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 != src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BLT)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 < src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BGE)
    {
      LONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 >= src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BLTU)
    {
      ULONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 < src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::BGEU)
    {
      ULONGEST src1, src2;
      regcache->cooked_read (insn.rs1 (), &src1);
      regcache->cooked_read (insn.rs2 (), &src2);
      if (src1 >= src2)
	next_pc = pc + insn.imm_signed ();
    }
  else if (insn.opcode () == riscv_insn::ECALL)
    {
      if (tdep->syscall_next_pc != nullptr)
	next_pc = tdep->syscall_next_pc (get_current_frame ());
    }

  return next_pc;
}

/* Return true if INSN is not a control transfer instruction and is allowed to
   appear in the middle of the lr/sc sequence.  */

static bool
riscv_insn_is_non_cti_and_allowed_in_atomic_sequence
  (const struct riscv_insn &insn)
{
  switch (insn.opcode ())
    {
    case riscv_insn::LUI:
    case riscv_insn::AUIPC:
    case riscv_insn::ADDI:
    case riscv_insn::ADDIW:
    case riscv_insn::SLTI:
    case riscv_insn::SLTIU:
    case riscv_insn::XORI:
    case riscv_insn::ORI:
    case riscv_insn::ANDI:
    case riscv_insn::SLLI:
    case riscv_insn::SLLIW:
    case riscv_insn::SRLI:
    case riscv_insn::SRLIW:
    case riscv_insn::SRAI:
    case riscv_insn::ADD:
    case riscv_insn::ADDW:
    case riscv_insn::SRAIW:
    case riscv_insn::SUB:
    case riscv_insn::SUBW:
    case riscv_insn::SLL:
    case riscv_insn::SLLW:
    case riscv_insn::SLT:
    case riscv_insn::SLTU:
    case riscv_insn::XOR:
    case riscv_insn::SRL:
    case riscv_insn::SRLW:
    case riscv_insn::SRA:
    case riscv_insn::SRAW:
    case riscv_insn::OR:
    case riscv_insn::AND:
      return true;
    }

    return false;
}

/* Return true if INSN is a direct branch instruction.  */

static bool
riscv_insn_is_direct_branch (const struct riscv_insn &insn)
{
  switch (insn.opcode ())
    {
    case riscv_insn::BEQ:
    case riscv_insn::BNE:
    case riscv_insn::BLT:
    case riscv_insn::BGE:
    case riscv_insn::BLTU:
    case riscv_insn::BGEU:
    case riscv_insn::JAL:
      return true;
    }

    return false;
}

/* We can't put a breakpoint in the middle of a lr/sc atomic sequence, so look
   for the end of the sequence and put the breakpoint there.  */

static std::vector<CORE_ADDR>
riscv_deal_with_atomic_sequence (struct regcache *regcache, CORE_ADDR pc)
{
  struct gdbarch *gdbarch = regcache->arch ();
  struct riscv_insn insn;
  CORE_ADDR cur_step_pc = pc, next_pc;
  std::vector<CORE_ADDR> next_pcs;
  bool found_valid_atomic_sequence = false;
  enum riscv_insn::opcode lr_opcode;

  /* First instruction has to be a load reserved.  */
  insn.decode (gdbarch, cur_step_pc);
  lr_opcode = insn.opcode ();
  if (lr_opcode != riscv_insn::LR_D && lr_opcode != riscv_insn::LR_W)
    return {};

  /* The loop comprises only an LR/SC sequence and code to retry the sequence in
     the case of failure, and must comprise at most 16 instructions placed
     sequentially in memory.  While our code tries to follow these restrictions,
     it has the following limitations:

       (a) We expect the loop to start with an LR and end with a BNE.
	   Apparently this does not cover all cases for a valid sequence.
       (b) The atomic limitations only apply to the code that is actually
	   executed, so here again it's overly restrictive.
       (c) The lr/sc are required to be for the same target address, but this
	   information is only known at runtime.  Same as (b), in order to check
	   this we will end up needing to simulate the sequence, which is more
	   complicated than what we're doing right now.

     Also note that we only expect a maximum of (16-2) instructions in the for
     loop as we have assumed the presence of LR and BNE at the beginning and end
     respectively.  */
  for (int insn_count = 0; insn_count < 16 - 2; ++insn_count)
    {
      cur_step_pc += insn.length ();
      insn.decode (gdbarch, cur_step_pc);

      /* The dynamic code executed between lr/sc can only contain instructions
	 from the base I instruction set, excluding loads, stores, backward
	 jumps, taken backward branches, JALR, FENCE, FENCE.I, and SYSTEM
	 instructions.  If the C extension is supported, then compressed forms
	 of the aforementioned I instructions are also permitted.  */

      if (riscv_insn_is_non_cti_and_allowed_in_atomic_sequence (insn))
	continue;
      /* Look for a conditional branch instruction, check if it's taken forward
	 or not.  */
      else if (riscv_insn_is_direct_branch (insn))
	{
	  if (insn.imm_signed () > 0)
	    {
	      next_pc = cur_step_pc + insn.imm_signed ();
	      next_pcs.push_back (next_pc);
	    }
	  else
	    break;
	}
      /* Look for a paired SC instruction which closes the atomic sequence.  */
      else if ((insn.opcode () == riscv_insn::SC_D
		&& lr_opcode == riscv_insn::LR_D)
	       || (insn.opcode () == riscv_insn::SC_W
		   && lr_opcode == riscv_insn::LR_W))
	found_valid_atomic_sequence = true;
      else
	break;
    }

  if (!found_valid_atomic_sequence)
    return {};

  /* Next instruction should be branch to start.  */
  insn.decode (gdbarch, cur_step_pc);
  if (insn.opcode () != riscv_insn::BNE)
    return {};
  if (pc != (cur_step_pc + insn.imm_signed ()))
    return {};
  cur_step_pc += insn.length ();

  /* Remove all PCs that jump within the sequence.  */
  auto matcher = [cur_step_pc] (const CORE_ADDR addr) -> bool
    {
      return addr < cur_step_pc;
    };
  auto it = std::remove_if (next_pcs.begin (), next_pcs.end (), matcher);
  next_pcs.erase (it, next_pcs.end ());

  next_pc = cur_step_pc;
  next_pcs.push_back (next_pc);
  return next_pcs;
}

/* This is called just before we want to resume the inferior, if we want to
   single-step it but there is no hardware or kernel single-step support.  We
   find the target of the coming instruction and breakpoint it.  */

std::vector<CORE_ADDR>
riscv_software_single_step (struct regcache *regcache)
{
  CORE_ADDR cur_pc = regcache_read_pc (regcache), next_pc;
  std::vector<CORE_ADDR> next_pcs
    = riscv_deal_with_atomic_sequence (regcache, cur_pc);

  if (!next_pcs.empty ())
    return next_pcs;

  next_pc = riscv_next_pc (regcache, cur_pc);

  return {next_pc};
}

/* Create RISC-V specific reggroups.  */

static void
riscv_init_reggroups ()
{
  csr_reggroup = reggroup_new ("csr", USER_REGGROUP);
}

/* See riscv-tdep.h.  */

void
riscv_supply_regset (const struct regset *regset,
		     struct regcache *regcache, int regnum,
		     const void *regs, size_t len)
{
  regcache->supply_regset (regset, regnum, regs, len);

  if (regnum == -1 || regnum == RISCV_ZERO_REGNUM)
    regcache->raw_supply_zeroed (RISCV_ZERO_REGNUM);

  struct gdbarch *gdbarch = regcache->arch ();
  riscv_gdbarch_tdep *tdep = gdbarch_tdep<riscv_gdbarch_tdep> (gdbarch);

  if (regnum == -1
      || regnum == tdep->fflags_regnum
      || regnum == tdep->frm_regnum)
    {
      int fcsr_regnum = RISCV_CSR_FCSR_REGNUM;

      /* Ensure that FCSR has been read into REGCACHE.  */
      if (regnum != -1)
	regcache->supply_regset (regset, fcsr_regnum, regs, len);

      /* Grab the FCSR value if it is now in the regcache.  We must check
	 the status first as, if the register was not supplied by REGSET,
	 this call will trigger a recursive attempt to fetch the
	 registers.  */
      if (regcache->get_register_status (fcsr_regnum) == REG_VALID)
	{
	  /* If we have an fcsr register then we should have fflags and frm
	     too, either provided by the target, or provided as a pseudo
	     register by GDB.  */
	  gdb_assert (tdep->fflags_regnum >= 0);
	  gdb_assert (tdep->frm_regnum >= 0);

	  ULONGEST fcsr_val;
	  regcache->raw_read (fcsr_regnum, &fcsr_val);

	  /* Extract the fflags and frm values.  */
	  ULONGEST fflags_val = fcsr_val & 0x1f;
	  ULONGEST frm_val = (fcsr_val >> 5) & 0x7;

	  /* And supply these if needed.  We can only supply real
	     registers, so don't try to supply fflags or frm if they are
	     implemented as pseudo-registers.  */
	  if ((regnum == -1 || regnum == tdep->fflags_regnum)
	      && tdep->fflags_regnum < gdbarch_num_regs (gdbarch))
	    regcache->raw_supply_integer (tdep->fflags_regnum,
					  (gdb_byte *) &fflags_val,
					  sizeof (fflags_val),
					  /* is_signed */ false);

	  if ((regnum == -1 || regnum == tdep->frm_regnum)
	      && tdep->frm_regnum < gdbarch_num_regs (gdbarch))
	    regcache->raw_supply_integer (tdep->frm_regnum,
					  (gdb_byte *)&frm_val,
					  sizeof (fflags_val),
					  /* is_signed */ false);
	}
    }
}

void _initialize_riscv_tdep ();
void
_initialize_riscv_tdep ()
{
  riscv_init_reggroups ();

  gdbarch_register (bfd_arch_riscv, riscv_gdbarch_init, NULL);

  /* Add root prefix command for all "set debug riscv" and "show debug
     riscv" commands.  */
  add_setshow_prefix_cmd ("riscv", no_class,
			  _("RISC-V specific debug commands."),
			  _("RISC-V specific debug commands."),
			  &setdebugriscvcmdlist, &showdebugriscvcmdlist,
			  &setdebuglist, &showdebuglist);

  add_setshow_boolean_cmd ("breakpoints", class_maintenance,
			   &riscv_debug_breakpoints,  _("\
Set riscv breakpoint debugging."), _("\
Show riscv breakpoint debugging."), _("\
When non-zero, print debugging information for the riscv specific parts\n\
of the breakpoint mechanism."),
			   nullptr,
			   show_riscv_debug_variable,
			   &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  add_setshow_boolean_cmd ("infcall", class_maintenance,
			   &riscv_debug_infcall,  _("\
Set riscv inferior call debugging."), _("\
Show riscv inferior call debugging."), _("\
When non-zero, print debugging information for the riscv specific parts\n\
of the inferior call mechanism."),
			   nullptr,
			   show_riscv_debug_variable,
			   &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  add_setshow_boolean_cmd ("unwinder", class_maintenance,
			   &riscv_debug_unwinder,  _("\
Set riscv stack unwinding debugging."), _("\
Show riscv stack unwinding debugging."), _("\
When on, print debugging information for the riscv specific parts\n\
of the stack unwinding mechanism."),
			   nullptr,
			   show_riscv_debug_variable,
			   &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  add_setshow_boolean_cmd ("gdbarch", class_maintenance,
			   &riscv_debug_gdbarch,  _("\
Set riscv gdbarch initialisation debugging."), _("\
Show riscv gdbarch initialisation debugging."), _("\
When non-zero, print debugging information for the riscv gdbarch\n\
initialisation process."),
			   nullptr,
			   show_riscv_debug_variable,
			   &setdebugriscvcmdlist, &showdebugriscvcmdlist);

  /* Add root prefix command for all "set riscv" and "show riscv" commands.  */
  add_setshow_prefix_cmd ("riscv", no_class,
			  _("RISC-V specific commands."),
			  _("RISC-V specific commands."),
			  &setriscvcmdlist, &showriscvcmdlist,
			  &setlist, &showlist);


  use_compressed_breakpoints = AUTO_BOOLEAN_AUTO;
  add_setshow_auto_boolean_cmd ("use-compressed-breakpoints", no_class,
				&use_compressed_breakpoints,
				_("\
Set debugger's use of compressed breakpoints."), _("	\
Show debugger's use of compressed breakpoints."), _("\
Debugging compressed code requires compressed breakpoints to be used. If\n\
left to 'auto' then gdb will use them if the existing instruction is a\n\
compressed instruction. If that doesn't give the correct behavior, then\n\
this option can be used."),
				NULL,
				show_use_compressed_breakpoints,
				&setriscvcmdlist,
				&showriscvcmdlist);
}
