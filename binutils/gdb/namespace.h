/* Code dealing with "using" directives for GDB.
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

#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "gdbsupport/gdb_vecs.h"
#include "gdbsupport/gdb_obstack.h"

/* This struct is designed to store data from using directives.  It
   says that names from namespace IMPORT_SRC should be visible within
   namespace IMPORT_DEST.  These form a linked list; NEXT is the next
   element of the list.  If the imported namespace or declaration has
   been aliased within the IMPORT_DEST namespace, ALIAS is set to a
   string representing the alias.  Otherwise, ALIAS is NULL.
   DECLARATION is the name of the imported declaration, if this import
   statement represents one.  Otherwise DECLARATION is NULL and this
   import statement represents a namespace.  DECL_LINE is the line
   where the using directive is written in the source code.

   C++:      using namespace A;
   Fortran:  use A
   import_src = "A"
   import_dest = local scope of the import statement even such as ""
   alias = NULL
   declaration = NULL
   excludes = NULL

   C++:      using A::x;
   Fortran:  use A, only: x
   import_src = "A"
   import_dest = local scope of the import statement even such as ""
   alias = NULL
   declaration = "x"
   excludes = NULL
   The declaration will get imported as import_dest::x.

   C++ has no way to import all names except those listed ones.
   Fortran:  use A, localname => x
   import_src = "A"
   import_dest = local scope of the import statement even such as ""
   alias = "localname"
   declaration = "x"
   excludes = NULL
   +
   import_src = "A"
   import_dest = local scope of the import statement even such as ""
   alias = NULL
   declaration = NULL
   excludes = ["x"]
   All the entries of A get imported except of "x".  "x" gets imported as
   "localname".  "x" is not defined as a local name by this statement.

   C++:      namespace LOCALNS = A;
   Fortran has no way to address non-local namespace/module.
   import_src = "A"
   import_dest = local scope of the import statement even such as ""
   alias = "LOCALNS"
   declaration = NULL
   excludes = NULL
   The namespace will get imported as the import_dest::LOCALNS
   namespace.

   C++ cannot express it, it would be something like: using localname
   = A::x;
   Fortran:  use A, only localname => x
   import_src = "A"
   import_dest = local scope of the import statement even such as ""
   alias = "localname"
   declaration = "x"
   excludes = NULL
   The declaration will get imported as localname or
   `import_dest`localname.  */

struct using_direct
{
  const char *import_src;
  const char *import_dest;

  const char *alias;
  const char *declaration;

  struct using_direct *next;

  /* The line where the using directive was declared on the source file.
     This is used to check if the using directive is already active at the
     point where the inferior is stopped.  */
  unsigned int decl_line;

  /* Used during import search to temporarily mark this node as
     searched.  */
  int searched;

  /* USING_DIRECT has variable allocation size according to the number of
     EXCLUDES entries, the last entry is NULL.  */
  const char *excludes[1];

  /* Returns true if the using_directive USING_DIR is valid in CURR_LINE.
     Because current GCC (at least version 12.2) sets the decl_line as
     the last line in the current block, we need to take this into
     consideration when checking the validity, by comparing it to
     BOUNDARY, the last line of the current block.  */
  bool valid_line (unsigned int boundary) const;
};

extern void add_using_directive (struct using_direct **using_directives,
				 const char *dest,
				 const char *src,
				 const char *alias,
				 const char *declaration,
				 const std::vector<const char *> &excludes,
				 const unsigned int decl_line,
				 int copy_names,
				 struct obstack *obstack);

#endif /* NAMESPACE_H */
