/* GCC C++ plug-in wrapper for GDB.

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

#ifndef COMPILE_GCC_CP_PLUGIN_H
#define COMPILE_GCC_CP_PLUGIN_H

/* A class representing the GCC C++ plug-in.  */

#include "gcc-cp-interface.h"

class gcc_cp_plugin
{
public:

  explicit gcc_cp_plugin (struct gcc_cp_context *gcc_cp)
    : m_context (gcc_cp)
  {
  }

  /* Set the oracle callbacks to be used by the compiler plug-in.  */
  void set_callbacks (gcc_cp_oracle_function *binding_oracle,
		      gcc_cp_symbol_address_function *address_oracle,
		      gcc_cp_enter_leave_user_expr_scope_function *enter_scope,
		      gcc_cp_enter_leave_user_expr_scope_function *leave_scope,
		      void *datum)
  {
    m_context->cp_ops->set_callbacks (m_context, binding_oracle,
				      address_oracle, enter_scope, leave_scope,
				      datum);
  }

  /* Returns the interface version of the compiler plug-in.  */
  int version () const { return m_context->cp_ops->cp_version; }

#define GCC_METHOD0(R, N) R N () const;
#define GCC_METHOD1(R, N, A) R N (A) const;
#define GCC_METHOD2(R, N, A, B) R N (A, B) const;
#define GCC_METHOD3(R, N, A, B, C) R N (A, B, C) const;
#define GCC_METHOD4(R, N, A, B, C, D) R N (A, B, C, D) const;
#define GCC_METHOD5(R, N, A, B, C, D, E) R N (A, B, C, D, E) const;
#define GCC_METHOD7(R, N, A, B, C, D, E, F, G) R N (A, B, C, D, E, F, G) const;

#include "gcc-cp-fe.def"

#undef GCC_METHOD0
#undef GCC_METHOD1
#undef GCC_METHOD2
#undef GCC_METHOD3
#undef GCC_METHOD4
#undef GCC_METHOD5
#undef GCC_METHOD7

  /* Special overloads of plug-in methods with added debugging information.  */

  gcc_expr build_decl (const char *debug_decltype, const char *name,
		       enum gcc_cp_symbol_kind sym_kind, gcc_type sym_type,
		       const char *substitution_name, gcc_address address,
		       const char *filename, unsigned int line_number);

  gcc_type start_class_type (const char *debug_name, gcc_decl typedecl,
			     const struct gcc_vbase_array *base_classes,
			     const char *filename, unsigned int line_number);

  int finish_class_type (const char *debug_name, unsigned long size_in_bytes);

  int pop_binding_level (const char *debug_name);

private:

  /* The GCC C++ context.  */
  struct gcc_cp_context *m_context;
};

#endif /* COMPILE_GCC_CP_PLUGIN_H */
