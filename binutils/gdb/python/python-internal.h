/* Gdb/Python header for private use by Python module.

   Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef PYTHON_PYTHON_INTERNAL_H
#define PYTHON_PYTHON_INTERNAL_H

#include "extension.h"
#include "extension-priv.h"

/* These WITH_* macros are defined by the CPython API checker that
   comes with the Python plugin for GCC.  See:
   https://gcc-python-plugin.readthedocs.org/en/latest/cpychecker.html
   The checker defines a WITH_ macro for each attribute it
   exposes.  Note that we intentionally do not use
   'cpychecker_returns_borrowed_ref' -- that idiom is forbidden in
   gdb.  */

#ifdef WITH_CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF_ATTRIBUTE
#define CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF(ARG)		\
  __attribute__ ((cpychecker_type_object_for_typedef (ARG)))
#else
#define CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF(ARG)
#endif

#ifdef WITH_CPYCHECKER_SETS_EXCEPTION_ATTRIBUTE
#define CPYCHECKER_SETS_EXCEPTION __attribute__ ((cpychecker_sets_exception))
#else
#define CPYCHECKER_SETS_EXCEPTION
#endif

#ifdef WITH_CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION_ATTRIBUTE
#define CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION		\
  __attribute__ ((cpychecker_negative_result_sets_exception))
#else
#define CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION
#endif

/* /usr/include/features.h on linux systems will define _POSIX_C_SOURCE
   if it sees _GNU_SOURCE (which config.h will define).
   pyconfig.h defines _POSIX_C_SOURCE to a different value than
   /usr/include/features.h does causing compilation to fail.
   To work around this, undef _POSIX_C_SOURCE before we include Python.h.

   Same problem with _XOPEN_SOURCE.  */
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

/* On sparc-solaris, /usr/include/sys/feature_tests.h defines
   _FILE_OFFSET_BITS, which pyconfig.h also defines.  Same work
   around technique as above.  */
#undef _FILE_OFFSET_BITS

/* A kludge to avoid redefinition of snprintf on Windows by pyerrors.h.  */
#if defined(_WIN32) && defined(HAVE_DECL_SNPRINTF)
#define HAVE_SNPRINTF 1
#endif

/* Another kludge to avoid compilation errors because MinGW defines
   'hypot' to '_hypot', but the C++ headers says "using ::hypot".  */
#ifdef __MINGW32__
# define _hypot hypot
#endif

/* Request clean size types from Python.  */
#define PY_SSIZE_T_CLEAN

/* Include the Python header files using angle brackets rather than
   double quotes.  On case-insensitive filesystems, this prevents us
   from including our python/python.h header file.  */
#include <Python.h>
#include <frameobject.h>
#include "py-ref.h"

#define Py_TPFLAGS_CHECKTYPES 0

/* If Python.h does not define WITH_THREAD, then the various
   GIL-related functions will not be defined.  However,
   PyGILState_STATE will be.  */
#ifndef WITH_THREAD
#define PyGILState_Ensure() ((PyGILState_STATE) 0)
#define PyGILState_Release(ARG) ((void)(ARG))
#define PyEval_InitThreads()
#define PyThreadState_Swap(ARG) ((void)(ARG))
#define PyEval_ReleaseLock()
#endif

/* Python supplies HAVE_LONG_LONG and some `long long' support when it
   is available.  These defines let us handle the differences more
   cleanly.  */
#ifdef HAVE_LONG_LONG

#define GDB_PY_LL_ARG "L"
#define GDB_PY_LLU_ARG "K"
typedef PY_LONG_LONG gdb_py_longest;
typedef unsigned PY_LONG_LONG gdb_py_ulongest;
#define gdb_py_long_as_ulongest PyLong_AsUnsignedLongLong
#define gdb_py_long_as_long_and_overflow PyLong_AsLongLongAndOverflow

#else /* HAVE_LONG_LONG */

#define GDB_PY_LL_ARG "L"
#define GDB_PY_LLU_ARG "K"
typedef long gdb_py_longest;
typedef unsigned long gdb_py_ulongest;
#define gdb_py_long_as_ulongest PyLong_AsUnsignedLong
#define gdb_py_long_as_long_and_overflow PyLong_AsLongAndOverflow

#endif /* HAVE_LONG_LONG */

#if PY_VERSION_HEX < 0x03020000
typedef long Py_hash_t;
#endif

/* PyMem_RawMalloc appeared in Python 3.4.  For earlier versions, we can just
   fall back to PyMem_Malloc.  */

#if PY_VERSION_HEX < 0x03040000
#define PyMem_RawMalloc PyMem_Malloc
#endif

