/* C/C++ language support for compilation.

   Copyright (C) 2014-2024 Free Software Foundation, Inc.

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
#include "compile-internal.h"
#include "compile-c.h"
#include "compile-cplus.h"
#include "compile.h"
#include "c-lang.h"
#include "macrotab.h"
#include "macroscope.h"
#include "regcache.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/gdb-dlfcn.h"
#include "gdbsupport/preprocessor.h"
#include "gdbarch.h"

/* See compile-internal.h.  */

const char *
c_get_mode_for_size (int size)
{
  const char *mode = NULL;

  switch (size)
    {
    case 1:
      mode = "QI";
      break;
    case 2:
      mode = "HI";
      break;
    case 4:
      mode = "SI";
      break;
    case 8:
      mode = "DI";
      break;
    default:
      internal_error (_("Invalid GCC mode size %d."), size);
    }

  return mode;
}

/* See compile-internal.h.  */

std::string
c_get_range_decl_name (const struct dynamic_prop *prop)
{
  return string_printf ("__gdb_prop_%s", host_address_to_string (prop));
}



/* Load the plug-in library FE_LIBCC and return the initialization function
   FE_CONTEXT.  */

template <typename FUNCTYPE>
FUNCTYPE *
load_libcompile (const char *fe_libcc, const char *fe_context)
{
  FUNCTYPE *func;

  /* gdb_dlopen will call error () on an error, so no need to check
     value.  */
  gdb_dlhandle_up handle = gdb_dlopen (fe_libcc);
  func = (FUNCTYPE *) gdb_dlsym (handle, fe_context);

  if (func == NULL)
    error (_("could not find symbol %s in library %s"), fe_context, fe_libcc);

  /* Leave the library open.  */
  handle.release ();
  return func;
}

/* Return the compile instance associated with the current context.
   This function calls the symbol returned from the load_libcompile
   function.  FE_LIBCC is the library to load.  BASE_VERSION is the
   base compile plug-in version we support.  API_VERSION is the
   API version supported.  */

template <typename INSTTYPE, typename FUNCTYPE, typename CTXTYPE,
	  typename BASE_VERSION_TYPE, typename API_VERSION_TYPE>
std::unique_ptr<compile_instance>
get_compile_context (const char *fe_libcc, const char *fe_context,
		     BASE_VERSION_TYPE base_version,
		     API_VERSION_TYPE api_version)
{
  static FUNCTYPE *func;
  static CTXTYPE *context;

  if (func == NULL)
    {
      func = load_libcompile<FUNCTYPE> (fe_libcc, fe_context);
      gdb_assert (func != NULL);
    }

  context = (*func) (base_version, api_version);
  if (context == NULL)
    error (_("The loaded version of GCC does not support the required version "
	     "of the API."));

  return std::make_unique<INSTTYPE> (context);
}

/* A C-language implementation of get_compile_context.  */

std::unique_ptr<compile_instance>
c_get_compile_context ()
{
  return get_compile_context
    <compile_c_instance, gcc_c_fe_context_function, gcc_c_context,
    gcc_base_api_version, gcc_c_api_version>
    (STRINGIFY (GCC_C_FE_LIBCC), STRINGIFY (GCC_C_FE_CONTEXT),
     GCC_FE_VERSION_0, GCC_C_FE_VERSION_0);
}

/* A C++-language implementation of get_compile_context.  */

std::unique_ptr<compile_instance>
cplus_get_compile_context ()
{
  return get_compile_context
    <compile_cplus_instance, gcc_cp_fe_context_function, gcc_cp_context,
     gcc_base_api_version, gcc_cp_api_version>
    (STRINGIFY (GCC_CP_FE_LIBCC), STRINGIFY (GCC_CP_FE_CONTEXT),
     GCC_FE_VERSION_0, GCC_CP_FE_VERSION_0);
}



/* Write one macro definition.  */

