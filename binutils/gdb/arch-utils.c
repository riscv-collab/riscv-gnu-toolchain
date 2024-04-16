/* Dynamic architecture support for GDB, the GNU debugger.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

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

#include "arch-utils.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "infrun.h"
#include "regcache.h"
#include "sim-regno.h"
#include "gdbcore.h"
#include "osabi.h"
#include "target-descriptions.h"
#include "objfiles.h"
#include "language.h"
#include "symtab.h"
#include "dummy-frame.h"
#include "frame-unwind.h"
#include "reggroups.h"
#include "auxv.h"
#include "observable.h"
#include "solib-target.h"

#include "gdbsupport/version.h"

#include "floatformat.h"

#include "dis-asm.h"

bool
default_displaced_step_hw_singlestep (struct gdbarch *gdbarch)
{
  return !gdbarch_software_single_step_p (gdbarch);
}

CORE_ADDR
displaced_step_at_entry_point (struct gdbarch *gdbarch)
{
  CORE_ADDR addr;
  int bp_len;

  addr = entry_point_address ();

  /* Inferior calls also use the entry point as a breakpoint location.
     We don't want displaced stepping to interfere with those
     breakpoints, so leave space.  */
  gdbarch_breakpoint_from_pc (gdbarch, &addr, &bp_len);
  addr += bp_len * 2;

  return addr;
}

int
legacy_register_sim_regno (struct gdbarch *gdbarch, int regnum)
{
  /* Only makes sense to supply raw registers.  */
  gdb_assert (regnum >= 0 && regnum < gdbarch_num_regs (gdbarch));
  /* NOTE: cagney/2002-05-13: The old code did it this way and it is
     suspected that some GDB/SIM combinations may rely on this
     behaviour.  The default should be one2one_register_sim_regno
     (below).  */
  if (gdbarch_register_name (gdbarch, regnum)[0] != '\0')
    return regnum;
  else
    return LEGACY_SIM_REGNO_IGNORE;
}

/* See arch-utils.h */

CORE_ADDR
default_remove_non_address_bits (struct gdbarch *gdbarch, CORE_ADDR pointer)
{
  /* By default, just return the pointer value.  */
  return pointer;
}

/* See arch-utils.h */

std::string
default_memtag_to_string (struct gdbarch *gdbarch, struct value *tag)
{
  error (_("This architecture has no method to convert a memory tag to"
	   " a string."));
}

/* See arch-utils.h */

bool
default_tagged_address_p (struct gdbarch *gdbarch, struct value *address)
{
  /* By default, assume the address is untagged.  */
  return false;
}

/* See arch-utils.h */

bool
default_memtag_matches_p (struct gdbarch *gdbarch, struct value *address)
{
  /* By default, assume the tags match.  */
  return true;
}

/* See arch-utils.h */

bool
default_set_memtags (struct gdbarch *gdbarch, struct value *address,
		     size_t length, const gdb::byte_vector &tags,
		     memtag_type tag_type)
{
  /* By default, return true (successful);  */
  return true;
}

/* See arch-utils.h */

struct value *
default_get_memtag (struct gdbarch *gdbarch, struct value *address,
		    memtag_type tag_type)
{
  /* By default, return no tag.  */
  return nullptr;
}

CORE_ADDR
generic_skip_trampoline_code (frame_info_ptr frame, CORE_ADDR pc)
{
  return 0;
}

CORE_ADDR
generic_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  return 0;
}

int
generic_in_solib_return_trampoline (struct gdbarch *gdbarch,
				    CORE_ADDR pc, const char *name)
{
  return 0;
}

int
generic_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  return 0;
}

int
default_code_of_frame_writable (struct gdbarch *gdbarch,
				frame_info_ptr frame)
{
  return 1;
}

/* Helper functions for gdbarch_inner_than */

int
core_addr_lessthan (CORE_ADDR lhs, CORE_ADDR rhs)
{
  return (lhs < rhs);
}

int
core_addr_greaterthan (CORE_ADDR lhs, CORE_ADDR rhs)
{
  return (lhs > rhs);
}

/* Misc helper functions for targets.  */

CORE_ADDR
core_addr_identity (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return addr;
}

CORE_ADDR
convert_from_func_ptr_addr_identity (struct gdbarch *gdbarch, CORE_ADDR addr,
				     struct target_ops *targ)
{
  return addr;
}

int
no_op_reg_to_regnum (struct gdbarch *gdbarch, int reg)
{
  return reg;
}

void
default_coff_make_msymbol_special (int val, struct minimal_symbol *msym)
{
  return;
}

/* See arch-utils.h.  */

void
default_make_symbol_special (struct symbol *sym, struct objfile *objfile)
{
  return;
}

/* See arch-utils.h.  */

CORE_ADDR
default_adjust_dwarf2_addr (CORE_ADDR pc)
{
  return pc;
}

/* See arch-utils.h.  */

CORE_ADDR
default_adjust_dwarf2_line (CORE_ADDR addr, int rel)
{
  return addr;
}

/* See arch-utils.h.  */

bool
default_execute_dwarf_cfa_vendor_op (struct gdbarch *gdbarch, gdb_byte op,
				     struct dwarf2_frame_state *fs)
{
  return false;
}

int
cannot_register_not (struct gdbarch *gdbarch, int regnum)
{
  return 0;
}

/* Legacy version of target_virtual_frame_pointer().  Assumes that
   there is an gdbarch_deprecated_fp_regnum and that it is the same,
   cooked or raw.  */