/* PyObject_CallMethod's 'method' and 'format' parameters were missing
   the 'const' qualifier before Python 3.4.  Hence, we wrap the
   function in our own version to avoid errors with string literals.
   Note, this is a variadic template because PyObject_CallMethod is a
   varargs function and Python doesn't have a "PyObject_VaCallMethod"
   variant taking a va_list that we could defer to instead.  */

template<typename... Args>
static inline PyObject *
gdb_PyObject_CallMethod (PyObject *o, const char *method, const char *format,
			 Args... args) /* ARI: editCase function */
{
  return PyObject_CallMethod (o,
			      const_cast<char *> (method),
			      const_cast<char *> (format),
			      args...);
}

#undef PyObject_CallMethod
#define PyObject_CallMethod gdb_PyObject_CallMethod

/* The 'name' parameter of PyErr_NewException was missing the 'const'
   qualifier in Python <= 3.4.  Hence, we wrap it in a function to
   avoid errors when compiled with -Werror.  */

static inline PyObject*
gdb_PyErr_NewException (const char *name, PyObject *base, PyObject *dict)
{
  return PyErr_NewException (const_cast<char *> (name), base, dict);
}

#define PyErr_NewException gdb_PyErr_NewException

/* PySys_GetObject's 'name' parameter was missing the 'const'
   qualifier before Python 3.4.  Hence, we wrap it in a function to
   avoid errors when compiled with -Werror.  */

static inline PyObject *
gdb_PySys_GetObject (const char *name)
{
  return PySys_GetObject (const_cast<char *> (name));
}

#define PySys_GetObject gdb_PySys_GetObject

/* PySys_SetPath was deprecated in Python 3.11.  Disable the deprecated
   code for Python 3.10 and newer.  */
#if PY_VERSION_HEX < 0x030a0000

/* PySys_SetPath's 'path' parameter was missing the 'const' qualifier
   before Python 3.6.  Hence, we wrap it in a function to avoid errors
   when compiled with -Werror.  */

# define GDB_PYSYS_SETPATH_CHAR wchar_t

static inline void
gdb_PySys_SetPath (const GDB_PYSYS_SETPATH_CHAR *path)
{
  PySys_SetPath (const_cast<GDB_PYSYS_SETPATH_CHAR *> (path));
}

#define PySys_SetPath gdb_PySys_SetPath
#endif

/* Wrap PyGetSetDef to allow convenient construction with string
   literals.  Unfortunately, PyGetSetDef's 'name' and 'doc' members
   are 'char *' instead of 'const char *', meaning that in order to
   list-initialize PyGetSetDef arrays with string literals (and
   without the wrapping below) would require writing explicit 'char *'
   casts.  Instead, we extend PyGetSetDef and add constexpr
   constructors that accept const 'name' and 'doc', hiding the ugly
   casts here in a single place.  */

struct gdb_PyGetSetDef : PyGetSetDef
{
  constexpr gdb_PyGetSetDef (const char *name_, getter get_, setter set_,
			     const char *doc_, void *closure_)
    : PyGetSetDef {const_cast<char *> (name_), get_, set_,
		   const_cast<char *> (doc_), closure_}
  {}

  /* Alternative constructor that allows omitting the closure in list
     initialization.  */
  constexpr gdb_PyGetSetDef (const char *name_, getter get_, setter set_,
			     const char *doc_)
    : gdb_PyGetSetDef {name_, get_, set_, doc_, NULL}
  {}

  /* Constructor for the sentinel entries.  */
  constexpr gdb_PyGetSetDef (std::nullptr_t)
    : gdb_PyGetSetDef {NULL, NULL, NULL, NULL, NULL}
  {}
};

/* The 'keywords' parameter of PyArg_ParseTupleAndKeywords has type
   'char **'.  However, string literals are const in C++, and so to
   avoid casting at every keyword array definition, we'll need to make
   the keywords array an array of 'const char *'.  To avoid having all
   callers add a 'const_cast<char **>' themselves when passing such an
   array through 'char **', we define our own version of
   PyArg_ParseTupleAndKeywords here with a corresponding 'keywords'
   parameter type that does the cast in a single place.  (This is not
   an overload of PyArg_ParseTupleAndKeywords in order to make it
   clearer that we're calling our own function instead of a function
   that exists in some newer Python version.)  */

static inline int
gdb_PyArg_ParseTupleAndKeywords (PyObject *args, PyObject *kw,
				 const char *format, const char **keywords, ...)
{
  va_list ap;
  int res;

  va_start (ap, keywords);
  res = PyArg_VaParseTupleAndKeywords (args, kw, format,
				       const_cast<char **> (keywords),
				       ap);
  va_end (ap);

  return res;
}

