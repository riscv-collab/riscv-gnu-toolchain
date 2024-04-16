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

#include "defs.h"
#include "namespace.h"
#include "frame.h"
#include "symtab.h"

/* Add a using directive to USING_DIRECTIVES.  If the using directive
   in question has already been added, don't add it twice.

   Create a new struct using_direct which imports the namespace SRC
   into the scope DEST.  ALIAS is the name of the imported namespace
   in the current scope.  If ALIAS is NULL then the namespace is known
   by its original name.  DECLARATION is the name if the imported
   variable if this is a declaration import (Eg. using A::x), otherwise
   it is NULL.  EXCLUDES is a list of names not to import from an
   imported module or NULL.  If COPY_NAMES is non-zero, then the
   arguments are copied into newly allocated memory so they can be
   temporaries.  For EXCLUDES the contents of the vector are copied,
   but the pointed to characters are not copied.  */

void
add_using_directive (struct using_direct **using_directives,
		     const char *dest,
		     const char *src,
		     const char *alias,
		     const char *declaration,
		     const std::vector<const char *> &excludes,
		     unsigned int decl_line,
		     int copy_names,
		     struct obstack *obstack)
{
  struct using_direct *current;
  struct using_direct *newobj;
  int alloc_len;

  /* Has it already been added?  */

  for (current = *using_directives; current != NULL; current = current->next)
    {
      int ix;

      if (strcmp (current->import_src, src) != 0)
	continue;
      if (strcmp (current->import_dest, dest) != 0)
	continue;
      if ((alias == NULL && current->alias != NULL)
	  || (alias != NULL && current->alias == NULL)
	  || (alias != NULL && current->alias != NULL
	      && strcmp (alias, current->alias) != 0))
	continue;
      if ((declaration == NULL && current->declaration != NULL)
	  || (declaration != NULL && current->declaration == NULL)
	  || (declaration != NULL && current->declaration != NULL
	      && strcmp (declaration, current->declaration) != 0))
	continue;

      /* Compare the contents of EXCLUDES.  */
      for (ix = 0; ix < excludes.size (); ++ix)
	if (current->excludes[ix] == NULL
	    || strcmp (excludes[ix], current->excludes[ix]) != 0)
	  break;
      if (ix < excludes.size () || current->excludes[ix] != NULL)
	continue;

      if (decl_line != current->decl_line)
	continue;

      /* Parameters exactly match CURRENT.  */
      return;
    }

  alloc_len = (sizeof(*newobj)
	       + (excludes.size () * sizeof(*newobj->excludes)));
  newobj = (struct using_direct *) obstack_alloc (obstack, alloc_len);
  memset (newobj, 0, sizeof (*newobj));

  if (copy_names)
    {
      newobj->import_src = obstack_strdup (obstack, src);
      newobj->import_dest = obstack_strdup (obstack, dest);
    }
  else
    {
      newobj->import_src = src;
      newobj->import_dest = dest;
    }

  if (alias != NULL && copy_names)
    newobj->alias = obstack_strdup (obstack, alias);
  else
    newobj->alias = alias;

  if (declaration != NULL && copy_names)
    newobj->declaration = obstack_strdup (obstack, declaration);
  else
    newobj->declaration = declaration;

  if (!excludes.empty ())
    memcpy (newobj->excludes, excludes.data (),
	    excludes.size () * sizeof (*newobj->excludes));
  newobj->excludes[excludes.size ()] = NULL;

  newobj->decl_line = decl_line;

  newobj->next = *using_directives;
  *using_directives = newobj;
}

/* See namespace.h.  */

bool
using_direct::valid_line (unsigned int boundary) const
{
  try
    {
      CORE_ADDR curr_pc = get_frame_pc (get_selected_frame (nullptr));
      symtab_and_line curr_sal = find_pc_line (curr_pc, 0);
      return (decl_line <= curr_sal.line)
	     || (decl_line >= boundary);
    }
  catch (const gdb_exception &ex)
    {
      return true;
    }
}