void
legacy_virtual_frame_pointer (struct gdbarch *gdbarch, 
			      CORE_ADDR pc,
			      int *frame_regnum,
			      LONGEST *frame_offset)
{
  /* FIXME: cagney/2002-09-13: This code is used when identifying the
     frame pointer of the current PC.  It is assuming that a single
     register and an offset can determine this.  I think it should
     instead generate a byte code expression as that would work better
     with things like Dwarf2's CFI.  */
  if (gdbarch_deprecated_fp_regnum (gdbarch) >= 0
      && gdbarch_deprecated_fp_regnum (gdbarch)
	   < gdbarch_num_regs (gdbarch))
    *frame_regnum = gdbarch_deprecated_fp_regnum (gdbarch);
  else if (gdbarch_sp_regnum (gdbarch) >= 0
	   && gdbarch_sp_regnum (gdbarch)
		< gdbarch_num_regs (gdbarch))
    *frame_regnum = gdbarch_sp_regnum (gdbarch);
  else
    /* Should this be an internal error?  I guess so, it is reflecting
       an architectural limitation in the current design.  */
    internal_error (_("No virtual frame pointer available"));
  *frame_offset = 0;
}

/* Return a floating-point format for a floating-point variable of
   length LEN in bits.  If non-NULL, NAME is the name of its type.
   If no suitable type is found, return NULL.  */

const struct floatformat **
default_floatformat_for_type (struct gdbarch *gdbarch,
			      const char *name, int len)
{
  const struct floatformat **format = NULL;

  /* Check if this is a bfloat16 type.  It has the same size as the
     IEEE half float type, so we use the base type name to tell them
     apart.  */
  if (name != nullptr && strcmp (name, "__bf16") == 0
      && len == gdbarch_bfloat16_bit (gdbarch))
    format = gdbarch_bfloat16_format (gdbarch);
  else if (len == gdbarch_half_bit (gdbarch))
    format = gdbarch_half_format (gdbarch);
  else if (len == gdbarch_float_bit (gdbarch))
    format = gdbarch_float_format (gdbarch);
  else if (len == gdbarch_double_bit (gdbarch))
    format = gdbarch_double_format (gdbarch);
  else if (len == gdbarch_long_double_bit (gdbarch))
    format = gdbarch_long_double_format (gdbarch);
  /* On i386 the 'long double' type takes 96 bits,
     while the real number of used bits is only 80,
     both in processor and in memory.
     The code below accepts the real bit size.  */
  else if (gdbarch_long_double_format (gdbarch) != NULL
	   && len == gdbarch_long_double_format (gdbarch)[0]->totalsize)
    format = gdbarch_long_double_format (gdbarch);

  return format;
}

int
generic_convert_register_p (struct gdbarch *gdbarch, int regnum,
			    struct type *type)
{
  return 0;
}

int
default_stabs_argument_has_addr (struct gdbarch *gdbarch, struct type *type)
{
  return 0;
}

int
generic_instruction_nullified (struct gdbarch *gdbarch,
			       struct regcache *regcache)
{
  return 0;
}

int
default_remote_register_number (struct gdbarch *gdbarch,
				int regno)
{
  return regno;
}

/* See arch-utils.h.  */

int
default_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range)
{
  return 0;
}


/* Functions to manipulate the endianness of the target.  */

static enum bfd_endian target_byte_order_user = BFD_ENDIAN_UNKNOWN;

static const char endian_big[] = "big";
static const char endian_little[] = "little";
static const char endian_auto[] = "auto";
static const char *const endian_enum[] =
{
  endian_big,
  endian_little,
  endian_auto,
  NULL,
};
static const char *set_endian_string = endian_auto;

enum bfd_endian
selected_byte_order (void)
{
  return target_byte_order_user;
}

/* Called by ``show endian''.  */

static void
show_endian (struct ui_file *file, int from_tty, struct cmd_list_element *c,
	     const char *value)
{
  if (target_byte_order_user == BFD_ENDIAN_UNKNOWN)
    if (gdbarch_byte_order (get_current_arch ()) == BFD_ENDIAN_BIG)
      gdb_printf (file, _("The target endianness is set automatically "
			  "(currently big endian).\n"));
    else
      gdb_printf (file, _("The target endianness is set automatically "
			  "(currently little endian).\n"));
  else
    if (target_byte_order_user == BFD_ENDIAN_BIG)
      gdb_printf (file,
		  _("The target is set to big endian.\n"));
    else
      gdb_printf (file,
		  _("The target is set to little endian.\n"));
}

static void
set_endian (const char *ignore_args, int from_tty, struct cmd_list_element *c)
{
  struct gdbarch_info info;

  if (set_endian_string == endian_auto)
    {
      target_byte_order_user = BFD_ENDIAN_UNKNOWN;
      if (! gdbarch_update_p (info))
	internal_error (_("set_endian: architecture update failed"));
    }
  else if (set_endian_string == endian_little)
    {
      info.byte_order = BFD_ENDIAN_LITTLE;
      if (! gdbarch_update_p (info))
	gdb_printf (gdb_stderr,
		    _("Little endian target not supported by GDB\n"));
      else
	target_byte_order_user = BFD_ENDIAN_LITTLE;
    }
  else if (set_endian_string == endian_big)
    {
      info.byte_order = BFD_ENDIAN_BIG;
      if (! gdbarch_update_p (info))
	gdb_printf (gdb_stderr,
		    _("Big endian target not supported by GDB\n"));
      else
	target_byte_order_user = BFD_ENDIAN_BIG;
    }
  else
    internal_error (_("set_endian: bad value"));

  show_endian (gdb_stdout, from_tty, NULL, NULL);
}