/* In order to be able to parse symtab_and_line_to_sal_object function
   a real symtab_and_line structure is needed.  */
#include "symtab.h"

/* Also needed to parse enum var_types. */
#include "command.h"
#include "breakpoint.h"

enum gdbpy_iter_kind { iter_keys, iter_values, iter_items };

struct block;
struct value;
struct language_defn;
struct program_space;
struct bpstat;
struct inferior;

extern int gdb_python_initialized;

extern PyObject *gdb_module;
extern PyObject *gdb_python_module;
extern PyTypeObject value_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("value_object");
extern PyTypeObject block_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF("block_object");
extern PyTypeObject symbol_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("symbol_object");
extern PyTypeObject event_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("event_object");
extern PyTypeObject breakpoint_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("breakpoint_object");
extern PyTypeObject frame_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("frame_object");
extern PyTypeObject thread_object_type
    CPYCHECKER_TYPE_OBJECT_FOR_TYPEDEF ("thread_object");

/* Ensure that breakpoint_object_type is initialized and return true.  If
   breakpoint_object_type can't be initialized then set a suitable Python
   error and return false.

   This function needs to be called from any gdbpy_initialize_* function
   that wants to reference breakpoint_object_type.  After all the
   gdbpy_initialize_* functions have been called then breakpoint_object_type
   is guaranteed to have been initialized, and this function does not need
   calling before referencing breakpoint_object_type.  */

extern bool gdbpy_breakpoint_init_breakpoint_type ();

struct gdbpy_breakpoint_object
{
  PyObject_HEAD

  /* The breakpoint number according to gdb.  */
  int number;

  /* The gdb breakpoint object, or NULL if the breakpoint has been
     deleted.  */
  struct breakpoint *bp;

  /* 1 is this is a FinishBreakpoint object, 0 otherwise.  */
  int is_finish_bp;
};

/* Require that BREAKPOINT be a valid breakpoint ID; throw a Python
   exception if it is invalid.  */
#define BPPY_REQUIRE_VALID(Breakpoint)                                  \
    do {                                                                \
      if ((Breakpoint)->bp == NULL)                                     \
	return PyErr_Format (PyExc_RuntimeError,                        \
			     _("Breakpoint %d is invalid."),            \
			     (Breakpoint)->number);                     \
    } while (0)

/* Require that BREAKPOINT be a valid breakpoint ID; throw a Python
   exception if it is invalid.  This macro is for use in setter functions.  */
#define BPPY_SET_REQUIRE_VALID(Breakpoint)                              \
    do {                                                                \
      if ((Breakpoint)->bp == NULL)                                     \
	{                                                               \
	  PyErr_Format (PyExc_RuntimeError, _("Breakpoint %d is invalid."), \
			(Breakpoint)->number);                          \
	  return -1;                                                    \
	}                                                               \
    } while (0)


/* Variables used to pass information between the Breakpoint
   constructor and the breakpoint-created hook function.  */
extern gdbpy_breakpoint_object *bppy_pending_object;


struct thread_object
{
  PyObject_HEAD

  /* The thread we represent.  */
  struct thread_info *thread;

  /* The Inferior object to which this thread belongs.  */
  PyObject *inf_obj;

  /* Dictionary holding user-added attributes.  This is the __dict__
     attribute of the object.  */
  PyObject *dict;
};

struct inferior_object;

extern struct cmd_list_element *set_python_list;
extern struct cmd_list_element *show_python_list;

/* extension_language_script_ops "methods".  */

/* Return true if auto-loading Python scripts is enabled.
   This is the extension_language_script_ops.auto_load_enabled "method".  */

extern bool gdbpy_auto_load_enabled (const struct extension_language_defn *);

/* extension_language_ops "methods".  */

extern enum ext_lang_rc gdbpy_apply_val_pretty_printer
  (const struct extension_language_defn *,
   struct value *value,
   struct ui_file *stream, int recurse,
   const struct value_print_options *options,
   const struct language_defn *language);
extern enum ext_lang_bt_status gdbpy_apply_frame_filter
  (const struct extension_language_defn *,
   frame_info_ptr frame, frame_filter_flags flags,
   enum ext_lang_frame_args args_type,
   struct ui_out *out, int frame_low, int frame_high);
extern void gdbpy_preserve_values (const struct extension_language_defn *,
				   struct objfile *objfile,
				   htab_t copied_types);
extern enum ext_lang_bp_stop gdbpy_breakpoint_cond_says_stop
  (const struct extension_language_defn *, struct breakpoint *);
