/* Parser definitions for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

   Modified from expread.y by the Department of Computer Science at the
   State University of New York at Buffalo.

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

#if !defined (PARSER_DEFS_H)
#define PARSER_DEFS_H 1

#include "expression.h"
#include "symtab.h"
#include "expop.h"

struct block;
struct language_defn;
struct internalvar;
class innermost_block_tracker;

/* A class that can be used to build a "struct expression".  */

struct expr_builder
{
  /* Constructor.  LANG is the language used to parse the expression.
     And GDBARCH is the gdbarch to use during parsing.  */

  expr_builder (const struct language_defn *lang,
		struct gdbarch *gdbarch)
    : expout (new expression (lang, gdbarch))
  {
  }

  DISABLE_COPY_AND_ASSIGN (expr_builder);

  /* Resize the allocated expression to the correct size, and return
     it as an expression_up -- passing ownership to the caller.  */
  ATTRIBUTE_UNUSED_RESULT expression_up release ()
  {
    return std::move (expout);
  }

  /* Return the gdbarch that was passed to the constructor.  */

  struct gdbarch *gdbarch ()
  {
    return expout->gdbarch;
  }

  /* Return the language that was passed to the constructor.  */

  const struct language_defn *language ()
  {
    return expout->language_defn;
  }

  /* Set the root operation of the expression that is currently being
     built.  */
  void set_operation (expr::operation_up &&op)
  {
    expout->op = std::move (op);
  }

  /* The expression related to this parser state.  */

  expression_up expout;
};

/* Complete an expression that references a field, like "x->y".  */

struct expr_complete_structop : public expr_completion_base
{
  explicit expr_complete_structop (expr::structop_base_operation *op)
    : m_op (op)
  {
  }

  bool complete (struct expression *exp,
		 completion_tracker &tracker) override
  {
    return m_op->complete (exp, tracker);
  }

private:

  /* The last struct expression directly before a '.' or '->'.  This
     is set when parsing and is only used when completing a field
     name.  It is nullptr if no dereference operation was found.  */
  expr::structop_base_operation *m_op = nullptr;
};

/* Complete a tag name in an expression.  This is used for something
   like "enum abc<TAB>".  */

struct expr_complete_tag : public expr_completion_base
{
  expr_complete_tag (enum type_code code,
		     gdb::unique_xmalloc_ptr<char> name)
    : m_code (code),
      m_name (std::move (name))
  {
    /* Parsers should enforce this statically.  */
    gdb_assert (code == TYPE_CODE_ENUM
		|| code == TYPE_CODE_UNION
		|| code == TYPE_CODE_STRUCT);
  }

  bool complete (struct expression *exp,
		 completion_tracker &tracker) override;

private:

  /* The kind of tag to complete.  */
  enum type_code m_code;

  /* The token for tagged type name completion.  */
  gdb::unique_xmalloc_ptr<char> m_name;
};

/* An instance of this type is instantiated during expression parsing,
   and passed to the appropriate parser.  It holds both inputs to the
   parser, and result.  */

struct parser_state : public expr_builder
{
  /* Constructor.  LANG is the language used to parse the expression.
     And GDBARCH is the gdbarch to use during parsing.  */

  parser_state (const struct language_defn *lang,
		struct gdbarch *gdbarch,
		const struct block *context_block,
		CORE_ADDR context_pc,
		parser_flags flags,
		const char *input,
		bool completion,
		innermost_block_tracker *tracker)
    : expr_builder (lang, gdbarch),
      expression_context_block (context_block),
      expression_context_pc (context_pc),
      lexptr (input),
      start_of_input (input),
      block_tracker (tracker),
      comma_terminates ((flags & PARSER_COMMA_TERMINATES) != 0),
      parse_completion (completion),
      void_context_p ((flags & PARSER_VOID_CONTEXT) != 0),
      debug ((flags & PARSER_DEBUG) != 0)
  {
  }

  DISABLE_COPY_AND_ASSIGN (parser_state);

  /* Begin counting arguments for a function call,
     saving the data about any containing call.  */

  void start_arglist ()
  {
    m_funcall_chain.push_back (arglist_len);
    arglist_len = 0;
  }

  /* Return the number of arguments in a function call just terminated,
     and restore the data for the containing function call.  */

  int end_arglist ()
  {
    int val = arglist_len;
    arglist_len = m_funcall_chain.back ();
    m_funcall_chain.pop_back ();
    return val;
  }

  /* Mark the given operation as the starting location of a structure
     expression.  This is used when completing on field names.  */

  void mark_struct_expression (expr::structop_base_operation *op);

  /* Indicate that the current parser invocation is completing a tag.
     TAG is the type code of the tag, and PTR and LENGTH represent the
     start of the tag name.  */

  void mark_completion_tag (enum type_code tag, const char *ptr, int length);

  /* Mark for completion, using an arbitrary completer.  */

  void mark_completion (std::unique_ptr<expr_completion_base> completer)
  {
    gdb_assert (m_completion_state == nullptr);
    m_completion_state = std::move (completer);
  }

