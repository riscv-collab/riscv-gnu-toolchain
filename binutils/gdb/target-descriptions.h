/* Target description support for GDB.

   Copyright (C) 2006-2024 Free Software Foundation, Inc.

   Contributed by CodeSourcery.

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

#ifndef TARGET_DESCRIPTIONS_H
#define TARGET_DESCRIPTIONS_H 1
#include "gdbsupport/tdesc.h"
#include "gdbarch.h"

struct tdesc_arch_data;
struct target_ops;
struct inferior;

/* Fetch the current inferior's description, and switch its current
   architecture to one which incorporates that description.  */

void target_find_description (void);

/* Discard any description fetched from the target for the current
   inferior, and switch the current architecture to one with no target
   description.  */

void target_clear_description (void);

/* Return the current inferior's target description.  This should only
   be used by gdbarch initialization code; most access should be
   through an existing gdbarch.  */

const struct target_desc *target_current_description (void);

/* Record architecture-specific functions to call for pseudo-register
   support.  If tdesc_use_registers is called and gdbarch_num_pseudo_regs
   is greater than zero, then these should be called as well.
   They are equivalent to the gdbarch methods with similar names,
   except that they will only be called for pseudo registers.  */

void set_tdesc_pseudo_register_name
  (struct gdbarch *gdbarch, gdbarch_register_name_ftype *pseudo_name);

void set_tdesc_pseudo_register_type
  (struct gdbarch *gdbarch, gdbarch_register_type_ftype *pseudo_type);

void set_tdesc_pseudo_register_reggroup_p
  (struct gdbarch *gdbarch,
   gdbarch_register_reggroup_p_ftype *pseudo_reggroup_p);

/* Pointer to a function that should be called for each unknown register in
   a target description, used by TDESC_USE_REGISTERS.

   GDBARCH is the architecture the target description is for, FEATURE is
   the feature the unknown register is in, and REG_NAME is the name of the
   register from the target description.  The POSSIBLE_REGNUM is a proposed
   (GDB internal) number for this register.

   The callback function can return, (-1) to indicate that the register
   should not be assigned POSSIBLE_REGNUM now (though it might be later),
   GDB will number the register automatically later on.  Return
   POSSIBLE_REGNUM (or greater) to have this register assigned that number.
   Returning a value less that POSSIBLE_REGNUM is also acceptable, but take
   care not to clash with a register number that has already been
   assigned.

   The callback will always be called on the registers in the order they
   appear in the target description.  This means all unknown registers
   within a single feature will be called one after another.  */

typedef int (*tdesc_unknown_register_ftype)
	(struct gdbarch *gdbarch, tdesc_feature *feature,
	 const char *reg_name, int possible_regnum);

/* A deleter adapter for a target arch data.  */

struct tdesc_arch_data_deleter
{
  void operator() (struct tdesc_arch_data *data) const;
};

/* A unique pointer specialization that holds a target_desc.  */

typedef std::unique_ptr<tdesc_arch_data, tdesc_arch_data_deleter>
  tdesc_arch_data_up;

/* Update GDBARCH to use the TARGET_DESC for registers.  TARGET_DESC
   may be GDBARCH's target description or (if GDBARCH does not have
   one which describes registers) another target description
   constructed by the gdbarch initialization routine.

   Fixed register assignments are taken from EARLY_DATA, which is freed.
   All registers which have not been assigned fixed numbers are given
   numbers above the current value of gdbarch_num_regs.
   gdbarch_num_regs and various  register-related predicates are updated to
   refer to the target description.  This function should only be called from
   the architecture's gdbarch initialization routine, and only after
   successfully validating the required registers.  */

void tdesc_use_registers (struct gdbarch *gdbarch,
			  const struct target_desc *target_desc,
			  tdesc_arch_data_up &&early_data,
			  tdesc_unknown_register_ftype unk_reg_cb = NULL);

/* Allocate initial data for validation of a target description during
   gdbarch initialization.  */

tdesc_arch_data_up tdesc_data_alloc ();

