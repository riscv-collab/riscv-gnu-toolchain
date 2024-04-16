/* Copyright (C) 2006-2024 Free Software Foundation, Inc.

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

#ifndef COMMON_TDESC_H
#define COMMON_TDESC_H

struct tdesc_feature;
struct tdesc_type;
struct tdesc_type_builtin;
struct tdesc_type_vector;
struct tdesc_type_with_fields;
struct tdesc_reg;
struct target_desc;

/* The interface to visit different elements of target description.  */

class tdesc_element_visitor
{
public:
  virtual void visit_pre (const target_desc *e)
  {}

  virtual void visit_post (const target_desc *e)
  {}

  virtual void visit_pre (const tdesc_feature *e)
  {}

  virtual void visit_post (const tdesc_feature *e)
  {}

  virtual void visit (const tdesc_type_builtin *e)
  {}

  virtual void visit (const tdesc_type_vector *e)
  {}

  virtual void visit (const tdesc_type_with_fields *e)
  {}

  virtual void visit (const tdesc_reg *e)
  {}
};

class tdesc_element
{
public:
  virtual void accept (tdesc_element_visitor &v) const = 0;
};

/* An individual register from a target description.  */

struct tdesc_reg : tdesc_element
{
  tdesc_reg (struct tdesc_feature *feature, const std::string &name_,
	     int regnum, int save_restore_, const char *group_,
	     int bitsize_, const char *type_);

  virtual ~tdesc_reg () = default;

  DISABLE_COPY_AND_ASSIGN (tdesc_reg);

  /* The name of this register.  In standard features, it may be
     recognized by the architecture support code, or it may be purely
     for the user.  */
  std::string name;

  /* The register number used by this target to refer to this
     register.  This is used for remote p/P packets and to determine
     the ordering of registers in the remote g/G packets.  */
  long target_regnum;

  /* If this flag is set, GDB should save and restore this register
     around calls to an inferior function.  */
  int save_restore;

  /* The name of the register group containing this register, or empty
     if the group should be automatically determined from the
     register's type.  If this is "general", "float", or "vector", the
     corresponding "info" command should display this register's
     value.  It can be an arbitrary string, but should be limited to
     alphanumeric characters and internal hyphens.  Currently other
     strings are ignored (treated as empty).  */
  std::string group;

  /* The size of the register, in bits.  */
  int bitsize;

  /* The type of the register.  This string corresponds to either
     a named type from the target description or a predefined
     type from GDB.  */
  std::string type;

  /* The target-described type corresponding to TYPE, if found.  */
  struct tdesc_type *tdesc_type;

  void accept (tdesc_element_visitor &v) const override
  {
    v.visit (this);
  }

  bool operator== (const tdesc_reg &other) const
  {
    return (name == other.name
       && target_regnum == other.target_regnum
       && save_restore == other.save_restore
       && bitsize == other.bitsize
       && group == other.group
       && type == other.type);
  }

  bool operator!= (const tdesc_reg &other) const
  {
    return !(*this == other);
  }
};

typedef std::unique_ptr<tdesc_reg> tdesc_reg_up;

/* Declaration of a structure that holds information about one
   "compatibility" entry within a target description.  */

struct tdesc_compatible_info;

/* A pointer to a single piece of compatibility information.  */

typedef std::unique_ptr<tdesc_compatible_info> tdesc_compatible_info_up;

/* Return a vector of compatibility information pointers from the target
   description TARGET_DESC.  */

const std::vector<tdesc_compatible_info_up> &tdesc_compatible_info_list
	(const target_desc *target_desc);

/* Return the architecture name from a compatibility information
   COMPATIBLE.  */

const char *tdesc_compatible_info_arch_name
	(const tdesc_compatible_info_up &compatible);

enum tdesc_type_kind
{
  /* Predefined types.  */
  TDESC_TYPE_BOOL,
  TDESC_TYPE_INT8,
  TDESC_TYPE_INT16,
  TDESC_TYPE_INT32,
  TDESC_TYPE_INT64,
  TDESC_TYPE_INT128,
  TDESC_TYPE_UINT8,
  TDESC_TYPE_UINT16,
  TDESC_TYPE_UINT32,
  TDESC_TYPE_UINT64,
  TDESC_TYPE_UINT128,
  TDESC_TYPE_CODE_PTR,
  TDESC_TYPE_DATA_PTR,
  TDESC_TYPE_IEEE_HALF,
  TDESC_TYPE_IEEE_SINGLE,
  TDESC_TYPE_IEEE_DOUBLE,
  TDESC_TYPE_ARM_FPA_EXT,
  TDESC_TYPE_I387_EXT,
  TDESC_TYPE_BFLOAT16,

