/* Objective-C language support definitions for GDB, the GNU debugger.

   Copyright (C) 1992-2024 Free Software Foundation, Inc.

   Contributed by Apple Computer, Inc.

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

#if !defined(OBJC_LANG_H)
#define OBJC_LANG_H

struct stoken;

struct value;
struct block;
struct parser_state;

extern CORE_ADDR lookup_objc_class     (struct gdbarch *gdbarch,
					const char *classname);
extern CORE_ADDR lookup_child_selector (struct gdbarch *gdbarch,
					const char *methodname);

extern int find_objc_msgcall (CORE_ADDR pc, CORE_ADDR *new_pc);

extern const char *find_imps (const char *method,
			      std::vector<const char *> *symbol_names);

extern struct value *value_nsstring (struct gdbarch *gdbarch,
				     const char *ptr, int len);

/* for parsing Objective C */
extern void start_msglist (void);
extern void add_msglist (struct stoken *str, int addcolon);
extern int end_msglist (struct parser_state *);

struct symbol *lookup_struct_typedef (const char *name,
				      const struct block *block,
				      int noerr);

#endif