/* Given SELECTED, a currently selected BFD architecture, and
   TARGET_DESC, the current target description, return what
   architecture to use.

   SELECTED may be NULL, in which case we return the architecture
   associated with TARGET_DESC.  If SELECTED specifies a variant
   of the architecture associated with TARGET_DESC, return the
   more specific of the two.

   If SELECTED is a different architecture, but it is accepted as
   compatible by the target, we can use the target architecture.

   If SELECTED is obviously incompatible, warn the user.  */

static const struct bfd_arch_info *
choose_architecture_for_target (const struct target_desc *target_desc,
				const struct bfd_arch_info *selected)
{
  const struct bfd_arch_info *from_target = tdesc_architecture (target_desc);
  const struct bfd_arch_info *compat1, *compat2;

  if (selected == NULL)
    return from_target;

  if (from_target == NULL)
    return selected;

  /* struct bfd_arch_info objects are singletons: that is, there's
     supposed to be exactly one instance for a given machine.  So you
     can tell whether two are equivalent by comparing pointers.  */
  if (from_target == selected)
    return selected;

  /* BFD's 'A->compatible (A, B)' functions return zero if A and B are
     incompatible.  But if they are compatible, it returns the 'more
     featureful' of the two arches.  That is, if A can run code
     written for B, but B can't run code written for A, then it'll
     return A.

     Some targets (e.g. MIPS as of 2006-12-04) don't fully
     implement this, instead always returning NULL or the first
     argument.  We detect that case by checking both directions.  */

  compat1 = selected->compatible (selected, from_target);
  compat2 = from_target->compatible (from_target, selected);

  if (compat1 == NULL && compat2 == NULL)
    {
      /* BFD considers the architectures incompatible.  Check our
	 target description whether it accepts SELECTED as compatible
	 anyway.  */
      if (tdesc_compatible_p (target_desc, selected))
	return from_target;

      warning (_("Selected architecture %s is not compatible "
		 "with reported target architecture %s"),
	       selected->printable_name, from_target->printable_name);
      return selected;
    }

  if (compat1 == NULL)
    return compat2;
  if (compat2 == NULL)
    return compat1;
  if (compat1 == compat2)
    return compat1;

  /* If the two didn't match, but one of them was a default
     architecture, assume the more specific one is correct.  This
     handles the case where an executable or target description just
     says "mips", but the other knows which MIPS variant.  */
  if (compat1->the_default)
    return compat2;
  if (compat2->the_default)
    return compat1;

  /* We have no idea which one is better.  This is a bug, but not
     a critical problem; warn the user.  */
  warning (_("Selected architecture %s is ambiguous with "
	     "reported target architecture %s"),
	   selected->printable_name, from_target->printable_name);
  return selected;
}

/* Functions to manipulate the architecture of the target.  */

enum set_arch { set_arch_auto, set_arch_manual };

static const struct bfd_arch_info *target_architecture_user;

static const char *set_architecture_string;

const char *
selected_architecture_name (void)
{
  if (target_architecture_user == NULL)
    return NULL;
  else
    return set_architecture_string;
}

/* Called if the user enters ``show architecture'' without an
   argument.  */

static void
show_architecture (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  if (target_architecture_user == NULL)
    gdb_printf (file, _("The target architecture is set to "
			"\"auto\" (currently \"%s\").\n"),
		gdbarch_bfd_arch_info (get_current_arch ())->printable_name);
  else
    gdb_printf (file, _("The target architecture is set to \"%s\".\n"),
		set_architecture_string);
}


/* Called if the user enters ``set architecture'' with or without an
   argument.  */

static void
set_architecture (const char *ignore_args,
		  int from_tty, struct cmd_list_element *c)
{
  struct gdbarch_info info;

  if (strcmp (set_architecture_string, "auto") == 0)
    {
      target_architecture_user = NULL;
      if (!gdbarch_update_p (info))
	internal_error (_("could not select an architecture automatically"));
    }
  else
    {
      info.bfd_arch_info = bfd_scan_arch (set_architecture_string);
      if (info.bfd_arch_info == NULL)
	internal_error (_("set_architecture: bfd_scan_arch failed"));
      if (gdbarch_update_p (info))
	target_architecture_user = info.bfd_arch_info;
      else
	gdb_printf (gdb_stderr,
		    _("Architecture `%s' not recognized.\n"),
		    set_architecture_string);
    }
  show_architecture (gdb_stdout, from_tty, NULL, NULL);
}

/* Try to select a global architecture that matches "info".  Return
   non-zero if the attempt succeeds.  */
int
gdbarch_update_p (struct gdbarch_info info)
{
  struct gdbarch *new_gdbarch;

  /* Check for the current file.  */
  if (info.abfd == NULL)
    info.abfd = current_program_space->exec_bfd ();
  if (info.abfd == NULL)
    info.abfd = core_bfd;

  /* Check for the current target description.  */
  if (info.target_desc == NULL)
    info.target_desc = target_current_description ();

  new_gdbarch = gdbarch_find_by_info (info);

  /* If there no architecture by that name, reject the request.  */
  if (new_gdbarch == NULL)
    {
      if (gdbarch_debug)
	gdb_printf (gdb_stdlog, "gdbarch_update_p: "
		    "Architecture not found\n");
      return 0;
    }

  /* If it is the same old architecture, accept the request (but don't
     swap anything).  */
  if (new_gdbarch == current_inferior ()->arch ())
    {
      if (gdbarch_debug)
	gdb_printf (gdb_stdlog, "gdbarch_update_p: "
		    "Architecture %s (%s) unchanged\n",
		    host_address_to_string (new_gdbarch),
		    gdbarch_bfd_arch_info (new_gdbarch)->printable_name);
      return 1;
    }

  /* It's a new architecture, swap it in.  */
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog, "gdbarch_update_p: "
		"New architecture %s (%s) selected\n",
		host_address_to_string (new_gdbarch),
		gdbarch_bfd_arch_info (new_gdbarch)->printable_name);

  current_inferior ()->set_arch (new_gdbarch);

  return 1;
}