static void
print_one_macro (const char *name, const struct macro_definition *macro,
		 struct macro_source_file *source, int line,
		 ui_file *file)
{
  /* Don't print command-line defines.  They will be supplied another
     way.  */
  if (line == 0)
    return;

  /* None of -Wno-builtin-macro-redefined, #undef first
     or plain #define of the same value would avoid a warning.  */
  gdb_printf (file, "#ifndef %s\n# define %s", name, name);

  if (macro->kind == macro_function_like)
    {
      int i;

      gdb_puts ("(", file);
      for (i = 0; i < macro->argc; i++)
	{
	  gdb_puts (macro->argv[i], file);
	  if (i + 1 < macro->argc)
	    gdb_puts (", ", file);
	}
      gdb_puts (")", file);
    }

  gdb_printf (file, " %s\n#endif\n", macro->replacement);
}

/* Write macro definitions at PC to FILE.  */

static void
write_macro_definitions (const struct block *block, CORE_ADDR pc,
			 struct ui_file *file)
{
  gdb::unique_xmalloc_ptr<struct macro_scope> scope;

  if (block != NULL)
    scope = sal_macro_scope (find_pc_line (pc, 0));
  else
    scope = default_macro_scope ();
  if (scope == NULL)
    scope = user_macro_scope ();

  if (scope != NULL && scope->file != NULL && scope->file->table != NULL)
    {
      macro_for_each_in_scope (scope->file, scope->line,
			       [&] (const char *name,
				    const macro_definition *macro,
				    macro_source_file *source,
				    int line)
	{
	  print_one_macro (name, macro, source, line, file);
	});
    }
}

/* Generate a structure holding all the registers used by the function
   we're generating.  */

static void
generate_register_struct (struct ui_file *stream, struct gdbarch *gdbarch,
			  const std::vector<bool> &registers_used)
{
  int i;
  int seen = 0;

  gdb_puts ("struct " COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG " {\n",
	    stream);

  if (!registers_used.empty ())
    for (i = 0; i < gdbarch_num_regs (gdbarch); ++i)
      {
	if (registers_used[i])
	  {
	    struct type *regtype = check_typedef (register_type (gdbarch, i));
	    std::string regname = compile_register_name_mangled (gdbarch, i);

	    seen = 1;

	    /* You might think we could use type_print here.  However,
	       target descriptions often use types with names like
	       "int64_t", which may not be defined in the inferior
	       (and in any case would not be looked up due to the
	       #pragma business).  So, we take a much simpler
	       approach: for pointer- or integer-typed registers, emit
	       the field in the most direct way; and for other
	       register types (typically flags or vectors), emit a
	       maximally-aligned array of the correct size.  */

	    gdb_puts ("  ", stream);
	    switch (regtype->code ())
	      {
	      case TYPE_CODE_PTR:
		gdb_printf (stream, "__gdb_uintptr %s",
			    regname.c_str ());
		break;

	      case TYPE_CODE_INT:
		{
		  const char *mode
		    = c_get_mode_for_size (regtype->length ());

		  if (mode != NULL)
		    {
		      if (regtype->is_unsigned ())
			gdb_puts ("unsigned ", stream);
		      gdb_printf (stream,
				  "int %s"
				  " __attribute__ ((__mode__(__%s__)))",
				  regname.c_str (),
				  mode);
		      break;
		    }
		}

		[[fallthrough]];

	      default:
		gdb_printf (stream,
			    "  unsigned char %s[%s]"
			    " __attribute__((__aligned__("
			    "__BIGGEST_ALIGNMENT__)))",
			    regname.c_str (),
			    pulongest (regtype->length ()));
	      }
	    gdb_puts (";\n", stream);
	  }
      }

  if (!seen)
    gdb_puts ("  char " COMPILE_I_SIMPLE_REGISTER_DUMMY ";\n",
	      stream);

  gdb_puts ("};\n\n", stream);
}

/* C-language policy to emit a push user expression pragma into BUF.  */

struct c_push_user_expression
{
  void push_user_expression (struct ui_file *buf)
  {
    gdb_puts ("#pragma GCC user_expression\n", buf);
  }
};

/* C-language policy to emit a pop user expression pragma into BUF.
   For C, this is a nop.  */

struct pop_user_expression_nop
{
  void pop_user_expression (struct ui_file *buf)
  {
    /* Nothing to do.  */
  }
};

/* C-language policy to construct a code header for a block of code.
   Takes a scope TYPE argument which selects the correct header to
   insert into BUF.  */