/* Search FEATURE for a register named NAME.  Record REGNO and the
   register in DATA; when tdesc_use_registers is called, REGNO will be
   assigned to the register.  1 is returned if the register was found,
   0 if it was not.  */

int tdesc_numbered_register (const struct tdesc_feature *feature,
			     struct tdesc_arch_data *data,
			     int regno, const char *name);

/* Search FEATURE for a register named NAME, but do not assign a fixed
   register number to it.  */

int tdesc_unnumbered_register (const struct tdesc_feature *feature,
			       const char *name);

/* Search FEATURE for a register named NAME, and return its size in
   bits.  The register must exist.  */

int tdesc_register_bitsize (const struct tdesc_feature *feature,
			    const char *name);

/* Search FEATURE for a register with any of the names from NAMES
   (NULL-terminated).  Record REGNO and the register in DATA; when
   tdesc_use_registers is called, REGNO will be assigned to the
   register.  1 is returned if the register was found, 0 if it was
   not.  */

int tdesc_numbered_register_choices (const struct tdesc_feature *feature,
				     struct tdesc_arch_data *data,
				     int regno, const char *const names[]);

/* Return true if DATA contains an entry for REGNO, a GDB register
   number.  */

extern bool tdesc_found_register (struct tdesc_arch_data *data, int regno);

/* Accessors for target descriptions.  */

/* Return the BFD architecture associated with this target
   description, or NULL if no architecture was specified.  */

const struct bfd_arch_info *tdesc_architecture
  (const struct target_desc *);

/* Return the OSABI associated with this target description, or
   GDB_OSABI_UNKNOWN if no osabi was specified.  */

enum gdb_osabi tdesc_osabi (const struct target_desc *);

/* Return non-zero if this target description is compatible
   with the given BFD architecture.  */

int tdesc_compatible_p (const struct target_desc *,
			const struct bfd_arch_info *);

/* Return the string value of a property named KEY, or NULL if the
   property was not specified.  */

const char *tdesc_property (const struct target_desc *,
			    const char *key);

/* Return 1 if this target description describes any registers.  */

int tdesc_has_registers (const struct target_desc *);

/* Return the feature with the given name, if present, or NULL if
   the named feature is not found.  */

const struct tdesc_feature *tdesc_find_feature (const struct target_desc *,
						const char *name);

/* Return the name of FEATURE.  */

const char *tdesc_feature_name (const struct tdesc_feature *feature);

/* Return the name of register REGNO, from the target description or
   from an architecture-provided pseudo_register_name method.  */

const char *tdesc_register_name (struct gdbarch *gdbarch, int regno);

/* Return the type of register REGNO, from the target description or
   from an architecture-provided pseudo_register_type method.  */

struct type *tdesc_register_type (struct gdbarch *gdbarch, int regno);

/* Return the type associated with ID, from the target description.  */

struct type *tdesc_find_type (struct gdbarch *gdbarch, const char *id);

/* Check whether REGNUM is a member of REGGROUP using the target
   description.  Return -1 if the target description does not
   specify a group.  */

int tdesc_register_in_reggroup_p (struct gdbarch *gdbarch, int regno,
				  const struct reggroup *reggroup);

/* Methods for constructing a target description.  */

void set_tdesc_architecture (struct target_desc *,
			     const struct bfd_arch_info *);
void set_tdesc_osabi (struct target_desc *, enum gdb_osabi osabi);
void set_tdesc_property (struct target_desc *,
			 const char *key, const char *value);
void tdesc_add_compatible (struct target_desc *,
			   const struct bfd_arch_info *);

#if GDB_SELF_TEST
namespace selftests {

/* Record that XML_FILE should generate a target description that equals
   TDESC, to be verified by the "maintenance check xml-descriptions"
   command.  This function takes ownership of TDESC.  */

void record_xml_tdesc (const char *xml_file,
		       const struct target_desc *tdesc);
}
#endif

#endif /* TARGET_DESCRIPTIONS_H */