/* Return the architecture for ABFD.  If no suitable architecture
   could be find, return NULL.  */

struct gdbarch *
gdbarch_from_bfd (bfd *abfd)
{
  struct gdbarch_info info;

  info.abfd = abfd;
  return gdbarch_find_by_info (info);
}

/* Set the dynamic target-system-dependent parameters (architecture,
   byte-order) using information found in the BFD */

void
set_gdbarch_from_file (bfd *abfd)
{
  struct gdbarch_info info;
  struct gdbarch *gdbarch;

  info.abfd = abfd;
  info.target_desc = target_current_description ();
  gdbarch = gdbarch_find_by_info (info);

  if (gdbarch == NULL)
    error (_("Architecture of file not recognized."));

  current_inferior ()->set_arch (gdbarch);
}

/* Initialize the current architecture.  Update the ``set
   architecture'' command so that it specifies a list of valid
   architectures.  */

#ifdef DEFAULT_BFD_ARCH
extern const bfd_arch_info_type DEFAULT_BFD_ARCH;
static const bfd_arch_info_type *default_bfd_arch = &DEFAULT_BFD_ARCH;
#else
static const bfd_arch_info_type *default_bfd_arch;
#endif

#ifdef DEFAULT_BFD_VEC
extern const bfd_target DEFAULT_BFD_VEC;
static const bfd_target *default_bfd_vec = &DEFAULT_BFD_VEC;
#else
static const bfd_target *default_bfd_vec;
#endif

static enum bfd_endian default_byte_order = BFD_ENDIAN_UNKNOWN;

/* Printable names of architectures.  Used as the enum list of the
   "set arch" command.  */
static std::vector<const char *> arches;

void
initialize_current_architecture (void)
{
  arches = gdbarch_printable_names ();
  
  /* Find a default architecture.  */
  if (default_bfd_arch == NULL)
    {
      /* Choose the architecture by taking the first one
	 alphabetically.  */
      const char *chosen = arches[0];

      for (const char *arch : arches)
	{
	  if (strcmp (arch, chosen) < 0)
	    chosen = arch;
	}

      if (chosen == NULL)
	internal_error (_("initialize_current_architecture: No arch"));

      default_bfd_arch = bfd_scan_arch (chosen);
      if (default_bfd_arch == NULL)
	internal_error (_("initialize_current_architecture: Arch not found"));
    }

  gdbarch_info info;
  info.bfd_arch_info = default_bfd_arch;

  /* Take several guesses at a byte order.  */
  if (default_byte_order == BFD_ENDIAN_UNKNOWN
      && default_bfd_vec != NULL)
    {
      /* Extract BFD's default vector's byte order.  */
      switch (default_bfd_vec->byteorder)
	{
	case BFD_ENDIAN_BIG:
	  default_byte_order = BFD_ENDIAN_BIG;
	  break;
	case BFD_ENDIAN_LITTLE:
	  default_byte_order = BFD_ENDIAN_LITTLE;
	  break;
	default:
	  break;
	}
    }
  if (default_byte_order == BFD_ENDIAN_UNKNOWN)
    {
      /* look for ``*el-*'' in the target name.  */
      const char *chp;
      chp = strchr (target_name, '-');
      if (chp != NULL
	  && chp - 2 >= target_name
	  && startswith (chp - 2, "el"))
	default_byte_order = BFD_ENDIAN_LITTLE;
    }
  if (default_byte_order == BFD_ENDIAN_UNKNOWN)
    {
      /* Wire it to big-endian!!! */
      default_byte_order = BFD_ENDIAN_BIG;
    }

  info.byte_order = default_byte_order;
  info.byte_order_for_code = info.byte_order;

  if (! gdbarch_update_p (info))
    internal_error (_("initialize_current_architecture: Selection of "
		      "initial architecture failed"));

  /* Create the ``set architecture'' command appending ``auto'' to the
     list of architectures.  */
  {
    /* Append ``auto''.  */
    set_architecture_string = "auto";
    arches.push_back (set_architecture_string);
    arches.push_back (nullptr);
    set_show_commands architecture_cmds
      = add_setshow_enum_cmd ("architecture", class_support,
			      arches.data (), &set_architecture_string,
			      _("Set architecture of target."),
			      _("Show architecture of target."), NULL,
			      set_architecture, show_architecture,
			      &setlist, &showlist);
    add_alias_cmd ("processor", architecture_cmds.set, class_support, 1,
		   &setlist);
  }
}

/* Similar to init, but this time fill in the blanks.  Information is
   obtained from the global "set ..." options and explicitly
   initialized INFO fields.  */