struct c_add_code_header
{
  void add_code_header (enum compile_i_scope_types type, struct ui_file *buf)
  {
    switch (type)
      {
      case COMPILE_I_SIMPLE_SCOPE:
	gdb_puts ("void "
		  GCC_FE_WRAPPER_FUNCTION
		  " (struct "
		  COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG
		  " *"
		  COMPILE_I_SIMPLE_REGISTER_ARG_NAME
		  ") {\n",
		  buf);
	break;

      case COMPILE_I_PRINT_ADDRESS_SCOPE:
      case COMPILE_I_PRINT_VALUE_SCOPE:
	/* <string.h> is needed for a memcpy call below.  */
	gdb_puts ("#include <string.h>\n"
		  "void "
		  GCC_FE_WRAPPER_FUNCTION
		  " (struct "
		  COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG
		  " *"
		  COMPILE_I_SIMPLE_REGISTER_ARG_NAME
		  ", "
		  COMPILE_I_PRINT_OUT_ARG_TYPE
		  " "
		  COMPILE_I_PRINT_OUT_ARG
		  ") {\n",
		  buf);
	break;

      case COMPILE_I_RAW_SCOPE:
	break;

      default:
	gdb_assert_not_reached ("Unknown compiler scope reached.");
      }
  }
};

/* C-language policy to construct a code footer for a block of code.
   Takes a scope TYPE which selects the correct footer to insert into BUF.  */

struct c_add_code_footer
{
  void add_code_footer (enum compile_i_scope_types type, struct ui_file *buf)
  {
    switch (type)
      {
      case COMPILE_I_SIMPLE_SCOPE:
      case COMPILE_I_PRINT_ADDRESS_SCOPE:
      case COMPILE_I_PRINT_VALUE_SCOPE:
	gdb_puts ("}\n", buf);
	break;

      case COMPILE_I_RAW_SCOPE:
	break;

      default:
	gdb_assert_not_reached ("Unknown compiler scope reached.");
      }
  }
};

/* C-language policy to emit the user code snippet INPUT into BUF based on the
   scope TYPE.  */

struct c_add_input
{
  void add_input (enum compile_i_scope_types type, const char *input,
		  struct ui_file *buf)
  {
    switch (type)
      {
      case COMPILE_I_PRINT_ADDRESS_SCOPE:
      case COMPILE_I_PRINT_VALUE_SCOPE:
	gdb_printf (buf,
		    "__auto_type " COMPILE_I_EXPR_VAL " = %s;\n"
		    "typeof (%s) *" COMPILE_I_EXPR_PTR_TYPE ";\n"
		    "memcpy (" COMPILE_I_PRINT_OUT_ARG ", %s"
		    COMPILE_I_EXPR_VAL ",\n"
		    "sizeof (*" COMPILE_I_EXPR_PTR_TYPE "));\n"
		    , input, input,
		    (type == COMPILE_I_PRINT_ADDRESS_SCOPE
		     ? "&" : ""));
	break;

      default:
	gdb_puts (input, buf);
	break;
      }
    gdb_puts ("\n", buf);
  }
};

/* C++-language policy to emit a push user expression pragma into
   BUF.  */

struct cplus_push_user_expression
{
  void push_user_expression (struct ui_file *buf)
  {
    gdb_puts ("#pragma GCC push_user_expression\n", buf);
  }
};

/* C++-language policy to emit a pop user expression pragma into BUF.  */

struct cplus_pop_user_expression
{
  void pop_user_expression (struct ui_file *buf)
  {
    gdb_puts ("#pragma GCC pop_user_expression\n", buf);
  }
};

/* C++-language policy to construct a code header for a block of code.
   Takes a scope TYPE argument which selects the correct header to
   insert into BUF.  */

