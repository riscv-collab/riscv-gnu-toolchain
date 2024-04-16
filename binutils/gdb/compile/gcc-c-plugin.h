/* GCC C plug-in wrapper for GDB.

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

#ifndef COMPILE_GCC_C_PLUGIN_H
#define COMPILE_GCC_C_PLUGIN_H

#include "compile-internal.h"

/* A class representing the C plug-in.  */

class gcc_c_plugin
{
public:

  explicit gcc_c_plugin (struct gcc_c_context *gcc_c)
    : m_context (gcc_c)
  {
  }

  /* Set the oracle callbacks to be used by the compiler plug-in.  */
  void set_callbacks (gcc_c_oracle_function *binding_oracle,
		      gcc_c_symbol_address_function *address_oracle,
		      void *datum)
  {
    m_context->c_ops->set_callbacks (m_context, binding_oracle,
				     address_oracle, datum);
  }

  /* Returns the interface version of the compiler plug-in.  */
  int version () const { return m_context->c_ops->c_version; }

#define GCC_METHOD0(R, N) R N () const;
#define GCC_METHOD1(R, N, A) R N (A) const;
#define GCC_METHOD2(R, N, A, B) R N (A, B) const;
#define GCC_METHOD3(R, N, A, B, C) R N (A, B, C) const;
#define GCC_METHOD4(R, N, A, B, C, D) R N (A, B, C, D) const;
#define GCC_METHOD5(R, N, A, B, C, D, E) R N (A, B, C, D, E) const;
#define GCC_METHOD7(R, N, A, B, C, D, E, F, G) R N (A, B, C, D, E, F, G) const;

#include "gcc-c-fe.def"

#undef GCC_METHOD0
#undef GCC_METHOD1
#undef GCC_METHOD2
#undef GCC_METHOD3
#undef GCC_METHOD4
#undef GCC_METHOD5
#undef GCC_METHOD7

private:
  /* The GCC C context.  */
  struct gcc_c_context *m_context;
};

#endif /* COMPILE_GCC_C_PLUGIN_H */