void
gdbarch_info_fill (struct gdbarch_info *info)
{
  /* "(gdb) set architecture ...".  */
  if (info->bfd_arch_info == NULL
      && target_architecture_user)
    info->bfd_arch_info = target_architecture_user;
  /* From the file.  */
  if (info->bfd_arch_info == NULL
      && info->abfd != NULL
      && bfd_get_arch (info->abfd) != bfd_arch_unknown
      && bfd_get_arch (info->abfd) != bfd_arch_obscure)
    info->bfd_arch_info = bfd_get_arch_info (info->abfd);
  /* From the target.  */
  if (info->target_desc != NULL)
    info->bfd_arch_info = choose_architecture_for_target
			   (info->target_desc, info->bfd_arch_info);
  /* From the default.  */
  if (info->bfd_arch_info == NULL)
    info->bfd_arch_info = default_bfd_arch;

  /* "(gdb) set byte-order ...".  */
  if (info->byte_order == BFD_ENDIAN_UNKNOWN
      && target_byte_order_user != BFD_ENDIAN_UNKNOWN)
    info->byte_order = target_byte_order_user;
  /* From the INFO struct.  */
  if (info->byte_order == BFD_ENDIAN_UNKNOWN
      && info->abfd != NULL)
    info->byte_order = (bfd_big_endian (info->abfd) ? BFD_ENDIAN_BIG
			: bfd_little_endian (info->abfd) ? BFD_ENDIAN_LITTLE
			: BFD_ENDIAN_UNKNOWN);
  /* From the default.  */
  if (info->byte_order == BFD_ENDIAN_UNKNOWN)
    info->byte_order = default_byte_order;
  info->byte_order_for_code = info->byte_order;
  /* Wire the default to the last selected byte order.  */
  default_byte_order = info->byte_order;

  /* "(gdb) set osabi ...".  Handled by gdbarch_lookup_osabi.  */
  /* From the manual override, or from file.  */
  if (info->osabi == GDB_OSABI_UNKNOWN)
    info->osabi = gdbarch_lookup_osabi (info->abfd);
  /* From the target.  */

  if (info->osabi == GDB_OSABI_UNKNOWN && info->target_desc != NULL)
    info->osabi = tdesc_osabi (info->target_desc);
  /* From the configured default.  */
#ifdef GDB_OSABI_DEFAULT
  if (info->osabi == GDB_OSABI_UNKNOWN)
    info->osabi = GDB_OSABI_DEFAULT;
#endif
  /* If we still don't know which osabi to pick, pick none.  */
  if (info->osabi == GDB_OSABI_UNKNOWN)
    info->osabi = GDB_OSABI_NONE;

  /* Must have at least filled in the architecture.  */
  gdb_assert (info->bfd_arch_info != NULL);
}

/* Return "current" architecture.  If the target is running, this is
   the architecture of the selected frame.  Otherwise, the "current"
   architecture defaults to the target architecture.

   This function should normally be called solely by the command
   interpreter routines to determine the architecture to execute a
   command in.  */
struct gdbarch *
get_current_arch (void)
{
  if (has_stack_frames ())
    return get_frame_arch (get_selected_frame (NULL));
  else
    return current_inferior ()->arch ();
}

int
default_has_shared_address_space (struct gdbarch *gdbarch)
{
  /* Simply say no.  In most unix-like targets each inferior/process
     has its own address space.  */
  return 0;
}

int
default_fast_tracepoint_valid_at (struct gdbarch *gdbarch, CORE_ADDR addr,
				  std::string *msg)
{
  /* We don't know if maybe the target has some way to do fast
     tracepoints that doesn't need gdbarch, so always say yes.  */
  if (msg)
    msg->clear ();
  return 1;
}

const gdb_byte *
default_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr,
			    int *lenptr)
{
  int kind = gdbarch_breakpoint_kind_from_pc (gdbarch, pcptr);

  return gdbarch_sw_breakpoint_from_kind (gdbarch, kind, lenptr);
}
int
default_breakpoint_kind_from_current_state (struct gdbarch *gdbarch,
					    struct regcache *regcache,
					    CORE_ADDR *pcptr)
{
  return gdbarch_breakpoint_kind_from_pc (gdbarch, pcptr);
}


void
default_gen_return_address (struct gdbarch *gdbarch,
			    struct agent_expr *ax, struct axs_value *value,
			    CORE_ADDR scope)
{
  error (_("This architecture has no method to collect a return address."));
}

int
default_return_in_first_hidden_param_p (struct gdbarch *gdbarch,
					struct type *type)
{
  /* Usually, the return value's address is stored the in the "first hidden"
     parameter if the return value should be passed by reference, as
     specified in ABI.  */
  return !(language_pass_by_reference (type).trivially_copyable);
}

int default_insn_is_call (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return 0;
}

int default_insn_is_ret (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return 0;
}

int default_insn_is_jump (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return 0;
}

/*  See arch-utils.h.  */

bool
default_program_breakpoint_here_p (struct gdbarch *gdbarch,
				   CORE_ADDR address)
{
  int len;
  const gdb_byte *bpoint = gdbarch_breakpoint_from_pc (gdbarch, &address, &len);

  /* Software breakpoints unsupported?  */
  if (bpoint == nullptr)
    return false;

  gdb_byte *target_mem = (gdb_byte *) alloca (len);

  /* Enable the automatic memory restoration from breakpoints while
     we read the memory.  Otherwise we may find temporary breakpoints, ones
     inserted by GDB, and flag them as permanent breakpoints.  */
  scoped_restore restore_memory
    = make_scoped_restore_show_memory_breakpoints (0);

  if (target_read_memory (address, target_mem, len) == 0)
    {
      /* Check if this is a breakpoint instruction for this architecture,
	 including ones used by GDB.  */
      if (memcmp (target_mem, bpoint, len) == 0)
	return true;
    }

  return false;
}