extern int gdbpy_breakpoint_has_cond (const struct extension_language_defn *,
				      struct breakpoint *b);

extern enum ext_lang_rc gdbpy_get_matching_xmethod_workers
  (const struct extension_language_defn *extlang,
   struct type *obj_type, const char *method_name,
   std::vector<xmethod_worker_up> *dm_vec);


PyObject *gdbpy_history (PyObject *self, PyObject *args);
PyObject *gdbpy_add_history (PyObject *self, PyObject *args);
extern PyObject *gdbpy_history_count (PyObject *self, PyObject *args);
PyObject *gdbpy_convenience_variable (PyObject *self, PyObject *args);
PyObject *gdbpy_set_convenience_variable (PyObject *self, PyObject *args);
PyObject *gdbpy_breakpoints (PyObject *, PyObject *);
PyObject *gdbpy_frame_stop_reason_string (PyObject *, PyObject *);
PyObject *gdbpy_lookup_symbol (PyObject *self, PyObject *args, PyObject *kw);
PyObject *gdbpy_lookup_global_symbol (PyObject *self, PyObject *args,
				      PyObject *kw);
PyObject *gdbpy_lookup_static_symbol (PyObject *self, PyObject *args,
				      PyObject *kw);
PyObject *gdbpy_lookup_static_symbols (PyObject *self, PyObject *args,
					   PyObject *kw);
PyObject *gdbpy_start_recording (PyObject *self, PyObject *args);
PyObject *gdbpy_current_recording (PyObject *self, PyObject *args);
PyObject *gdbpy_stop_recording (PyObject *self, PyObject *args);
PyObject *gdbpy_newest_frame (PyObject *self, PyObject *args);
PyObject *gdbpy_selected_frame (PyObject *self, PyObject *args);
PyObject *gdbpy_lookup_type (PyObject *self, PyObject *args, PyObject *kw);
int gdbpy_is_field (PyObject *obj);
PyObject *gdbpy_create_lazy_string_object (CORE_ADDR address, long length,
					   const char *encoding,
					   struct type *type);
PyObject *gdbpy_inferiors (PyObject *unused, PyObject *unused2);
PyObject *gdbpy_create_ptid_object (ptid_t ptid);
PyObject *gdbpy_selected_thread (PyObject *self, PyObject *args);
PyObject *gdbpy_selected_inferior (PyObject *self, PyObject *args);
PyObject *gdbpy_string_to_argv (PyObject *self, PyObject *args);
PyObject *gdbpy_parameter_value (const setting &var);
gdb::unique_xmalloc_ptr<char> gdbpy_parse_command_name
  (const char *name, struct cmd_list_element ***base_list,
   struct cmd_list_element **start_list);
PyObject *gdbpy_register_tui_window (PyObject *self, PyObject *args,
				     PyObject *kw);

PyObject *symtab_and_line_to_sal_object (struct symtab_and_line sal);
PyObject *symtab_to_symtab_object (struct symtab *symtab);
PyObject *symbol_to_symbol_object (struct symbol *sym);
PyObject *block_to_block_object (const struct block *block,
				 struct objfile *objfile);
PyObject *value_to_value_object (struct value *v);
PyObject *type_to_type_object (struct type *);
PyObject *frame_info_to_frame_object (frame_info_ptr frame);
PyObject *symtab_to_linetable_object (PyObject *symtab);
gdbpy_ref<> pspace_to_pspace_object (struct program_space *);
PyObject *pspy_get_printers (PyObject *, void *);
PyObject *pspy_get_frame_filters (PyObject *, void *);
PyObject *pspy_get_frame_unwinders (PyObject *, void *);
PyObject *pspy_get_xmethods (PyObject *, void *);

gdbpy_ref<> objfile_to_objfile_object (struct objfile *);
PyObject *objfpy_get_printers (PyObject *, void *);
PyObject *objfpy_get_frame_filters (PyObject *, void *);
PyObject *objfpy_get_frame_unwinders (PyObject *, void *);
PyObject *objfpy_get_xmethods (PyObject *, void *);
PyObject *gdbpy_lookup_objfile (PyObject *self, PyObject *args, PyObject *kw);

PyObject *gdbarch_to_arch_object (struct gdbarch *gdbarch);
PyObject *gdbpy_all_architecture_names (PyObject *self, PyObject *args);

PyObject *gdbpy_new_register_descriptor_iterator (struct gdbarch *gdbarch,
						  const char *group_name);
PyObject *gdbpy_new_reggroup_iterator (struct gdbarch *gdbarch);

gdbpy_ref<thread_object> create_thread_object (struct thread_info *tp);
gdbpy_ref<> thread_to_thread_object (thread_info *thr);;
gdbpy_ref<inferior_object> inferior_to_inferior_object (inferior *inf);