struct cplus_add_code_header
{
  void add_code_header (enum compile_i_scope_types type, struct ui_file *buf)
  {
  switch (type)
    {
    case COMPILE_I_SIMPLE_SCOPE:
      gdb_puts ("void "
		GCC_FE_WRAPPER_FUNCTION
		" (struct "
		COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG
		" *"
		COMPILE_I_SIMPLE_REGISTER_ARG_NAME
		") {\n",
		buf);
      break;

    case COMPILE_I_PRINT_ADDRESS_SCOPE:
    case COMPILE_I_PRINT_VALUE_SCOPE:
      gdb_puts (
		"#include <cstring>\n"
		"#include <bits/move.h>\n"
		"void "
		GCC_FE_WRAPPER_FUNCTION
		" (struct "
		COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG
		" *"
		COMPILE_I_SIMPLE_REGISTER_ARG_NAME
		", "
		COMPILE_I_PRINT_OUT_ARG_TYPE
		" "
		COMPILE_I_PRINT_OUT_ARG
		") {\n",
		buf);
      break;

    case COMPILE_I_RAW_SCOPE:
      break;

    default:
      gdb_assert_not_reached ("Unknown compiler scope reached.");
    }
  }
};

/* C++-language policy to emit the user code snippet INPUT into BUF based on
   the scope TYPE.  */

struct cplus_add_input
{
  void add_input (enum compile_i_scope_types type, const char *input,
		  struct ui_file *buf)
  {
    switch (type)
      {
      case COMPILE_I_PRINT_VALUE_SCOPE:
      case COMPILE_I_PRINT_ADDRESS_SCOPE:
	gdb_printf
	  (buf,
	   /* "auto" strips ref- and cv- qualifiers, so we need to also strip
	      those from COMPILE_I_EXPR_PTR_TYPE.  */
	   "auto " COMPILE_I_EXPR_VAL " = %s;\n"
	   "typedef "
	     "std::add_pointer<std::remove_cv<decltype (%s)>::type>::type "
	     " __gdb_expr_ptr;\n"
	   "__gdb_expr_ptr " COMPILE_I_EXPR_PTR_TYPE ";\n"
	   "std::memcpy (" COMPILE_I_PRINT_OUT_ARG ", %s ("
	   COMPILE_I_EXPR_VAL "),\n"
	   "\tsizeof (*" COMPILE_I_EXPR_PTR_TYPE "));\n"
	   ,input, input,
	   (type == COMPILE_I_PRINT_ADDRESS_SCOPE
	    ? "__builtin_addressof" : ""));
	break;

      default:
	gdb_puts (input, buf);
	break;
      }
    gdb_puts ("\n", buf);
  }
};

/* A host class representing a compile program.

   CompileInstanceType is the type of the compile_instance for the
   language.

   PushUserExpressionPolicy and PopUserExpressionPolicy are used to
   push and pop user expression pragmas to the compile plug-in.

   AddCodeHeaderPolicy and AddCodeFooterPolicy are used to add the appropriate
   code header and footer, respectively.

   AddInputPolicy adds the actual user code.  */

template <class CompileInstanceType, class PushUserExpressionPolicy,
	  class PopUserExpressionPolicy, class AddCodeHeaderPolicy,
	  class AddCodeFooterPolicy, class AddInputPolicy>