void
default_skip_permanent_breakpoint (struct regcache *regcache)
{
  struct gdbarch *gdbarch = regcache->arch ();
  CORE_ADDR current_pc = regcache_read_pc (regcache);
  int bp_len;

  gdbarch_breakpoint_from_pc (gdbarch, &current_pc, &bp_len);
  current_pc += bp_len;
  regcache_write_pc (regcache, current_pc);
}

CORE_ADDR
default_infcall_mmap (CORE_ADDR size, unsigned prot)
{
  error (_("This target does not support inferior memory allocation by mmap."));
}

void
default_infcall_munmap (CORE_ADDR addr, CORE_ADDR size)
{
  /* Memory reserved by inferior mmap is kept leaked.  */
}

/* -mcmodel=large is used so that no GOT (Global Offset Table) is needed to be
   created in inferior memory by GDB (normally it is set by ld.so).  */

std::string
default_gcc_target_options (struct gdbarch *gdbarch)
{
  return string_printf ("-m%d%s", gdbarch_ptr_bit (gdbarch),
			(gdbarch_ptr_bit (gdbarch) == 64
			 ? " -mcmodel=large" : ""));
}

/* gdbarch gnu_triplet_regexp method.  */

const char *
default_gnu_triplet_regexp (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->arch_name;
}

/* Default method for gdbarch_addressable_memory_unit_size.  The default is
   based on the bits_per_byte defined in the bfd library for the current
   architecture, this is usually 8-bits, and so this function will usually
   return 1 indicating 1 byte is 1 octet.  */

int
default_addressable_memory_unit_size (struct gdbarch *gdbarch)
{
  return gdbarch_bfd_arch_info (gdbarch)->bits_per_byte / 8;
}

void
default_guess_tracepoint_registers (struct gdbarch *gdbarch,
				    struct regcache *regcache,
				    CORE_ADDR addr)
{
  int pc_regno = gdbarch_pc_regnum (gdbarch);
  gdb_byte *regs;

  /* This guessing code below only works if the PC register isn't
     a pseudo-register.  The value of a pseudo-register isn't stored
     in the (non-readonly) regcache -- instead it's recomputed
     (probably from some other cached raw register) whenever the
     register is read.  In this case, a custom method implementation
     should be used by the architecture.  */
  if (pc_regno < 0 || pc_regno >= gdbarch_num_regs (gdbarch))
    return;

  regs = (gdb_byte *) alloca (register_size (gdbarch, pc_regno));
  store_unsigned_integer (regs, register_size (gdbarch, pc_regno),
			  gdbarch_byte_order (gdbarch), addr);
  regcache->raw_supply (pc_regno, regs);
}

int
default_print_insn (bfd_vma memaddr, disassemble_info *info)
{
  disassembler_ftype disassemble_fn;

  disassemble_fn = disassembler (info->arch, info->endian == BFD_ENDIAN_BIG,
				 info->mach, current_program_space->exec_bfd ());

  gdb_assert (disassemble_fn != NULL);
  return (*disassemble_fn) (memaddr, info);
}

/* See arch-utils.h.  */

CORE_ADDR
gdbarch_skip_prologue_noexcept (gdbarch *gdbarch, CORE_ADDR pc) noexcept
{
  CORE_ADDR new_pc = pc;

  try
    {
      new_pc = gdbarch_skip_prologue (gdbarch, pc);
    }
  catch (const gdb_exception &ex)
    {}

  return new_pc;
}

/* See arch-utils.h.  */

bool
default_in_indirect_branch_thunk (gdbarch *gdbarch, CORE_ADDR pc)
{
  return false;
}

/* See arch-utils.h.  */

ULONGEST
default_type_align (struct gdbarch *gdbarch, struct type *type)
{
  return 0;
}

/* See arch-utils.h.  */

std::string
default_get_pc_address_flags (frame_info_ptr frame, CORE_ADDR pc)
{
  return "";
}

/* See arch-utils.h.  */
void
default_read_core_file_mappings
  (struct gdbarch *gdbarch,
   struct bfd *cbfd,
   read_core_file_mappings_pre_loop_ftype pre_loop_cb,
   read_core_file_mappings_loop_ftype loop_cb)
{
}

/* See arch-utils.h.  */
bool
default_use_target_description_from_corefile_notes (struct gdbarch *gdbarch,
						    struct bfd *corefile_bfd)
{
  /* Always trust the corefile target description contained in the target
     description note.  */
  return true;
}

CORE_ADDR
default_get_return_buf_addr (struct type *val_type, frame_info_ptr cur_frame)
{
  return 0;
}

bool
default_dwarf2_omit_typedef_p (struct type *target_type, const char *producer,
			       const char *name)
{
  return false;
}

static CORE_ADDR
default_update_call_site_pc (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  return pc;
}

/* Non-zero if we want to trace architecture code.  */

#ifndef GDBARCH_DEBUG
#define GDBARCH_DEBUG 0
#endif
unsigned int gdbarch_debug = GDBARCH_DEBUG;
static void
show_gdbarch_debug (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("Architecture debugging is %s.\n"), value);
}

static const char *
pformat (struct gdbarch *gdbarch, const struct floatformat **format)
{
  if (format == NULL)
    return "(null)";

  int format_index = gdbarch_byte_order (gdbarch) == BFD_ENDIAN_LITTLE ? 1 : 0;
  return format[format_index]->name;
}

static const char *
pstring (const char *string)
{
  if (string == NULL)
    return "(null)";
  return string;
}

static const char *
pstring_ptr (char **string)
{
  if (string == NULL || *string == NULL)
    return "(null)";
  return *string;
}