PyObject *gdbpy_buffer_to_membuf (gdb::unique_xmalloc_ptr<gdb_byte> buffer,
				  CORE_ADDR address, ULONGEST length);

struct process_stratum_target;
gdbpy_ref<> target_to_connection_object (process_stratum_target *target);
PyObject *gdbpy_connections (PyObject *self, PyObject *args);

const struct block *block_object_to_block (PyObject *obj);
struct symbol *symbol_object_to_symbol (PyObject *obj);
struct value *value_object_to_value (PyObject *self);
struct value *convert_value_from_python (PyObject *obj);
struct type *type_object_to_type (PyObject *obj);
struct symtab *symtab_object_to_symtab (PyObject *obj);
struct symtab_and_line *sal_object_to_symtab_and_line (PyObject *obj);
frame_info_ptr frame_object_to_frame_info (PyObject *frame_obj);
struct gdbarch *arch_object_to_gdbarch (PyObject *obj);

extern PyObject *gdbpy_execute_mi_command (PyObject *self, PyObject *args,
					   PyObject *kw);

/* Serialize RESULTS and print it in MI format to the current_uiout.

   This function handles the top-level results passed as a dictionary.
   The caller is responsible for ensuring that.  The values within this
   dictionary can be a wider range of types.  Handling the values of the top-level
   dictionary is done by serialize_mi_result_1, see that function for more
   details.

   If anything goes wrong while parsing and printing the MI output then an
   error is thrown.  */

extern void serialize_mi_results (PyObject *results);

/* Implementation of the gdb.notify_mi function.  */

extern PyObject *gdbpy_notify_mi (PyObject *self, PyObject *args,
				  PyObject *kw);

/* Convert Python object OBJ to a program_space pointer.  OBJ must be a
   gdb.Progspace reference.  Return nullptr if the gdb.Progspace is not
   valid (see gdb.Progspace.is_valid), otherwise return the program_space
   pointer.  */

extern struct program_space *progspace_object_to_program_space (PyObject *obj);

/* A class for managing the initialization, and finalization functions
   from all Python files (e.g. gdb/python/py-*.c).

   Within any Python file, create an instance of this class, passing in
   the initialization function, and, optionally, the finalization
   function.

   These functions are added to a single global list of functions, which
   can then be called from do_start_initialization and finalize_python
   (see python.c) to initialize all the Python files within GDB.  */

class gdbpy_initialize_file
{
  /* The type of a function that can be called just after GDB has setup the
     Python interpreter.  This function will setup any additional Python
     state required by a particular subsystem.  Return 0 if the setup was
     successful, or return -1 if setup failed, in which case a Python
     exception should have been raised.  */

  using gdbpy_initialize_file_ftype = int (*) (void);

  /* The type of a function that can be called just before GDB shuts down
     the Python interpreter.  This function can cleanup an Python state
     that is cached within GDB, for example, if GDB is holding any
     references to Python objects, these should be released before the
     Python interpreter is shut down.

     There is no error return in this case.  This function is only called
     when GDB is already shutting down.  The function should make a best
     effort to clean up, and then return.  */

  using gdbpy_finalize_file_ftype = void (*) (void);

  /* The type for an initialization and finalization function pair.  */

  using callback_pair_t = std::pair<gdbpy_initialize_file_ftype,
				    gdbpy_finalize_file_ftype>;

  /* Return the vector of callbacks.  The vector is defined as a static
     variable within this function so that it will be initialized the first
     time this function is called.  This is important, as this function is
     called as part of the global object initialization process; if the
     vector was a static variable within this class then we could not
     guarantee that it had been initialized before it was used.  */

  static std::vector<callback_pair_t> &
  callbacks ()
  {
    static std::vector<callback_pair_t> list;
    return list;
  }

public:

  /* Register the initialization (INIT) and finalization (FINI) functions
     for a Python file.  See the comments on the function types above for
     when these functions will be called.

     Either of these functions can be nullptr, in which case no function
     will be called.

     The FINI argument is optional, and defaults to nullptr (no function to
     call).  */

  gdbpy_initialize_file (gdbpy_initialize_file_ftype init,
			 gdbpy_finalize_file_ftype fini = nullptr)
  {
    callbacks ().emplace_back (init, fini);
  }

  /* Run all the Python file initialize functions and return true.  If any
     of the initialize functions fails then this function returns false.
     In the case of failure it is undefined how many of the initialize
     functions will have been called.  */