  /* Types defined by a target feature.  */
  TDESC_TYPE_VECTOR,
  TDESC_TYPE_STRUCT,
  TDESC_TYPE_UNION,
  TDESC_TYPE_FLAGS,
  TDESC_TYPE_ENUM
};

struct tdesc_type : tdesc_element
{
  tdesc_type (const std::string &name_, enum tdesc_type_kind kind_)
    : name (name_), kind (kind_)
  {}

  virtual ~tdesc_type () = default;

  DISABLE_COPY_AND_ASSIGN (tdesc_type);

  /* The name of this type.  */
  std::string name;

  /* Identify the kind of this type.  */
  enum tdesc_type_kind kind;

  bool operator== (const tdesc_type &other) const
  {
    return name == other.name && kind == other.kind;
  }

  bool operator!= (const tdesc_type &other) const
  {
    return !(*this == other);
  }
};

typedef std::unique_ptr<tdesc_type> tdesc_type_up;

struct tdesc_type_builtin : tdesc_type
{
  tdesc_type_builtin (const std::string &name, enum tdesc_type_kind kind)
  : tdesc_type (name, kind)
  {}

  void accept (tdesc_element_visitor &v) const override
  {
    v.visit (this);
  }
};

/* tdesc_type for vector types.  */

struct tdesc_type_vector : tdesc_type
{
  tdesc_type_vector (const std::string &name, tdesc_type *element_type_,
		     int count_)
  : tdesc_type (name, TDESC_TYPE_VECTOR),
    element_type (element_type_), count (count_)
  {}

  void accept (tdesc_element_visitor &v) const override
  {
    v.visit (this);
  }

  struct tdesc_type *element_type;
  int count;
};

/* A named type from a target description.  */

struct tdesc_type_field
{
  tdesc_type_field (const std::string &name_, tdesc_type *type_,
		    int start_, int end_)
  : name (name_), type (type_), start (start_), end (end_)
  {}

  std::string name;
  struct tdesc_type *type;
  /* For non-enum-values, either both are -1 (non-bitfield), or both are
     not -1 (bitfield).  For enum values, start is the value (which could be
     -1), end is -1.  */
  int start, end;
};

/* tdesc_type for struct, union, flags, and enum types.  */

struct tdesc_type_with_fields : tdesc_type
{
  tdesc_type_with_fields (const std::string &name, tdesc_type_kind kind,
			  int size_ = 0)
  : tdesc_type (name, kind), size (size_)
  {}

  void accept (tdesc_element_visitor &v) const override
  {
    v.visit (this);
  }

  std::vector<tdesc_type_field> fields;
  int size;
};

/* A feature from a target description.  Each feature is a collection
   of other elements, e.g. registers and types.  */

struct tdesc_feature : tdesc_element
{
  tdesc_feature (const std::string &name_)
    : name (name_)
  {}

  virtual ~tdesc_feature () = default;

  DISABLE_COPY_AND_ASSIGN (tdesc_feature);

  /* The name of this feature.  It may be recognized by the architecture
     support code.  */
  std::string name;

  /* The registers associated with this feature.  */
  std::vector<tdesc_reg_up> registers;

  /* The types associated with this feature.  */
  std::vector<tdesc_type_up> types;

  void accept (tdesc_element_visitor &v) const override;

  bool operator== (const tdesc_feature &other) const;

  bool operator!= (const tdesc_feature &other) const
  {
    return !(*this == other);
  }
};

typedef std::unique_ptr<tdesc_feature> tdesc_feature_up;

/* A deleter adapter for a target_desc.  There are different
   implementations of this deleter class in gdb and gdbserver because even
   though the target_desc name is shared between the two projects, the
   actual implementations of target_desc are completely different.  */

struct target_desc_deleter
{
  void operator() (struct target_desc *desc) const;
};

/* A unique pointer specialization that holds a target_desc.  */

typedef std::unique_ptr<target_desc, target_desc_deleter> target_desc_up;

/* Allocate a new target_desc.  */
target_desc_up allocate_target_description (void);

/* Set TARGET_DESC's architecture by NAME.  */
void set_tdesc_architecture (target_desc *target_desc,
			     const char *name);

/* Return the architecture associated with this target description as a string,
   or NULL if no architecture was specified.  */
const char *tdesc_architecture_name (const struct target_desc *target_desc);

/* Set TARGET_DESC's osabi by NAME.  */
void set_tdesc_osabi (target_desc *target_desc, const char *name);