/* Helper function to print a list of strings, represented as "const
   char *const *".  The list is printed comma-separated.  */

static const char *
pstring_list (const char *const *list)
{
  static char ret[100];
  const char *const *p;
  size_t offset = 0;

  if (list == NULL)
    return "(null)";

  ret[0] = '\0';
  for (p = list; *p != NULL && offset < sizeof (ret); ++p)
    {
      size_t s = xsnprintf (ret + offset, sizeof (ret) - offset, "%s, ", *p);
      offset += 2 + s;
    }

  if (offset > 0)
    {
      gdb_assert (offset - 2 < sizeof (ret));
      ret[offset - 2] = '\0';
    }

  return ret;
}

#include "gdbarch.c"

enum return_value_convention
default_gdbarch_return_value
     (struct gdbarch *gdbarch, struct value *function, struct type *valtype,
      struct regcache *regcache, struct value **read_value,
      const gdb_byte *writebuf)
{
  gdb_byte *readbuf = nullptr;

  if (read_value != nullptr)
    {
      *read_value = value::allocate (valtype);
      readbuf = (*read_value)->contents_raw ().data ();
    }

  return gdbarch->return_value (gdbarch, function, valtype, regcache,
				readbuf, writebuf);
}

obstack *gdbarch_obstack (gdbarch *arch)
{
  return &arch->obstack;
}

/* See gdbarch.h.  */

char *
gdbarch_obstack_strdup (struct gdbarch *arch, const char *string)
{
  return obstack_strdup (&arch->obstack, string);
}

/* Free a gdbarch struct.  This should never happen in normal
   operation --- once you've created a gdbarch, you keep it around.
   However, if an architecture's init function encounters an error
   building the structure, it may need to clean up a partially
   constructed gdbarch.  */

void
gdbarch_free (struct gdbarch *arch)
{
  gdb_assert (arch != NULL);
  gdb_assert (!arch->initialized_p);
  delete arch;
}

/* See gdbarch.h.  */

struct gdbarch_tdep_base *
gdbarch_tdep_1 (struct gdbarch *gdbarch)
{
  if (gdbarch_debug >= 2)
    gdb_printf (gdb_stdlog, "gdbarch_tdep_1 called\n");
  return gdbarch->tdep.get ();
}

registry<gdbarch> *
registry_accessor<gdbarch>::get (gdbarch *arch)
{
  return &arch->registry_fields;
}

/* Keep a registry of the architectures known by GDB.  */

struct gdbarch_registration
{
  enum bfd_architecture bfd_architecture;
  gdbarch_init_ftype *init;
  gdbarch_dump_tdep_ftype *dump_tdep;
  gdbarch_supports_arch_info_ftype *supports_arch_info;
  struct gdbarch_list *arches;
  struct gdbarch_registration *next;
};

static struct gdbarch_registration *gdbarch_registry = NULL;

std::vector<const char *>
gdbarch_printable_names ()
{
  /* Accumulate a list of names based on the registered list of
     architectures.  */
  std::vector<const char *> arches;

  for (gdbarch_registration *rego = gdbarch_registry;
       rego != nullptr;
       rego = rego->next)
    {
      const struct bfd_arch_info *ap
	= bfd_lookup_arch (rego->bfd_architecture, 0);
      if (ap == nullptr)
	internal_error (_("gdbarch_architecture_names: multi-arch unknown"));
      do
	{
	  if (rego->supports_arch_info == nullptr
	      || rego->supports_arch_info (ap))
	    arches.push_back (ap->printable_name);
	  ap = ap->next;
	}
      while (ap != NULL);
    }

  return arches;
}


void
gdbarch_register (enum bfd_architecture bfd_architecture,
		  gdbarch_init_ftype *init,
		  gdbarch_dump_tdep_ftype *dump_tdep,
		  gdbarch_supports_arch_info_ftype *supports_arch_info)
{
  struct gdbarch_registration **curr;
  const struct bfd_arch_info *bfd_arch_info;

  /* Check that BFD recognizes this architecture */
  bfd_arch_info = bfd_lookup_arch (bfd_architecture, 0);
  if (bfd_arch_info == NULL)
    {
      internal_error (_("gdbarch: Attempt to register "
			"unknown architecture (%d)"),
		      bfd_architecture);
    }
  /* Check that we haven't seen this architecture before.  */
  for (curr = &gdbarch_registry;
       (*curr) != NULL;
       curr = &(*curr)->next)
    {
      if (bfd_architecture == (*curr)->bfd_architecture)
	internal_error (_("gdbarch: Duplicate registration "
			  "of architecture (%s)"),
			bfd_arch_info->printable_name);
    }
  /* log it */
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog, "gdbarch_register (%s, %s)\n",
		bfd_arch_info->printable_name,
		host_address_to_string (init));
  /* Append it */
  (*curr) = XNEW (struct gdbarch_registration);
  (*curr)->bfd_architecture = bfd_architecture;
  (*curr)->init = init;
  (*curr)->dump_tdep = dump_tdep;
  (*curr)->supports_arch_info = supports_arch_info;
  (*curr)->arches = NULL;
  (*curr)->next = NULL;
}

/* Look for an architecture using gdbarch_info.  */

struct gdbarch_list *
gdbarch_list_lookup_by_info (struct gdbarch_list *arches,
			     const struct gdbarch_info *info)
{
  for (; arches != NULL; arches = arches->next)
    {
      if (info->bfd_arch_info != arches->gdbarch->bfd_arch_info)
	continue;
      if (info->byte_order != arches->gdbarch->byte_order)
	continue;
      if (info->osabi != arches->gdbarch->osabi)
	continue;
      if (info->target_desc != arches->gdbarch->target_desc)
	continue;
      return arches;
    }
  return NULL;
}