  static bool
  initialize_all ()
  {
    /* The initialize_all function should only be called once.  The
       following check reverses the global list, which will effect this
       initialize_all call, as well as the later finalize_all call.

       The environment variable checked here is the same as the one checked
       in the generated init.c file.  */
    if (getenv ("GDB_REVERSE_INIT_FUNCTIONS") != nullptr)
      std::reverse (callbacks ().begin (), callbacks ().end ());

    for (const auto &p : gdbpy_initialize_file::callbacks ())
      {
	if (p.first != nullptr && p.first () < 0)
	  return false;
      }
    return true;
  }

  /* Run all the Python file finalize functions.  */

  static void
  finalize_all ()
  {
    for (const auto &p : gdbpy_initialize_file::callbacks ())
      {
	if (p.second != nullptr)
	  p.second ();
      }
  }
};

/* Macro to simplify registering the initialization and finalization
   functions for a Python file.  */

#define GDBPY_INITIALIZE_FILE(INIT, ...)				\
  static gdbpy_initialize_file						\
    CONCAT(gdbpy_initialize_file_obj_, __LINE__) (INIT, ##__VA_ARGS__)

PyMODINIT_FUNC gdbpy_events_mod_func ();

/* A wrapper for PyErr_Fetch that handles reference counting for the
   caller.  */
class gdbpy_err_fetch
{
public:

  gdbpy_err_fetch ()
  {
    PyObject *error_type, *error_value, *error_traceback;

    PyErr_Fetch (&error_type, &error_value, &error_traceback);
    m_error_type.reset (error_type);
    m_error_value.reset (error_value);
    m_error_traceback.reset (error_traceback);
  }

  /* Call PyErr_Restore using the values stashed in this object.
     After this call, this object is invalid and neither the to_string
     nor restore methods may be used again.  */

  void restore ()
  {
    PyErr_Restore (m_error_type.release (),
		   m_error_value.release (),
		   m_error_traceback.release ());
  }

  /* Return the string representation of the exception represented by
     this object.  If the result is NULL a python error occurred, the
     caller must clear it.  */

  gdb::unique_xmalloc_ptr<char> to_string () const;

  /* Return the string representation of the type of the exception
     represented by this object.  If the result is NULL a python error
     occurred, the caller must clear it.  */

  gdb::unique_xmalloc_ptr<char> type_to_string () const;

  /* Return true if the stored type matches TYPE, false otherwise.  */

  bool type_matches (PyObject *type) const
  {
    return PyErr_GivenExceptionMatches (m_error_type.get (), type);
  }

  /* Return a new reference to the exception value object.  */

  gdbpy_ref<> value ()
  {
    return m_error_value;
  }

private:

  gdbpy_ref<> m_error_type, m_error_value, m_error_traceback;
};

/* Called before entering the Python interpreter to install the
   current language and architecture to be used for Python values.
   Also set the active extension language for GDB so that SIGINT's
   are directed our way, and if necessary install the right SIGINT
   handler.  */
class gdbpy_enter
{
 public:

  /* Set the ambient Python architecture to GDBARCH and the language
     to LANGUAGE.  If GDBARCH is nullptr, then the architecture will
     be computed, when needed, using get_current_arch; see the
     get_gdbarch method.  If LANGUAGE is not nullptr, then the current
     language at time of construction will be saved (to be restored on
     destruction), and the current language will be set to
     LANGUAGE.  */
  explicit gdbpy_enter (struct gdbarch *gdbarch = nullptr,
			const struct language_defn *language = nullptr);

  ~gdbpy_enter ();

  DISABLE_COPY_AND_ASSIGN (gdbpy_enter);

  /* Return the current gdbarch, as known to the Python layer.  This
     is either python_gdbarch (which comes from the most recent call
     to the gdbpy_enter constructor), or, if that is nullptr, the
     result of get_current_arch.  */
  static struct gdbarch *get_gdbarch ();

  /* Called only during gdb shutdown.  This sets python_gdbarch to an
     acceptable value.  */
  static void finalize ();

 private:

  /* The current gdbarch, according to Python.  This can be
     nullptr.  */
  static struct gdbarch *python_gdbarch;

  struct active_ext_lang_state *m_previous_active;
  PyGILState_STATE m_state;
  struct gdbarch *m_gdbarch;
  const struct language_defn *m_language;

  /* An optional is used here because we don't want to call
     PyErr_Fetch too early.  */
  std::optional<gdbpy_err_fetch> m_error;
};

/* Like gdbpy_enter, but takes a varobj.  This is a subclass just to
   make constructor delegation a little nicer.  */
class gdbpy_enter_varobj : public gdbpy_enter
{
 public:

  /* This is defined in varobj.c, where it can access varobj
     internals.  */
  gdbpy_enter_varobj (const struct varobj *var);

};

/* The opposite of gdb_enter: this releases the GIL around a region,
   allowing other Python threads to run.  No Python APIs may be used
   while this is active.  */
class gdbpy_allow_threads
{
public:

  gdbpy_allow_threads ()
    : m_save (PyEval_SaveThread ())
  {
    gdb_assert (m_save != nullptr);
  }

  ~gdbpy_allow_threads ()
  {
    PyEval_RestoreThread (m_save);
  }

  DISABLE_COPY_AND_ASSIGN (gdbpy_allow_threads);

private:

  PyThreadState *m_save;
};

/* A helper class to save and restore the GIL, but without touching
   the other globals that are handled by gdbpy_enter.  */

class gdbpy_gil
{
public:

  gdbpy_gil ()
    : m_state (PyGILState_Ensure ())
  {
  }

  ~gdbpy_gil ()
  {
    PyGILState_Release (m_state);
  }

  DISABLE_COPY_AND_ASSIGN (gdbpy_gil);

private:

  PyGILState_STATE m_state;
};

/* Use this after a TRY_EXCEPT to throw the appropriate Python
   exception.  */
#define GDB_PY_HANDLE_EXCEPTION(Exception)	\
  do {						\
    if (Exception.reason < 0)			\
      {						\
	gdbpy_convert_exception (Exception);	\
	return NULL;				\
      }						\
  } while (0)

/* Use this after a TRY_EXCEPT to throw the appropriate Python
   exception.  This macro is for use inside setter functions.  */
#define GDB_PY_SET_HANDLE_EXCEPTION(Exception)				\
    do {								\
      if (Exception.reason < 0)						\
	{								\
	  gdbpy_convert_exception (Exception);				\
	  return -1;							\
	}								\
    } while (0)

int gdbpy_print_python_errors_p (void);
void gdbpy_print_stack (void);
void gdbpy_print_stack_or_quit ();
void gdbpy_handle_exception () ATTRIBUTE_NORETURN;

/* A wrapper around calling 'error'.  Prefixes the error message with an
   'Error occurred in Python' string.  Use this in C++ code if we spot
   something wrong with an object returned from Python code.  The prefix
   string gives the user a hint that the mistake is within Python code,
   rather than some other part of GDB.

   This always calls error, and never returns.  */

void gdbpy_error (const char *fmt, ...)
  ATTRIBUTE_NORETURN ATTRIBUTE_PRINTF (1, 2);

gdbpy_ref<> python_string_to_unicode (PyObject *obj);
gdb::unique_xmalloc_ptr<char> unicode_to_target_string (PyObject *unicode_str);
gdb::unique_xmalloc_ptr<char> python_string_to_target_string (PyObject *obj);
gdbpy_ref<> python_string_to_target_python_string (PyObject *obj);
gdb::unique_xmalloc_ptr<char> python_string_to_host_string (PyObject *obj);
gdbpy_ref<> host_string_to_python_string (const char *str);
int gdbpy_is_string (PyObject *obj);
gdb::unique_xmalloc_ptr<char> gdbpy_obj_to_string (PyObject *obj);

int gdbpy_is_lazy_string (PyObject *result);
void gdbpy_extract_lazy_string (PyObject *string, CORE_ADDR *addr,
				struct type **str_type,
				long *length,
				gdb::unique_xmalloc_ptr<char> *encoding);

int gdbpy_is_value_object (PyObject *obj);

/* Note that these are declared here, and not in python.h with the
   other pretty-printer functions, because they refer to PyObject.  */
gdbpy_ref<> apply_varobj_pretty_printer (PyObject *print_obj,
					 struct value **replacement,
					 struct ui_file *stream,
					 const value_print_options *opts);
gdbpy_ref<> gdbpy_get_varobj_pretty_printer (struct value *value);
gdb::unique_xmalloc_ptr<char> gdbpy_get_display_hint (PyObject *printer);
PyObject *gdbpy_default_visualizer (PyObject *self, PyObject *args);

PyObject *gdbpy_print_options (PyObject *self, PyObject *args);
void gdbpy_get_print_options (value_print_options *opts);
extern const struct value_print_options *gdbpy_current_print_options;

void bpfinishpy_pre_stop_hook (struct gdbpy_breakpoint_object *bp_obj);
void bpfinishpy_post_stop_hook (struct gdbpy_breakpoint_object *bp_obj);
void bpfinishpy_pre_delete_hook (struct gdbpy_breakpoint_object *bp_obj);

extern PyObject *gdbpy_doc_cst;
extern PyObject *gdbpy_children_cst;
extern PyObject *gdbpy_to_string_cst;
extern PyObject *gdbpy_display_hint_cst;
extern PyObject *gdbpy_enabled_cst;
extern PyObject *gdbpy_value_cst;

/* Exception types.  */
extern PyObject *gdbpy_gdb_error;
extern PyObject *gdbpy_gdb_memory_error;
extern PyObject *gdbpy_gdberror_exc;

extern void gdbpy_convert_exception (const struct gdb_exception &)
    CPYCHECKER_SETS_EXCEPTION;

int get_addr_from_python (PyObject *obj, CORE_ADDR *addr)
    CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;

gdbpy_ref<> gdb_py_object_from_longest (LONGEST l);
gdbpy_ref<> gdb_py_object_from_ulongest (ULONGEST l);
int gdb_py_int_as_long (PyObject *, long *);

PyObject *gdb_py_generic_dict (PyObject *self, void *closure);

int gdb_pymodule_addobject (PyObject *module, const char *name,
			    PyObject *object)
  CPYCHECKER_NEGATIVE_RESULT_SETS_EXCEPTION;


/* Return a Python string (str) object that represents SELF.  SELF can be
   any object type, but should be in an "invalid" state.  What "invalid"
   means is up to the caller.  The returned string will take the form
   "<TYPENAME (invalid)>", without the quotes, and with TYPENAME replaced
   with the type of SELF.  */

PyObject *gdb_py_invalid_object_repr (PyObject *self);

struct varobj_iter;
struct varobj;
std::unique_ptr<varobj_iter> py_varobj_get_iterator
     (struct varobj *var,
      PyObject *printer,
      const value_print_options *opts);

/* Deleter for Py_buffer unique_ptr specialization.  */

struct Py_buffer_deleter
{
  void operator() (Py_buffer *b) const
  {
    PyBuffer_Release (b);
  }
};

/* A unique_ptr specialization for Py_buffer.  */
typedef std::unique_ptr<Py_buffer, Py_buffer_deleter> Py_buffer_up;

/* Parse a register number from PYO_REG_ID and place the register number
   into *REG_NUM.  The register is a register for GDBARCH.

   If a register is parsed successfully then *REG_NUM will have been
   updated, and true is returned.  Otherwise the contents of *REG_NUM are
   undefined, and false is returned.  When false is returned, the
   Python error is set.

   The PYO_REG_ID object can be a string, the name of the register.  This
   is the slowest approach as GDB has to map the name to a number for each
   call.  Alternatively PYO_REG_ID can be an internal GDB register
   number.  This is quick but should not be encouraged as this means
   Python scripts are now dependent on GDB's internal register numbering.
   Final PYO_REG_ID can be a gdb.RegisterDescriptor object, these objects
   can be looked up by name once, and then cache the register number so
   should be as quick as using a register number.  */

extern bool gdbpy_parse_register_id (struct gdbarch *gdbarch,
				     PyObject *pyo_reg_id, int *reg_num);

/* Return true if OBJ is a gdb.Architecture object, otherwise, return
   false.  */

extern bool gdbpy_is_architecture (PyObject *obj);

/* Return true if OBJ is a gdb.Progspace object, otherwise, return false.  */

extern bool gdbpy_is_progspace (PyObject *obj);

/* Take DOC, the documentation string for a GDB command defined in Python,
   and return an (possibly) modified version of that same string.

   When a command is defined in Python, the documentation string will
   usually be indented based on the indentation of the surrounding Python
   code.  However, the documentation string is a literal string, all the
   white-space added for indentation is included within the documentation
   string.

   This indentation is then included in the help text that GDB displays,
   which looks odd out of the context of the original Python source code.

   This function analyses DOC and tries to figure out what white-space
   within DOC was added as part of the indentation, and then removes that
   white-space from the copy that is returned.

   If the analysis of DOC fails then DOC will be returned unmodified.  */

extern gdb::unique_xmalloc_ptr<char> gdbpy_fix_doc_string_indentation
  (gdb::unique_xmalloc_ptr<char> doc);

/* Implement the 'print_insn' hook for Python.  Disassemble an instruction
   whose address is ADDRESS for architecture GDBARCH.  The bytes of the
   instruction should be read with INFO->read_memory_func as the
   instruction being disassembled might actually be in a buffer.

   Used INFO->fprintf_func to print the results of the disassembly, and
   return the length of the instruction in octets.

   If no instruction can be disassembled then return an empty value.  */

extern std::optional<int> gdbpy_print_insn (struct gdbarch *gdbarch,
					    CORE_ADDR address,
					    disassemble_info *info);

#endif /* PYTHON_PYTHON_INTERNAL_H */