class compile_program
  : private PushUserExpressionPolicy, private PopUserExpressionPolicy,
    private AddCodeHeaderPolicy, private AddCodeFooterPolicy,
    private AddInputPolicy
{
public:

  /* Construct a compile_program using the compiler instance INST
     using the architecture given by GDBARCH.  */
  compile_program (CompileInstanceType *inst, struct gdbarch *gdbarch)
    : m_instance (inst), m_arch (gdbarch)
  {
  }

  /* Take the source code provided by the user with the 'compile'
     command and compute the additional wrapping, macro, variable and
     register operations needed.  INPUT is the source code derived from
     the 'compile' command, EXPR_BLOCK denotes the block relevant contextually
     to the inferior when the expression was created, and EXPR_PC
     indicates the value of $PC.

     Returns the text of the program to compile.  */
  std::string compute (const char *input, const struct block *expr_block,
		       CORE_ADDR expr_pc)
  {
    string_file var_stream;
    string_file buf;

    /* Do not generate local variable information for "raw"
       compilations.  In this case we aren't emitting our own function
       and the user's code may only refer to globals.  */
    if (m_instance->scope () != COMPILE_I_RAW_SCOPE)
      {
	/* Generate the code to compute variable locations, but do it
	   before generating the function header, so we can define the
	   register struct before the function body.  This requires a
	   temporary stream.  */
	std::vector<bool> registers_used
	  = generate_c_for_variable_locations (m_instance, &var_stream, m_arch,
					       expr_block, expr_pc);

	buf.puts ("typedef unsigned int"
		  " __attribute__ ((__mode__(__pointer__)))"
		  " __gdb_uintptr;\n");
	buf.puts ("typedef int"
		  " __attribute__ ((__mode__(__pointer__)))"
		  " __gdb_intptr;\n");

	/* Iterate all log2 sizes in bytes supported by c_get_mode_for_size.  */
	for (int i = 0; i < 4; ++i)
	  {
	    const char *mode = c_get_mode_for_size (1 << i);

	    gdb_assert (mode != NULL);
	    buf.printf ("typedef int"
			" __attribute__ ((__mode__(__%s__)))"
			" __gdb_int_%s;\n",
			mode, mode);
	  }

	generate_register_struct (&buf, m_arch, registers_used);
      }

    AddCodeHeaderPolicy::add_code_header (m_instance->scope (), &buf);

    if (m_instance->scope () == COMPILE_I_SIMPLE_SCOPE
	|| m_instance->scope () == COMPILE_I_PRINT_ADDRESS_SCOPE
	|| m_instance->scope () == COMPILE_I_PRINT_VALUE_SCOPE)
      {
	buf.write (var_stream.c_str (), var_stream.size ());
	PushUserExpressionPolicy::push_user_expression (&buf);
      }

    write_macro_definitions (expr_block, expr_pc, &buf);

    /* The user expression has to be in its own scope, so that "extern"
       works properly.  Otherwise gcc thinks that the "extern"
       declaration is in the same scope as the declaration provided by
       gdb.  */
    if (m_instance->scope () != COMPILE_I_RAW_SCOPE)
      buf.puts ("{\n");

    buf.puts ("#line 1 \"gdb command line\"\n");

    AddInputPolicy::add_input (m_instance->scope (), input, &buf);

    /* For larger user expressions the automatic semicolons may be
       confusing.  */
    if (strchr (input, '\n') == NULL)
      buf.puts (";\n");

    if (m_instance->scope () != COMPILE_I_RAW_SCOPE)
      buf.puts ("}\n");

    if (m_instance->scope () == COMPILE_I_SIMPLE_SCOPE
	|| m_instance->scope () == COMPILE_I_PRINT_ADDRESS_SCOPE
	|| m_instance->scope () == COMPILE_I_PRINT_VALUE_SCOPE)
      PopUserExpressionPolicy::pop_user_expression (&buf);

    AddCodeFooterPolicy::add_code_footer (m_instance->scope (), &buf);
    return buf.release ();
  }

private:

  /* The compile instance to be used for compilation and
     type-conversion.  */
  CompileInstanceType *m_instance;

  /* The architecture to be used.  */
  struct gdbarch *m_arch;
};

/* The types used for C and C++ program computations.  */

typedef compile_program<compile_c_instance,
			c_push_user_expression, pop_user_expression_nop,
			c_add_code_header, c_add_code_footer,
			c_add_input> c_compile_program;

typedef compile_program<compile_cplus_instance,
			cplus_push_user_expression, cplus_pop_user_expression,
			cplus_add_code_header, c_add_code_footer,
			cplus_add_input> cplus_compile_program;

/* The compute_program method for C.  */

std::string
c_compute_program (compile_instance *inst,
		   const char *input,
		   struct gdbarch *gdbarch,
		   const struct block *expr_block,
		   CORE_ADDR expr_pc)
{
  compile_c_instance *c_inst = static_cast<compile_c_instance *> (inst);
  c_compile_program program (c_inst, gdbarch);

  return program.compute (input, expr_block, expr_pc);
}

/* The compute_program method for C++.  */

std::string
cplus_compute_program (compile_instance *inst,
		       const char *input,
		       struct gdbarch *gdbarch,
		       const struct block *expr_block,
		       CORE_ADDR expr_pc)
{
  compile_cplus_instance *cplus_inst
    = static_cast<compile_cplus_instance *> (inst);
  cplus_compile_program program (cplus_inst, gdbarch);

  return program.compute (input, expr_block, expr_pc);
}