/* Find an architecture that matches the specified INFO.  Create a new
   architecture if needed.  Return that new architecture.  */

struct gdbarch *
gdbarch_find_by_info (struct gdbarch_info info)
{
  struct gdbarch *new_gdbarch;
  struct gdbarch_registration *rego;

  /* Fill in missing parts of the INFO struct using a number of
     sources: "set ..."; INFOabfd supplied; and the global
     defaults.  */
  gdbarch_info_fill (&info);

  /* Must have found some sort of architecture.  */
  gdb_assert (info.bfd_arch_info != nullptr);

  if (gdbarch_debug)
    {
      gdb_printf (gdb_stdlog,
		  "gdbarch_find_by_info: info.bfd_arch_info %s\n",
		  (info.bfd_arch_info != nullptr
		   ? info.bfd_arch_info->printable_name
		   : "(null)"));
      gdb_printf (gdb_stdlog,
		  "gdbarch_find_by_info: info.byte_order %d (%s)\n",
		  info.byte_order,
		  (info.byte_order == BFD_ENDIAN_BIG ? "big"
		   : info.byte_order == BFD_ENDIAN_LITTLE ? "little"
		   : "default"));
      gdb_printf (gdb_stdlog,
		  "gdbarch_find_by_info: info.osabi %d (%s)\n",
		  info.osabi, gdbarch_osabi_name (info.osabi));
      gdb_printf (gdb_stdlog,
		  "gdbarch_find_by_info: info.abfd %s\n",
		  host_address_to_string (info.abfd));
    }

  /* Find the tdep code that knows about this architecture.  */
  for (rego = gdbarch_registry;
       rego != nullptr;
       rego = rego->next)
    if (rego->bfd_architecture == info.bfd_arch_info->arch)
      break;
  if (rego == nullptr)
    {
      if (gdbarch_debug)
	gdb_printf (gdb_stdlog, "gdbarch_find_by_info: "
		    "No matching architecture\n");
      return nullptr;
    }

  /* Ask the tdep code for an architecture that matches "info".  */
  new_gdbarch = rego->init (info, rego->arches);

  /* Did the tdep code like it?  No.  Reject the change and revert to
     the old architecture.  */
  if (new_gdbarch == nullptr)
    {
      if (gdbarch_debug)
	gdb_printf (gdb_stdlog, "gdbarch_find_by_info: "
		    "Target rejected architecture\n");
      return nullptr;
    }

  /* Is this a pre-existing architecture (as determined by already
     being initialized)?  Move it to the front of the architecture
     list (keeping the list sorted Most Recently Used).  */
  if (new_gdbarch->initialized_p)
    {
      struct gdbarch_list **list;
      struct gdbarch_list *self;
      if (gdbarch_debug)
	gdb_printf (gdb_stdlog, "gdbarch_find_by_info: "
		    "Previous architecture %s (%s) selected\n",
		    host_address_to_string (new_gdbarch),
		    new_gdbarch->bfd_arch_info->printable_name);
      /* Find the existing arch in the list.  */
      for (list = &rego->arches;
	   (*list) != nullptr && (*list)->gdbarch != new_gdbarch;
	   list = &(*list)->next);
      /* It had better be in the list of architectures.  */
      gdb_assert ((*list) != nullptr && (*list)->gdbarch == new_gdbarch);
      /* Unlink SELF.  */
      self = (*list);
      (*list) = self->next;
      /* Insert SELF at the front.  */
      self->next = rego->arches;
      rego->arches = self;
      /* Return it.  */
      return new_gdbarch;
    }

  /* It's a new architecture.  */
  if (gdbarch_debug)
    gdb_printf (gdb_stdlog, "gdbarch_find_by_info: "
		"New architecture %s (%s) selected\n",
		host_address_to_string (new_gdbarch),
		new_gdbarch->bfd_arch_info->printable_name);

  /* Insert the new architecture into the front of the architecture
     list (keep the list sorted Most Recently Used).  */
  {
    struct gdbarch_list *self = XNEW (struct gdbarch_list);
    self->next = rego->arches;
    self->gdbarch = new_gdbarch;
    rego->arches = self;
  }

  /* Check that the newly installed architecture is valid.  Plug in
     any post init values.  */
  new_gdbarch->dump_tdep = rego->dump_tdep;
  verify_gdbarch (new_gdbarch);
  new_gdbarch->initialized_p = true;

  if (gdbarch_debug)
    gdbarch_dump (new_gdbarch, gdb_stdlog);

  gdb::observers::new_architecture.notify (new_gdbarch);

  return new_gdbarch;
}

/* See gdbarch.h.  */

bool
gdbarch_initialized_p (gdbarch *arch)
{
  return arch->initialized_p;
}

void _initialize_gdbarch_utils ();
void
_initialize_gdbarch_utils ()
{
  add_setshow_enum_cmd ("endian", class_support,
			endian_enum, &set_endian_string, 
			_("Set endianness of target."),
			_("Show endianness of target."),
			NULL, set_endian, show_endian,
			&setlist, &showlist);
  add_setshow_zuinteger_cmd ("arch", class_maintenance, &gdbarch_debug, _("\
Set architecture debugging."), _("\
Show architecture debugging."), _("\
When non-zero, architecture debugging is enabled."),
			    NULL,
			    show_gdbarch_debug,
			    &setdebuglist, &showdebuglist);
}
