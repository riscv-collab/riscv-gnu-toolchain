/* Variable initialization.  MIPS version.
   Copyright (C) 2001, 2002, 2003, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <ldsodefs.h>

#ifdef SHARED

void
_dl_var_init (void *array[])
{
  /* It has to match "variables" below. */
  enum
    {
      DL_PAGESIZE = 0
    };

  GLRO(dl_pagesize) = *((size_t *) array[DL_PAGESIZE]);
}

#else

static void *variables[] =
{
  &GLRO(dl_pagesize)
};

static void
_dl_unprotect_relro (struct link_map *l)
{
  ElfW(Addr) start = ((l->l_addr + l->l_relro_addr)
		      & ~(GLRO(dl_pagesize) - 1));
  ElfW(Addr) end = ((l->l_addr + l->l_relro_addr + l->l_relro_size)
		    & ~(GLRO(dl_pagesize) - 1));

  if (start != end)
    __mprotect ((void *) start, end - start, PROT_READ | PROT_WRITE);
}

void
_dl_static_init (struct link_map *l)
{
  struct link_map *rtld_map = l;
  struct r_scope_elem **scope;
  const ElfW(Sym) *ref = NULL;
  lookup_t loadbase;
  void (*f) (void *[]);
  size_t i;

  loadbase = _dl_lookup_symbol_x ("_dl_var_init", l, &ref, l->l_local_scope,
				  NULL, 0, 1, NULL);
  
  for (scope = l->l_local_scope; *scope != NULL; scope++)
    for (i = 0; i < (*scope)->r_nlist; i++)
      if ((*scope)->r_list[i] == loadbase)
	{
	  rtld_map = (*scope)->r_list[i];
	  break;
	}

  if (ref != NULL)
    {
      f = (void (*) (void *[])) DL_SYMBOL_ADDRESS (loadbase, ref);
      _dl_unprotect_relro (rtld_map);
      f (variables);
      _dl_protect_relro (rtld_map);
    }
}

#endif