/* Return the osabi associated with this target description as a string,
   or NULL if no osabi was specified.  */
const char *tdesc_osabi_name (const struct target_desc *target_desc);

/* Return the type associated with ID in the context of FEATURE, or
   NULL if none.  */
struct tdesc_type *tdesc_named_type (const struct tdesc_feature *feature,
				     const char *id);

/* Return the created feature named NAME in target description TDESC.  */
struct tdesc_feature *tdesc_create_feature (struct target_desc *tdesc,
					    const char *name);

/* Return the created vector tdesc_type named NAME in FEATURE.  */
struct tdesc_type *tdesc_create_vector (struct tdesc_feature *feature,
					const char *name,
					struct tdesc_type *field_type,
					int count);

/* Return the created struct tdesc_type named NAME in FEATURE.  */
tdesc_type_with_fields *tdesc_create_struct (struct tdesc_feature *feature,
					     const char *name);

/* Return the created union tdesc_type named NAME in FEATURE.  */
tdesc_type_with_fields *tdesc_create_union (struct tdesc_feature *feature,
					    const char *name);

/* Return the created flags tdesc_type named NAME in FEATURE.  */
tdesc_type_with_fields *tdesc_create_flags (struct tdesc_feature *feature,
					    const char *name,
					    int size);

/* Return the created enum tdesc_type named NAME in FEATURE.  */
tdesc_type_with_fields *tdesc_create_enum (struct tdesc_feature *feature,
					   const char *name,
					   int size);

/* Add a new field to TYPE.  FIELD_NAME is its name, and FIELD_TYPE is
   its type.  */
void tdesc_add_field (tdesc_type_with_fields *type, const char *field_name,
		      struct tdesc_type *field_type);

/* Add a new bitfield to TYPE, with range START to END.  FIELD_NAME is its name,
   and FIELD_TYPE is its type.  */
void tdesc_add_typed_bitfield (tdesc_type_with_fields *type,
			       const char *field_name,
			       int start, int end,
			       struct tdesc_type *field_type);

/* Set the total length of TYPE.  Structs which contain bitfields may
   omit the reserved bits, so the end of the last field may not
   suffice.  */
void tdesc_set_struct_size (tdesc_type_with_fields *type, int size);

/* Add a new untyped bitfield to TYPE.
   Untyped bitfields become either uint32 or uint64 depending on the size
   of the underlying type.  */
void tdesc_add_bitfield (tdesc_type_with_fields *type, const char *field_name,
			 int start, int end);

/* A flag is just a typed(bool) single-bit bitfield.
   This function is kept to minimize changes in generated files.  */
void tdesc_add_flag (tdesc_type_with_fields *type, int start,
		     const char *flag_name);

/* Add field with VALUE and NAME to the enum TYPE.  */
void tdesc_add_enum_value (tdesc_type_with_fields *type, int value,
			   const char *name);

/* Create a register in feature FEATURE.  */
void tdesc_create_reg (struct tdesc_feature *feature, const char *name,
		       int regnum, int save_restore, const char *group,
		       int bitsize, const char *type);

/* Return the tdesc in string XML format.  */

const char *tdesc_get_features_xml (const target_desc *tdesc);

/* Print target description as xml.  */

class print_xml_feature : public tdesc_element_visitor
{
public:
  print_xml_feature (std::string *buffer_)
    : m_buffer (buffer_),
      m_depth (0)
  {}

  void visit_pre (const target_desc *e) override;
  void visit_post (const target_desc *e) override;
  void visit_pre (const tdesc_feature *e) override;
  void visit_post (const tdesc_feature *e) override;
  void visit (const tdesc_type_builtin *type) override;
  void visit (const tdesc_type_vector *type) override;
  void visit (const tdesc_type_with_fields *type) override;
  void visit (const tdesc_reg *reg) override;

private:

  /* Called with a positive value of ADJUST when we move inside an element,
     for example inside <target>, and with a negative value when we leave
     the element.  In this class this function does nothing, but a
     sub-class can override this to track the current level of nesting.  */
  void indent (int adjust)
  {
    m_depth += (adjust * 2);
  }

  /* Functions to add lines to the output buffer M_BUFFER.  Each of these
     functions appends a newline, so don't include one in the strings being
     passed.  */
  void add_line (const std::string &str);
  void add_line (const char *fmt, ...) ATTRIBUTE_PRINTF (2, 3);

  /* The buffer we are writing too.  */
  std::string *m_buffer;

  /* The current indentation depth.  */
  int m_depth;
};

#endif /* COMMON_TDESC_H */