  /* Push an operation on the stack.  */
  void push (expr::operation_up &&op)
  {
    m_operations.push_back (std::move (op));
  }

  /* Create a new operation and push it on the stack.  */
  template<typename T, typename... Arg>
  void push_new (Arg... args)
  {
    m_operations.emplace_back (new T (std::forward<Arg> (args)...));
  }

  /* Push a new C string operation.  */
  void push_c_string (int, struct stoken_vector *vec);

  /* Push a symbol reference.  If SYM is nullptr, look for a minimal
     symbol.  */
  void push_symbol (const char *name, block_symbol sym);

  /* Push a reference to $mumble.  This may result in a convenience
     variable, a history reference, or a register.  */
  void push_dollar (struct stoken str);

  /* Pop an operation from the stack.  */
  expr::operation_up pop ()
  {
    expr::operation_up result = std::move (m_operations.back ());
    m_operations.pop_back ();
    return result;
  }

  /* Pop N elements from the stack and return a vector.  */
  std::vector<expr::operation_up> pop_vector (int n)
  {
    std::vector<expr::operation_up> result (n);
    for (int i = 1; i <= n; ++i)
      result[n - i] = pop ();
    return result;
  }

  /* A helper that pops an operation, wraps it in some other
     operation, and pushes it again.  */
  template<typename T>
  void wrap ()
  {
    using namespace expr;
    operation_up v = ::expr::make_operation<T> (pop ());
    push (std::move (v));
  }

  /* A helper that pops two operations, wraps them in some other
     operation, and pushes the result.  */
  template<typename T>
  void wrap2 ()
  {
    expr::operation_up rhs = pop ();
    expr::operation_up lhs = pop ();
    push (expr::make_operation<T> (std::move (lhs), std::move (rhs)));
  }

  /* Function called from the various parsers' yyerror functions to throw
     an error.  The error will include a message identifying the location
     of the error within the current expression.  */
  void parse_error (const char *msg);

  /* If this is nonzero, this block is used as the lexical context for
     symbol names.  */

  const struct block * const expression_context_block;

  /* If expression_context_block is non-zero, then this is the PC
     within the block that we want to evaluate expressions at.  When
     debugging C or C++ code, we use this to find the exact line we're
     at, and then look up the macro definitions active at that
     point.  */
  const CORE_ADDR expression_context_pc;

  /* During parsing of a C expression, the pointer to the next character
     is in this variable.  */

  const char *lexptr;

  /* After a token has been recognized, this variable points to it.
     Currently used only for error reporting.  */
  const char *prev_lexptr = nullptr;

  /* A pointer to the start of the full input, used for error reporting.  */
  const char *start_of_input = nullptr;

  /* Number of arguments seen so far in innermost function call.  */

  int arglist_len = 0;

  /* Completion state is updated here.  */
  std::unique_ptr<expr_completion_base> m_completion_state;

  /* The innermost block tracker.  */
  innermost_block_tracker *block_tracker;

  /* Nonzero means stop parsing on first comma (if not within parentheses).  */
  bool comma_terminates;

  /* True if parsing an expression to attempt completion.  */
  bool parse_completion;

  /* True if no value is expected from the expression.  */
  bool void_context_p;

  /* True if parser debugging should be enabled.  */
  bool debug;

private:

  /* Data structure for saving values of arglist_len for function calls whose
     arguments contain other function calls.  */

  std::vector<int> m_funcall_chain;

  /* Stack of operations.  */
  std::vector<expr::operation_up> m_operations;
};

/* A string token, either a char-string or bit-string.  Char-strings are
   used, for example, for the names of symbols.  */

struct stoken
  {
    /* Pointer to first byte of char-string or first bit of bit-string.  */
    const char *ptr;
    /* Length of string in bytes for char-string or bits for bit-string.  */
    int length;
  };

struct typed_stoken
  {
    /* A language-specific type field.  */
    int type;
    /* Pointer to first byte of char-string or first bit of bit-string.  */
    char *ptr;
    /* Length of string in bytes for char-string or bits for bit-string.  */
    int length;
  };

struct stoken_vector
  {
    int len;
    struct typed_stoken *tokens;
  };

struct ttype
  {
    struct stoken stoken;
    struct type *type;
  };

struct symtoken
  {
    struct stoken stoken;
    struct block_symbol sym;
    int is_a_field_of_this;
  };

struct objc_class_str
  {
    struct stoken stoken;
    struct type *type;
    int theclass;
  };

extern const char *find_template_name_end (const char *);

extern std::string copy_name (struct stoken);

extern bool parse_float (const char *p, int len,
			 const struct type *type, gdb_byte *data);
extern bool fits_in_type (int n_sign, ULONGEST n, int type_bits,
			  bool type_signed_p);
extern bool fits_in_type (int n_sign, const gdb_mpz &n, int type_bits,
			  bool type_signed_p);


/* Function used to avoid direct calls to fprintf
   in the code generated by the bison parser.  */

extern void parser_fprintf (FILE *, const char *, ...) ATTRIBUTE_PRINTF (2, 3);

#endif /* PARSER_DEFS_H */

