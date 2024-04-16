/* Copyright (C) 1992-2024 Free Software Foundation, Inc.

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
#include "observable.h"
#include "gdbcmd.h"
#include "target.h"
#include "ada-lang.h"
#include "gdbcore.h"
#include "inferior.h"
#include "gdbthread.h"
#include "progspace.h"
#include "objfiles.h"
#include "cli/cli-style.h"

static int ada_build_task_list ();

/* The name of the array in the GNAT runtime where the Ada Task Control
   Block of each task is stored.  */
#define KNOWN_TASKS_NAME "system__tasking__debug__known_tasks"

/* The maximum number of tasks known to the Ada runtime.  */
static const int MAX_NUMBER_OF_KNOWN_TASKS = 1000;

/* The name of the variable in the GNAT runtime where the head of a task
   chain is saved.  This is an alternate mechanism to find the list of known
   tasks.  */
#define KNOWN_TASKS_LIST "system__tasking__debug__first_task"

enum task_states
{
  Unactivated,
  Runnable,
  Terminated,
  Activator_Sleep,
  Acceptor_Sleep,
  Entry_Caller_Sleep,
  Async_Select_Sleep,
  Delay_Sleep,
  Master_Completion_Sleep,
  Master_Phase_2_Sleep,
  Interrupt_Server_Idle_Sleep,
  Interrupt_Server_Blocked_Interrupt_Sleep,
  Timer_Server_Sleep,
  AST_Server_Sleep,
  Asynchronous_Hold,
  Interrupt_Server_Blocked_On_Event_Flag,
  Activating,
  Acceptor_Delay_Sleep
};

/* A short description corresponding to each possible task state.  */
static const char * const task_states[] = {
  N_("Unactivated"),
  N_("Runnable"),
  N_("Terminated"),
  N_("Child Activation Wait"),
  N_("Accept or Select Term"),
  N_("Waiting on entry call"),
  N_("Async Select Wait"),
  N_("Delay Sleep"),
  N_("Child Termination Wait"),
  N_("Wait Child in Term Alt"),
  "",
  "",
  "",
  "",
  N_("Asynchronous Hold"),
  "",
  N_("Activating"),
  N_("Selective Wait")
};

/* Return a string representing the task state.  */
static const char *
get_state (unsigned value)
{
  if (value >= 0
      && value <= ARRAY_SIZE (task_states)
      && task_states[value][0] != '\0')
    return _(task_states[value]);

  static char buffer[100];
  xsnprintf (buffer, sizeof (buffer), _("Unknown task state: %d"), value);
  return buffer;
}

/* A longer description corresponding to each possible task state.  */
static const char * const long_task_states[] = {
  N_("Unactivated"),
  N_("Runnable"),
  N_("Terminated"),
  N_("Waiting for child activation"),
  N_("Blocked in accept or select with terminate"),
  N_("Waiting on entry call"),
  N_("Asynchronous Selective Wait"),
  N_("Delay Sleep"),
  N_("Waiting for children termination"),
  N_("Waiting for children in terminate alternative"),
  "",
  "",
  "",
  "",
  N_("Asynchronous Hold"),
  "",
  N_("Activating"),
  N_("Blocked in selective wait statement")
};

/* Return a string representing the task state.  This uses the long
   descriptions.  */
static const char *
get_long_state (unsigned value)
{
  if (value >= 0
      && value <= ARRAY_SIZE (long_task_states)
      && long_task_states[value][0] != '\0')
    return _(long_task_states[value]);

  static char buffer[100];
  xsnprintf (buffer, sizeof (buffer), _("Unknown task state: %d"), value);
  return buffer;
}

/* The index of certain important fields in the Ada Task Control Block
   record and sub-records.  */

struct atcb_fieldnos
{
  /* Fields in record Ada_Task_Control_Block.  */
  int common;
  int entry_calls;
  int atc_nesting_level;

  /* Fields in record Common_ATCB.  */
  int state;
  int parent;
  int priority;
  int image;
  int image_len;     /* This field may be missing.  */
  int activation_link;
  int call;
  int ll;
  int base_cpu;

  /* Fields in Task_Primitives.Private_Data.  */
  int ll_thread;
  int ll_lwp;        /* This field may be missing.  */

  /* Fields in Common_ATCB.Call.all.  */
  int call_self;
};

/* This module's per-program-space data.  */

struct ada_tasks_pspace_data
{
  /* Nonzero if the data has been initialized.  If set to zero,
     it means that the data has either not been initialized, or
     has potentially become stale.  */
  int initialized_p = 0;

  /* The ATCB record type.  */
  struct type *atcb_type = nullptr;

  /* The ATCB "Common" component type.  */
  struct type *atcb_common_type = nullptr;

  /* The type of the "ll" field, from the atcb_common_type.  */
  struct type *atcb_ll_type = nullptr;

  /* The type of the "call" field, from the atcb_common_type.  */
  struct type *atcb_call_type = nullptr;

  /* The index of various fields in the ATCB record and sub-records.  */
  struct atcb_fieldnos atcb_fieldno {};

  /* On some systems, gdbserver applies an offset to the CPU that is
     reported.  */
  unsigned int cpu_id_offset = 0;
};

/* Key to our per-program-space data.  */
static const registry<program_space>::key<ada_tasks_pspace_data>
  ada_tasks_pspace_data_handle;

/* The kind of data structure used by the runtime to store the list
   of Ada tasks.  */

enum ada_known_tasks_kind
{
  /* Use this value when we haven't determined which kind of structure
     is being used, or when we need to recompute it.

     We set the value of this enumerate to zero on purpose: This allows
     us to use this enumerate in a structure where setting all fields
     to zero will result in this kind being set to unknown.  */
  ADA_TASKS_UNKNOWN = 0,

  /* This value means that we did not find any task list.  Unless
     there is a bug somewhere, this means that the inferior does not
     use tasking.  */
  ADA_TASKS_NOT_FOUND,

  /* This value means that the task list is stored as an array.
     This is the usual method, as it causes very little overhead.
     But this method is not always used, as it does use a certain
     amount of memory, which might be scarse in certain environments.  */
  ADA_TASKS_ARRAY,

  /* This value means that the task list is stored as a linked list.
     This has more runtime overhead than the array approach, but
     also require less memory when the number of tasks is small.  */
  ADA_TASKS_LIST,
};

/* This module's per-inferior data.  */

struct ada_tasks_inferior_data
{
  /* The type of data structure used by the runtime to store
     the list of Ada tasks.  The value of this field influences
     the interpretation of the known_tasks_addr field below:
       - ADA_TASKS_UNKNOWN: The value of known_tasks_addr hasn't
	 been determined yet;
       - ADA_TASKS_NOT_FOUND: The program probably does not use tasking
	 and the known_tasks_addr is irrelevant;
       - ADA_TASKS_ARRAY: The known_tasks is an array;
       - ADA_TASKS_LIST: The known_tasks is a list.  */
  enum ada_known_tasks_kind known_tasks_kind = ADA_TASKS_UNKNOWN;

  /* The address of the known_tasks structure.  This is where
     the runtime stores the information for all Ada tasks.
     The interpretation of this field depends on KNOWN_TASKS_KIND
     above.  */
  CORE_ADDR known_tasks_addr = 0;

  /* Type of elements of the known task.  Usually a pointer.  */
  struct type *known_tasks_element = nullptr;

  /* Number of elements in the known tasks array.  */
  unsigned int known_tasks_length = 0;

  /* When nonzero, this flag indicates that the task_list field
     below is up to date.  When set to zero, the list has either
     not been initialized, or has potentially become stale.  */
  bool task_list_valid_p = false;

  /* The list of Ada tasks.

     Note: To each task we associate a number that the user can use to
     reference it - this number is printed beside each task in the tasks
     info listing displayed by "info tasks".  This number is equal to
     its index in the vector + 1.  Reciprocally, to compute the index
     of a task in the vector, we need to substract 1 from its number.  */
  std::vector<ada_task_info> task_list;
};

/* Key to our per-inferior data.  */
static const registry<inferior>::key<ada_tasks_inferior_data>
  ada_tasks_inferior_data_handle;

/* Return a string with TASKNO followed by the task name if TASK_INFO
   contains a name.  */

static std::string
task_to_str (int taskno, const ada_task_info *task_info)
{
  if (task_info->name[0] == '\0')
    return string_printf ("%d", taskno);
  else
    return string_printf ("%d \"%s\"", taskno, task_info->name);
}

/* Return the ada-tasks module's data for the given program space (PSPACE).
   If none is found, add a zero'ed one now.

   This function always returns a valid object.  */

static struct ada_tasks_pspace_data *
get_ada_tasks_pspace_data (struct program_space *pspace)
{
  struct ada_tasks_pspace_data *data;

  data = ada_tasks_pspace_data_handle.get (pspace);
  if (data == NULL)
    data = ada_tasks_pspace_data_handle.emplace (pspace);

  return data;
}

/* Return the ada-tasks module's data for the given inferior (INF).
   If none is found, add a zero'ed one now.

   This function always returns a valid object.

   Note that we could use an observer of the inferior-created event
   to make sure that the ada-tasks per-inferior data always exists.
   But we preferred this approach, as it avoids this entirely as long
   as the user does not use any of the tasking features.  This is
   quite possible, particularly in the case where the inferior does
   not use tasking.  */

static struct ada_tasks_inferior_data *
get_ada_tasks_inferior_data (struct inferior *inf)
{
  struct ada_tasks_inferior_data *data;

  data = ada_tasks_inferior_data_handle.get (inf);
  if (data == NULL)
    data = ada_tasks_inferior_data_handle.emplace (inf);

  return data;
}

/* Return the task number of the task whose thread is THREAD, or zero
   if the task could not be found.  */

int
ada_get_task_number (thread_info *thread)
{
  struct inferior *inf = thread->inf;
  struct ada_tasks_inferior_data *data;

  gdb_assert (inf != NULL);
  data = get_ada_tasks_inferior_data (inf);

  for (int i = 0; i < data->task_list.size (); i++)
    if (data->task_list[i].ptid == thread->ptid)
      return i + 1;

  return 0;  /* No matching task found.  */
}

/* Return the task number of the task running in inferior INF which
   matches TASK_ID , or zero if the task could not be found.  */
 
static int
get_task_number_from_id (CORE_ADDR task_id, struct inferior *inf)
{
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  for (int i = 0; i < data->task_list.size (); i++)
    {
      if (data->task_list[i].task_id == task_id)
	return i + 1;
    }

  /* Task not found.  Return 0.  */
  return 0;
}

/* Return non-zero if TASK_NUM is a valid task number.  */

int
valid_task_id (int task_num)
{
  struct ada_tasks_inferior_data *data;

  ada_build_task_list ();
  data = get_ada_tasks_inferior_data (current_inferior ());
  return task_num > 0 && task_num <= data->task_list.size ();
}

/* Return non-zero iff the task STATE corresponds to a non-terminated
   task state.  */

static int
ada_task_is_alive (const struct ada_task_info *task_info)
{
  return (task_info->state != Terminated);
}

/* Search through the list of known tasks for the one whose ptid is
   PTID, and return it.  Return NULL if the task was not found.  */

struct ada_task_info *
ada_get_task_info_from_ptid (ptid_t ptid)
{
  struct ada_tasks_inferior_data *data;

  ada_build_task_list ();
  data = get_ada_tasks_inferior_data (current_inferior ());

  for (ada_task_info &task : data->task_list)
    {
      if (task.ptid == ptid)
	return &task;
    }

  return NULL;
}

/* Call the ITERATOR function once for each Ada task that hasn't been
   terminated yet.  */

void
iterate_over_live_ada_tasks (ada_task_list_iterator_ftype iterator)
{
  struct ada_tasks_inferior_data *data;

  ada_build_task_list ();
  data = get_ada_tasks_inferior_data (current_inferior ());

  for (ada_task_info &task : data->task_list)
    {
      if (!ada_task_is_alive (&task))
	continue;
      iterator (&task);
    }
}

/* Extract the contents of the value as a string whose length is LENGTH,
   and store the result in DEST.  */

static void
value_as_string (char *dest, struct value *val, int length)
{
  memcpy (dest, val->contents ().data (), length);
  dest[length] = '\0';
}

/* Extract the string image from the fat string corresponding to VAL,
   and store it in DEST.  If the string length is greater than MAX_LEN,
   then truncate the result to the first MAX_LEN characters of the fat
   string.  */

static void
read_fat_string_value (char *dest, struct value *val, int max_len)
{
  struct value *array_val;
  struct value *bounds_val;
  int len;

  /* The following variables are made static to avoid recomputing them
     each time this function is called.  */
  static int initialize_fieldnos = 1;
  static int array_fieldno;
  static int bounds_fieldno;
  static int upper_bound_fieldno;

  /* Get the index of the fields that we will need to read in order
     to extract the string from the fat string.  */
  if (initialize_fieldnos)
    {
      struct type *type = val->type ();
      struct type *bounds_type;

      array_fieldno = ada_get_field_index (type, "P_ARRAY", 0);
      bounds_fieldno = ada_get_field_index (type, "P_BOUNDS", 0);

      bounds_type = type->field (bounds_fieldno).type ();
      if (bounds_type->code () == TYPE_CODE_PTR)
	bounds_type = bounds_type->target_type ();
      if (bounds_type->code () != TYPE_CODE_STRUCT)
	error (_("Unknown task name format. Aborting"));
      upper_bound_fieldno = ada_get_field_index (bounds_type, "UB0", 0);

      initialize_fieldnos = 0;
    }

  /* Get the size of the task image by checking the value of the bounds.
     The lower bound is always 1, so we only need to read the upper bound.  */
  bounds_val = value_ind (value_field (val, bounds_fieldno));
  len = value_as_long (value_field (bounds_val, upper_bound_fieldno));

  /* Make sure that we do not read more than max_len characters...  */
  if (len > max_len)
    len = max_len;

  /* Extract LEN characters from the fat string.  */
  array_val = value_ind (value_field (val, array_fieldno));
  read_memory (array_val->address (), (gdb_byte *) dest, len);

  /* Add the NUL character to close the string.  */
  dest[len] = '\0';
}

/* Get, from the debugging information, the type description of all types
   related to the Ada Task Control Block that are needed in order to
   read the list of known tasks in the Ada runtime.  If all of the info
   needed to do so is found, then save that info in the module's per-
   program-space data, and return NULL.  Otherwise, if any information
   cannot be found, leave the per-program-space data untouched, and
   return an error message explaining what was missing (that error
   message does NOT need to be deallocated).  */

const char *
ada_get_tcb_types_info (void)
{
  struct type *type;
  struct type *common_type;
  struct type *ll_type;
  struct type *call_type;
  struct atcb_fieldnos fieldnos;
  struct ada_tasks_pspace_data *pspace_data;

  const char *atcb_name = "system__tasking__ada_task_control_block___XVE";
  const char *atcb_name_fixed = "system__tasking__ada_task_control_block";
  const char *common_atcb_name = "system__tasking__common_atcb";
  const char *private_data_name = "system__task_primitives__private_data";
  const char *entry_call_record_name = "system__tasking__entry_call_record";

  /* ATCB symbols may be found in several compilation units.  As we
     are only interested in one instance, use standard (literal,
     C-like) lookups to get the first match.  */

  struct symbol *atcb_sym =
    lookup_symbol_in_language (atcb_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;
  const struct symbol *common_atcb_sym =
    lookup_symbol_in_language (common_atcb_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;
  const struct symbol *private_data_sym =
    lookup_symbol_in_language (private_data_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;
  const struct symbol *entry_call_record_sym =
    lookup_symbol_in_language (entry_call_record_name, NULL, STRUCT_DOMAIN,
			       language_c, NULL).symbol;

  if (atcb_sym == NULL || atcb_sym->type () == NULL)
    {
      /* In Ravenscar run-time libs, the  ATCB does not have a dynamic
	 size, so the symbol name differs.  */
      atcb_sym = lookup_symbol_in_language (atcb_name_fixed, NULL,
					    STRUCT_DOMAIN, language_c,
					    NULL).symbol;

      if (atcb_sym == NULL || atcb_sym->type () == NULL)
	return _("Cannot find Ada_Task_Control_Block type");

      type = atcb_sym->type ();
    }
  else
    {
      /* Get a static representation of the type record
	 Ada_Task_Control_Block.  */
      type = atcb_sym->type ();
      type = ada_template_to_fixed_record_type_1 (type, NULL, 0, NULL, 0);
    }

  if (common_atcb_sym == NULL || common_atcb_sym->type () == NULL)
    return _("Cannot find Common_ATCB type");
  if (private_data_sym == NULL || private_data_sym->type ()== NULL)
    return _("Cannot find Private_Data type");
  if (entry_call_record_sym == NULL || entry_call_record_sym->type () == NULL)
    return _("Cannot find Entry_Call_Record type");

  /* Get the type for Ada_Task_Control_Block.Common.  */
  common_type = common_atcb_sym->type ();

  /* Get the type for Ada_Task_Control_Bloc.Common.Call.LL.  */
  ll_type = private_data_sym->type ();

  /* Get the type for Common_ATCB.Call.all.  */
  call_type = entry_call_record_sym->type ();

  /* Get the field indices.  */
  fieldnos.common = ada_get_field_index (type, "common", 0);
  fieldnos.entry_calls = ada_get_field_index (type, "entry_calls", 1);
  fieldnos.atc_nesting_level =
    ada_get_field_index (type, "atc_nesting_level", 1);
  fieldnos.state = ada_get_field_index (common_type, "state", 0);
  fieldnos.parent = ada_get_field_index (common_type, "parent", 1);
  fieldnos.priority = ada_get_field_index (common_type, "base_priority", 0);
  fieldnos.image = ada_get_field_index (common_type, "task_image", 1);
  fieldnos.image_len = ada_get_field_index (common_type, "task_image_len", 1);
  fieldnos.activation_link = ada_get_field_index (common_type,
						  "activation_link", 1);
  fieldnos.call = ada_get_field_index (common_type, "call", 1);
  fieldnos.ll = ada_get_field_index (common_type, "ll", 0);
  fieldnos.base_cpu = ada_get_field_index (common_type, "base_cpu", 0);
  fieldnos.ll_thread = ada_get_field_index (ll_type, "thread", 0);
  fieldnos.ll_lwp = ada_get_field_index (ll_type, "lwp", 1);
  fieldnos.call_self = ada_get_field_index (call_type, "self", 0);

  /* On certain platforms such as x86-windows, the "lwp" field has been
     named "thread_id".  This field will likely be renamed in the future,
     but we need to support both possibilities to avoid an unnecessary
     dependency on a recent compiler.  We therefore try locating the
     "thread_id" field in place of the "lwp" field if we did not find
     the latter.  */
  if (fieldnos.ll_lwp < 0)
    fieldnos.ll_lwp = ada_get_field_index (ll_type, "thread_id", 1);

  /* Check for the CPU offset.  */
  bound_minimal_symbol first_id_sym
    = lookup_bound_minimal_symbol ("__gnat_gdb_cpu_first_id");
  unsigned int first_id = 0;
  if (first_id_sym.minsym != nullptr)
    {
      CORE_ADDR addr = first_id_sym.value_address ();
      gdbarch *arch = current_inferior ()->arch ();
      /* This symbol always has type uint32_t.  */
      struct type *u32type = builtin_type (arch)->builtin_uint32;
      first_id = value_as_long (value_at (u32type, addr));
    }

  /* Set all the out parameters all at once, now that we are certain
     that there are no potential error() anymore.  */
  pspace_data = get_ada_tasks_pspace_data (current_program_space);
  pspace_data->initialized_p = 1;
  pspace_data->atcb_type = type;
  pspace_data->atcb_common_type = common_type;
  pspace_data->atcb_ll_type = ll_type;
  pspace_data->atcb_call_type = call_type;
  pspace_data->atcb_fieldno = fieldnos;
  pspace_data->cpu_id_offset = first_id;
  return NULL;
}

/* Build the PTID of the task from its COMMON_VALUE, which is the "Common"
   component of its ATCB record.  This PTID needs to match the PTID used
   by the thread layer.  */

static ptid_t
ptid_from_atcb_common (struct value *common_value)
{
  ULONGEST thread;
  CORE_ADDR lwp = 0;
  struct value *ll_value;
  ptid_t ptid;
  const struct ada_tasks_pspace_data *pspace_data
    = get_ada_tasks_pspace_data (current_program_space);

  ll_value = value_field (common_value, pspace_data->atcb_fieldno.ll);

  if (pspace_data->atcb_fieldno.ll_lwp >= 0)
    lwp = value_as_address (value_field (ll_value,
					 pspace_data->atcb_fieldno.ll_lwp));
  thread = value_as_long (value_field (ll_value,
				       pspace_data->atcb_fieldno.ll_thread));

  ptid = target_get_ada_task_ptid (lwp, thread);

  return ptid;
}

/* Read the ATCB data of a given task given its TASK_ID (which is in practice
   the address of its associated ATCB record), and store the result inside
   TASK_INFO.  */

static void
read_atcb (CORE_ADDR task_id, struct ada_task_info *task_info)
{
  struct value *tcb_value;
  struct value *common_value;
  struct value *atc_nesting_level_value;
  struct value *entry_calls_value;
  struct value *entry_calls_value_element;
  int called_task_fieldno = -1;
  static const char ravenscar_task_name[] = "Ravenscar task";
  const struct ada_tasks_pspace_data *pspace_data
    = get_ada_tasks_pspace_data (current_program_space);

  /* Clear the whole structure to start with, so that everything
     is always initialized the same.  */
  memset (task_info, 0, sizeof (struct ada_task_info));

  if (!pspace_data->initialized_p)
    {
      const char *err_msg = ada_get_tcb_types_info ();

      if (err_msg != NULL)
	error (_("%s. Aborting"), err_msg);
    }

  tcb_value = value_from_contents_and_address (pspace_data->atcb_type,
					       NULL, task_id);
  common_value = value_field (tcb_value, pspace_data->atcb_fieldno.common);

  /* Fill in the task_id.  */

  task_info->task_id = task_id;

  /* Compute the name of the task.

     Depending on the GNAT version used, the task image is either a fat
     string, or a thin array of characters.  Older versions of GNAT used
     to use fat strings, and therefore did not need an extra field in
     the ATCB to store the string length.  For efficiency reasons, newer
     versions of GNAT replaced the fat string by a static buffer, but this
     also required the addition of a new field named "Image_Len" containing
     the length of the task name.  The method used to extract the task name
     is selected depending on the existence of this field.

     In some run-time libs (e.g. Ravenscar), the name is not in the ATCB;
     we may want to get it from the first user frame of the stack.  For now,
     we just give a dummy name.  */

  if (pspace_data->atcb_fieldno.image_len == -1)
    {
      if (pspace_data->atcb_fieldno.image >= 0)
	read_fat_string_value (task_info->name,
			       value_field (common_value,
					    pspace_data->atcb_fieldno.image),
			       sizeof (task_info->name) - 1);
      else
	{
	  struct bound_minimal_symbol msym;

	  msym = lookup_minimal_symbol_by_pc (task_id);
	  if (msym.minsym)
	    {
	      const char *full_name = msym.minsym->linkage_name ();
	      const char *task_name = full_name;
	      const char *p;

	      /* Strip the prefix.  */
	      for (p = full_name; *p; p++)
		if (p[0] == '_' && p[1] == '_')
		  task_name = p + 2;

	      /* Copy the task name.  */
	      strncpy (task_info->name, task_name,
		       sizeof (task_info->name) - 1);
	      task_info->name[sizeof (task_info->name) - 1] = 0;
	    }
	  else
	    {
	      /* No symbol found.  Use a default name.  */
	      strcpy (task_info->name, ravenscar_task_name);
	    }
	}
    }
  else
    {
      int len = value_as_long
		  (value_field (common_value,
				pspace_data->atcb_fieldno.image_len));

      value_as_string (task_info->name,
		       value_field (common_value,
				    pspace_data->atcb_fieldno.image),
		       len);
    }

  /* Compute the task state and priority.  */

  task_info->state =
    value_as_long (value_field (common_value,
				pspace_data->atcb_fieldno.state));
  task_info->priority =
    value_as_long (value_field (common_value,
				pspace_data->atcb_fieldno.priority));

  /* If the ATCB contains some information about the parent task,
     then compute it as well.  Otherwise, zero.  */

  if (pspace_data->atcb_fieldno.parent >= 0)
    task_info->parent =
      value_as_address (value_field (common_value,
				     pspace_data->atcb_fieldno.parent));

  /* If the task is in an entry call waiting for another task,
     then determine which task it is.  */

  if (task_info->state == Entry_Caller_Sleep
      && pspace_data->atcb_fieldno.atc_nesting_level > 0
      && pspace_data->atcb_fieldno.entry_calls > 0)
    {
      /* Let My_ATCB be the Ada task control block of a task calling the
	 entry of another task; then the Task_Id of the called task is
	 in My_ATCB.Entry_Calls (My_ATCB.ATC_Nesting_Level).Called_Task.  */
      atc_nesting_level_value =
	value_field (tcb_value, pspace_data->atcb_fieldno.atc_nesting_level);
      entry_calls_value =
	ada_coerce_to_simple_array_ptr
	  (value_field (tcb_value, pspace_data->atcb_fieldno.entry_calls));
      entry_calls_value_element =
	value_subscript (entry_calls_value,
			 value_as_long (atc_nesting_level_value));
      called_task_fieldno =
	ada_get_field_index (entry_calls_value_element->type (),
			     "called_task", 0);
      task_info->called_task =
	value_as_address (value_field (entry_calls_value_element,
				       called_task_fieldno));
    }

  /* If the ATCB contains some information about RV callers, then
     compute the "caller_task".  Otherwise, leave it as zero.  */

  if (pspace_data->atcb_fieldno.call >= 0)
    {
      /* Get the ID of the caller task from Common_ATCB.Call.all.Self.
	 If Common_ATCB.Call is null, then there is no caller.  */
      const CORE_ADDR call =
	value_as_address (value_field (common_value,
				       pspace_data->atcb_fieldno.call));
      struct value *call_val;

      if (call != 0)
	{
	  call_val =
	    value_from_contents_and_address (pspace_data->atcb_call_type,
					     NULL, call);
	  task_info->caller_task =
	    value_as_address
	      (value_field (call_val, pspace_data->atcb_fieldno.call_self));
	}
    }

  task_info->base_cpu
    = (pspace_data->cpu_id_offset
      + value_as_long (value_field (common_value,
				    pspace_data->atcb_fieldno.base_cpu)));

  /* And finally, compute the task ptid.  Note that there is not point
     in computing it if the task is no longer alive, in which case
     it is good enough to set its ptid to the null_ptid.  */
  if (ada_task_is_alive (task_info))
    task_info->ptid = ptid_from_atcb_common (common_value);
  else
    task_info->ptid = null_ptid;
}

/* Read the ATCB info of the given task (identified by TASK_ID), and
   add the result to the given inferior's TASK_LIST.  */

static void
add_ada_task (CORE_ADDR task_id, struct inferior *inf)
{
  struct ada_task_info task_info;
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  read_atcb (task_id, &task_info);
  data->task_list.push_back (task_info);
}

/* Read the Known_Tasks array from the inferior memory, and store
   it in the current inferior's TASK_LIST.  Return true upon success.  */

static bool
read_known_tasks_array (struct ada_tasks_inferior_data *data)
{
  const int target_ptr_byte = data->known_tasks_element->length ();
  const int known_tasks_size = target_ptr_byte * data->known_tasks_length;
  gdb_byte *known_tasks = (gdb_byte *) alloca (known_tasks_size);
  int i;

  /* Build a new list by reading the ATCBs from the Known_Tasks array
     in the Ada runtime.  */
  read_memory (data->known_tasks_addr, known_tasks, known_tasks_size);
  for (i = 0; i < data->known_tasks_length; i++)
    {
      CORE_ADDR task_id =
	extract_typed_address (known_tasks + i * target_ptr_byte,
			       data->known_tasks_element);

      if (task_id != 0)
	add_ada_task (task_id, current_inferior ());
    }

  return true;
}

/* Read the known tasks from the inferior memory, and store it in
   the current inferior's TASK_LIST.  Return true upon success.  */

static bool
read_known_tasks_list (struct ada_tasks_inferior_data *data)
{
  const int target_ptr_byte = data->known_tasks_element->length ();
  gdb_byte *known_tasks = (gdb_byte *) alloca (target_ptr_byte);
  CORE_ADDR task_id;
  const struct ada_tasks_pspace_data *pspace_data
    = get_ada_tasks_pspace_data (current_program_space);

  /* Sanity check.  */
  if (pspace_data->atcb_fieldno.activation_link < 0)
    return false;

  /* Build a new list by reading the ATCBs.  Read head of the list.  */
  read_memory (data->known_tasks_addr, known_tasks, target_ptr_byte);
  task_id = extract_typed_address (known_tasks, data->known_tasks_element);
  while (task_id != 0)
    {
      struct value *tcb_value;
      struct value *common_value;

      add_ada_task (task_id, current_inferior ());

      /* Read the chain.  */
      tcb_value = value_from_contents_and_address (pspace_data->atcb_type,
						   NULL, task_id);
      common_value = value_field (tcb_value, pspace_data->atcb_fieldno.common);
      task_id = value_as_address
		  (value_field (common_value,
				pspace_data->atcb_fieldno.activation_link));
    }

  return true;
}

/* Set all fields of the current inferior ada-tasks data pointed by DATA.
   Do nothing if those fields are already set and still up to date.  */

static void
ada_tasks_inferior_data_sniffer (struct ada_tasks_inferior_data *data)
{
  struct bound_minimal_symbol msym;
  struct symbol *sym;

  /* Return now if already set.  */
  if (data->known_tasks_kind != ADA_TASKS_UNKNOWN)
    return;

  /* Try array.  */

  msym = lookup_minimal_symbol (KNOWN_TASKS_NAME, NULL, NULL);
  if (msym.minsym != NULL)
    {
      data->known_tasks_kind = ADA_TASKS_ARRAY;
      data->known_tasks_addr = msym.value_address ();

      /* Try to get pointer type and array length from the symtab.  */
      sym = lookup_symbol_in_language (KNOWN_TASKS_NAME, NULL, VAR_DOMAIN,
				       language_c, NULL).symbol;
      if (sym != NULL)
	{
	  /* Validate.  */
	  struct type *type = check_typedef (sym->type ());
	  struct type *eltype = NULL;
	  struct type *idxtype = NULL;

	  if (type->code () == TYPE_CODE_ARRAY)
	    eltype = check_typedef (type->target_type ());
	  if (eltype != NULL
	      && eltype->code () == TYPE_CODE_PTR)
	    idxtype = check_typedef (type->index_type ());
	  if (idxtype != NULL
	      && idxtype->bounds ()->low.is_constant ()
	      && idxtype->bounds ()->high.is_constant ())
	    {
	      data->known_tasks_element = eltype;
	      data->known_tasks_length =
		(idxtype->bounds ()->high.const_val ()
		 - idxtype->bounds ()->low.const_val () + 1);
	      return;
	    }
	}

      /* Fallback to default values.  The runtime may have been stripped (as
	 in some distributions), but it is likely that the executable still
	 contains debug information on the task type (due to implicit with of
	 Ada.Tasking).  */
      data->known_tasks_element =
	builtin_type (current_inferior ()->arch ())->builtin_data_ptr;
      data->known_tasks_length = MAX_NUMBER_OF_KNOWN_TASKS;
      return;
    }


  /* Try list.  */

  msym = lookup_minimal_symbol (KNOWN_TASKS_LIST, NULL, NULL);
  if (msym.minsym != NULL)
    {
      data->known_tasks_kind = ADA_TASKS_LIST;
      data->known_tasks_addr = msym.value_address ();
      data->known_tasks_length = 1;

      sym = lookup_symbol_in_language (KNOWN_TASKS_LIST, NULL, VAR_DOMAIN,
				       language_c, NULL).symbol;
      if (sym != NULL && sym->value_address () != 0)
	{
	  /* Validate.  */
	  struct type *type = check_typedef (sym->type ());

	  if (type->code () == TYPE_CODE_PTR)
	    {
	      data->known_tasks_element = type;
	      return;
	    }
	}

      /* Fallback to default values.  */
      data->known_tasks_element =
	builtin_type (current_inferior ()->arch ())->builtin_data_ptr;
      data->known_tasks_length = 1;
      return;
    }

  /* Can't find tasks.  */

  data->known_tasks_kind = ADA_TASKS_NOT_FOUND;
  data->known_tasks_addr = 0;
}

/* Read the known tasks from the current inferior's memory, and store it
   in the current inferior's data TASK_LIST.  */

static void
read_known_tasks ()
{
  struct ada_tasks_inferior_data *data =
    get_ada_tasks_inferior_data (current_inferior ());

  /* Step 1: Clear the current list, if necessary.  */
  data->task_list.clear ();

  /* Step 2: do the real work.
     If the application does not use task, then no more needs to be done.
     It is important to have the task list cleared (see above) before we
     return, as we don't want a stale task list to be used...  This can
     happen for instance when debugging a non-multitasking program after
     having debugged a multitasking one.  */
  ada_tasks_inferior_data_sniffer (data);
  gdb_assert (data->known_tasks_kind != ADA_TASKS_UNKNOWN);

  /* Step 3: Set task_list_valid_p, to avoid re-reading the Known_Tasks
     array unless needed.  */
  switch (data->known_tasks_kind)
    {
    case ADA_TASKS_NOT_FOUND: /* Tasking not in use in inferior.  */
      break;
    case ADA_TASKS_ARRAY:
      data->task_list_valid_p = read_known_tasks_array (data);
      break;
    case ADA_TASKS_LIST:
      data->task_list_valid_p = read_known_tasks_list (data);
      break;
    }
}

/* Build the task_list by reading the Known_Tasks array from
   the inferior, and return the number of tasks in that list
   (zero means that the program is not using tasking at all).  */

static int
ada_build_task_list ()
{
  struct ada_tasks_inferior_data *data;

  if (!target_has_stack ())
    error (_("Cannot inspect Ada tasks when program is not running"));

  data = get_ada_tasks_inferior_data (current_inferior ());
  if (!data->task_list_valid_p)
    read_known_tasks ();

  return data->task_list.size ();
}

/* Print a table providing a short description of all Ada tasks
   running inside inferior INF.  If ARG_STR is set, it will be
   interpreted as a task number, and the table will be limited to
   that task only.  */

void
print_ada_task_info (struct ui_out *uiout,
		     const char *arg_str,
		     struct inferior *inf)
{
  struct ada_tasks_inferior_data *data;
  int taskno, nb_tasks;
  int taskno_arg = 0;
  int nb_columns;

  if (ada_build_task_list () == 0)
    {
      uiout->message (_("Your application does not use any Ada tasks.\n"));
      return;
    }

  if (arg_str != NULL && arg_str[0] != '\0')
    taskno_arg = value_as_long (parse_and_eval (arg_str));

  if (uiout->is_mi_like_p ())
    /* In GDB/MI mode, we want to provide the thread ID corresponding
       to each task.  This allows clients to quickly find the thread
       associated to any task, which is helpful for commands that
       take a --thread argument.  However, in order to be able to
       provide that thread ID, the thread list must be up to date
       first.  */
    target_update_thread_list ();

  data = get_ada_tasks_inferior_data (inf);

  /* Compute the number of tasks that are going to be displayed
     in the output.  If an argument was given, there will be
     at most 1 entry.  Otherwise, there will be as many entries
     as we have tasks.  */
  if (taskno_arg)
    {
      if (taskno_arg > 0 && taskno_arg <= data->task_list.size ())
	nb_tasks = 1;
      else
	nb_tasks = 0;
    }
  else
    nb_tasks = data->task_list.size ();

  nb_columns = uiout->is_mi_like_p () ? 8 : 7;
  ui_out_emit_table table_emitter (uiout, nb_columns, nb_tasks, "tasks");
  uiout->table_header (1, ui_left, "current", "");
  uiout->table_header (3, ui_right, "id", "ID");
  {
    size_t tid_width = 9;
    /* Grown below in case the largest entry is bigger.  */

    if (!uiout->is_mi_like_p ())
      {
	for (taskno = 1; taskno <= data->task_list.size (); taskno++)
	  {
	    const struct ada_task_info *const task_info
	      = &data->task_list[taskno - 1];

	    gdb_assert (task_info != NULL);

	    tid_width = std::max (tid_width,
				  1 + strlen (phex_nz (task_info->task_id,
						       sizeof (CORE_ADDR))));
	  }
      }
    uiout->table_header (tid_width, ui_right, "task-id", "TID");
  }
  /* The following column is provided in GDB/MI mode only because
     it is only really useful in that mode, and also because it
     allows us to keep the CLI output shorter and more compact.  */
  if (uiout->is_mi_like_p ())
    uiout->table_header (4, ui_right, "thread-id", "");
  uiout->table_header (4, ui_right, "parent-id", "P-ID");
  uiout->table_header (3, ui_right, "priority", "Pri");
  uiout->table_header (22, ui_left, "state", "State");
  /* Use ui_noalign for the last column, to prevent the CLI uiout
     from printing an extra space at the end of each row.  This
     is a bit of a hack, but does get the job done.  */
  uiout->table_header (1, ui_noalign, "name", "Name");
  uiout->table_body ();

  for (taskno = 1; taskno <= data->task_list.size (); taskno++)
    {
      const struct ada_task_info *const task_info =
	&data->task_list[taskno - 1];
      int parent_id;

      gdb_assert (task_info != NULL);

      /* If the user asked for the output to be restricted
	 to one task only, and this is not the task, skip
	 to the next one.  */
      if (taskno_arg && taskno != taskno_arg)
	continue;

      ui_out_emit_tuple tuple_emitter (uiout, NULL);

      /* Print a star if this task is the current task (or the task
	 currently selected).  */
      if (task_info->ptid == inferior_ptid)
	uiout->field_string ("current", "*");
      else
	uiout->field_skip ("current");

      /* Print the task number.  */
      uiout->field_signed ("id", taskno);

      /* Print the Task ID.  */
      uiout->field_string ("task-id", phex_nz (task_info->task_id,
					       sizeof (CORE_ADDR)));

      /* Print the associated Thread ID.  */
      if (uiout->is_mi_like_p ())
	{
	  thread_info *thread = (ada_task_is_alive (task_info)
				 ? inf->find_thread (task_info->ptid)
				 : nullptr);

	  if (thread != NULL)
	    uiout->field_signed ("thread-id", thread->global_num);
	  else
	    {
	      /* This can happen if the thread is no longer alive.  */
	      uiout->field_skip ("thread-id");
	    }
	}

      /* Print the ID of the parent task.  */
      parent_id = get_task_number_from_id (task_info->parent, inf);
      if (parent_id)
	uiout->field_signed ("parent-id", parent_id);
      else
	uiout->field_skip ("parent-id");

      /* Print the base priority of the task.  */
      uiout->field_signed ("priority", task_info->priority);

      /* Print the task current state.  */
      if (task_info->caller_task)
	uiout->field_fmt ("state",
			  _("Accepting RV with %-4d"),
			  get_task_number_from_id (task_info->caller_task,
						   inf));
      else if (task_info->called_task)
	uiout->field_fmt ("state",
			  _("Waiting on RV with %-3d"),
			  get_task_number_from_id (task_info->called_task,
						   inf));
      else
	uiout->field_string ("state", get_state (task_info->state));

      /* Finally, print the task name, without quotes around it, as mi like
	 is not expecting quotes, and in non mi-like no need for quotes
	 as there is a specific column for the name.  */
      uiout->field_fmt ("name",
			(task_info->name[0] != '\0'
			 ? ui_file_style ()
			 : metadata_style.style ()),
			"%s",
			(task_info->name[0] != '\0'
			 ? task_info->name
			 : _("<no name>")));

      uiout->text ("\n");
    }
}

/* Print a detailed description of the Ada task whose ID is TASKNO_STR
   for the given inferior (INF).  */

static void
info_task (struct ui_out *uiout, const char *taskno_str, struct inferior *inf)
{
  const int taskno = value_as_long (parse_and_eval (taskno_str));
  struct ada_task_info *task_info;
  int parent_taskno = 0;
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  if (ada_build_task_list () == 0)
    {
      uiout->message (_("Your application does not use any Ada tasks.\n"));
      return;
    }

  if (taskno <= 0 || taskno > data->task_list.size ())
    error (_("Task ID %d not known.  Use the \"info tasks\" command to\n"
	     "see the IDs of currently known tasks"), taskno);
  task_info = &data->task_list[taskno - 1];

  /* Print the Ada task ID.  */
  gdb_printf (_("Ada Task: %s\n"),
	      paddress (current_inferior ()->arch (), task_info->task_id));

  /* Print the name of the task.  */
  if (task_info->name[0] != '\0')
    gdb_printf (_("Name: %s\n"), task_info->name);
  else
    fprintf_styled (gdb_stdout, metadata_style.style (), _("<no name>\n"));

  /* Print the TID and LWP.  */
  gdb_printf (_("Thread: 0x%s\n"), phex_nz (task_info->ptid.tid (),
					    sizeof (ULONGEST)));
  gdb_printf (_("LWP: %#lx\n"), task_info->ptid.lwp ());

  /* If set, print the base CPU.  */
  if (task_info->base_cpu != 0)
    gdb_printf (_("Base CPU: %d\n"), task_info->base_cpu);

  /* Print who is the parent (if any).  */
  if (task_info->parent != 0)
    parent_taskno = get_task_number_from_id (task_info->parent, inf);
  if (parent_taskno)
    {
      struct ada_task_info *parent = &data->task_list[parent_taskno - 1];

      gdb_printf (_("Parent: %d"), parent_taskno);
      if (parent->name[0] != '\0')
	gdb_printf (" (%s)", parent->name);
      gdb_printf ("\n");
    }
  else
    gdb_printf (_("No parent\n"));

  /* Print the base priority.  */
  gdb_printf (_("Base Priority: %d\n"), task_info->priority);

  /* print the task current state.  */
  {
    int target_taskno = 0;

    if (task_info->caller_task)
      {
	target_taskno = get_task_number_from_id (task_info->caller_task, inf);
	gdb_printf (_("State: Accepting rendezvous with %d"),
		    target_taskno);
      }
    else if (task_info->called_task)
      {
	target_taskno = get_task_number_from_id (task_info->called_task, inf);
	gdb_printf (_("State: Waiting on task %d's entry"),
		    target_taskno);
      }
    else
      gdb_printf (_("State: %s"), get_long_state (task_info->state));

    if (target_taskno)
      {
	ada_task_info *target_task_info = &data->task_list[target_taskno - 1];

	if (target_task_info->name[0] != '\0')
	  gdb_printf (" (%s)", target_task_info->name);
      }

    gdb_printf ("\n");
  }
}

/* If ARG is empty or null, then print a list of all Ada tasks.
   Otherwise, print detailed information about the task whose ID
   is ARG.
   
   Does nothing if the program doesn't use Ada tasking.  */

static void
info_tasks_command (const char *arg, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  if (arg == NULL || *arg == '\0')
    print_ada_task_info (uiout, NULL, current_inferior ());
  else
    info_task (uiout, arg, current_inferior ());
}

/* Print a message telling the user id of the current task.
   This function assumes that tasking is in use in the inferior.  */

static void
display_current_task_id (void)
{
  const int current_task = ada_get_task_number (inferior_thread ());

  if (current_task == 0)
    gdb_printf (_("[Current task is unknown]\n"));
  else
    {
      struct ada_tasks_inferior_data *data
	= get_ada_tasks_inferior_data (current_inferior ());
      struct ada_task_info *task_info = &data->task_list[current_task - 1];

      gdb_printf (_("[Current task is %s]\n"),
		  task_to_str (current_task, task_info).c_str ());
    }
}

/* Parse and evaluate TIDSTR into a task id, and try to switch to
   that task.  Print an error message if the task switch failed.  */

static void
task_command_1 (const char *taskno_str, int from_tty, struct inferior *inf)
{
  const int taskno = value_as_long (parse_and_eval (taskno_str));
  struct ada_task_info *task_info;
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  if (taskno <= 0 || taskno > data->task_list.size ())
    error (_("Task ID %d not known.  Use the \"info tasks\" command to\n"
	     "see the IDs of currently known tasks"), taskno);
  task_info = &data->task_list[taskno - 1];

  if (!ada_task_is_alive (task_info))
    error (_("Cannot switch to task %s: Task is no longer running"),
	   task_to_str (taskno, task_info).c_str ());
   
  /* On some platforms, the thread list is not updated until the user
     performs a thread-related operation (by using the "info threads"
     command, for instance).  So this thread list may not be up to date
     when the user attempts this task switch.  Since we cannot switch
     to the thread associated to our task if GDB does not know about
     that thread, we need to make sure that any new threads gets added
     to the thread list.  */
  target_update_thread_list ();

  /* Verify that the ptid of the task we want to switch to is valid
     (in other words, a ptid that GDB knows about).  Otherwise, we will
     cause an assertion failure later on, when we try to determine
     the ptid associated thread_info data.  We should normally never
     encounter such an error, but the wrong ptid can actually easily be
     computed if target_get_ada_task_ptid has not been implemented for
     our target (yet).  Rather than cause an assertion error in that case,
     it's nicer for the user to just refuse to perform the task switch.  */
  thread_info *tp = inf->find_thread (task_info->ptid);
  if (tp == NULL)
    error (_("Unable to compute thread ID for task %s.\n"
	     "Cannot switch to this task."),
	   task_to_str (taskno, task_info).c_str ());

  switch_to_thread (tp);
  ada_find_printable_frame (get_selected_frame (NULL));
  gdb_printf (_("[Switching to task %s]\n"),
	      task_to_str (taskno, task_info).c_str ());
  print_stack_frame (get_selected_frame (NULL),
		     frame_relative_level (get_selected_frame (NULL)),
		     SRC_AND_LOC, 1);
}


/* Print the ID of the current task if TASKNO_STR is empty or NULL.
   Otherwise, switch to the task indicated by TASKNO_STR.  */

static void
task_command (const char *taskno_str, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  if (ada_build_task_list () == 0)
    {
      uiout->message (_("Your application does not use any Ada tasks.\n"));
      return;
    }

  if (taskno_str == NULL || taskno_str[0] == '\0')
    display_current_task_id ();
  else
    task_command_1 (taskno_str, from_tty, current_inferior ());
}

/* Indicate that the given inferior's task list may have changed,
   so invalidate the cache.  */

static void
ada_task_list_changed (struct inferior *inf)
{
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  data->task_list_valid_p = false;
}

/* Invalidate the per-program-space data.  */

static void
ada_tasks_invalidate_pspace_data (struct program_space *pspace)
{
  get_ada_tasks_pspace_data (pspace)->initialized_p = 0;
}

/* Invalidate the per-inferior data.  */

static void
ada_tasks_invalidate_inferior_data (struct inferior *inf)
{
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  data->known_tasks_kind = ADA_TASKS_UNKNOWN;
  data->task_list_valid_p = false;
}

/* The 'normal_stop' observer notification callback.  */

static void
ada_tasks_normal_stop_observer (struct bpstat *unused_args, int unused_args2)
{
  /* The inferior has been resumed, and just stopped. This means that
     our task_list needs to be recomputed before it can be used again.  */
  ada_task_list_changed (current_inferior ());
}

/* Clear data associated to PSPACE and all inferiors using that program
   space.  */

static void
ada_tasks_clear_pspace_data (program_space *pspace)
{
  /* The associated program-space data might have changed after
     this objfile was added.  Invalidate all cached data.  */
  ada_tasks_invalidate_pspace_data (pspace);

  /* Invalidate the per-inferior cache for all inferiors using
     this program space.  */
  for (inferior *inf : all_inferiors ())
    if (inf->pspace == pspace)
      ada_tasks_invalidate_inferior_data (inf);
}

/* Called when a new objfile was added.  */

static void
ada_tasks_new_objfile_observer (objfile *objfile)
{
  ada_tasks_clear_pspace_data (objfile->pspace);
}

/* The qcs command line flags for the "task apply" commands.  Keep
   this in sync with the "frame apply" commands.  */

using qcs_flag_option_def
  = gdb::option::flag_option_def<qcs_flags>;

static const gdb::option::option_def task_qcs_flags_option_defs[] = {
  qcs_flag_option_def {
    "q", [] (qcs_flags *opt) { return &opt->quiet; },
    N_("Disables printing the task information."),
  },

  qcs_flag_option_def {
    "c", [] (qcs_flags *opt) { return &opt->cont; },
    N_("Print any error raised by COMMAND and continue."),
  },

  qcs_flag_option_def {
    "s", [] (qcs_flags *opt) { return &opt->silent; },
    N_("Silently ignore any errors or empty output produced by COMMAND."),
  },
};

/* Create an option_def_group for the "task apply all" options, with
   FLAGS as context.  */

static inline std::array<gdb::option::option_def_group, 1>
make_task_apply_all_options_def_group (qcs_flags *flags)
{
  return {{
    { {task_qcs_flags_option_defs}, flags },
  }};
}

/* Create an option_def_group for the "task apply" options, with
   FLAGS as context.  */

static inline gdb::option::option_def_group
make_task_apply_options_def_group (qcs_flags *flags)
{
  return {{task_qcs_flags_option_defs}, flags};
}

/* Implementation of 'task apply all'.  */

static void
task_apply_all_command (const char *cmd, int from_tty)
{
  qcs_flags flags;

  auto group = make_task_apply_all_options_def_group (&flags);
  gdb::option::process_options
    (&cmd, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  validate_flags_qcs ("task apply all", &flags);

  if (cmd == nullptr || *cmd == '\0')
    error (_("Please specify a command at the end of 'task apply all'"));

  update_thread_list ();
  ada_build_task_list ();

  inferior *inf = current_inferior ();
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  /* Save a copy of the thread list and increment each thread's
     refcount while executing the command in the context of each
     thread, in case the command affects this.  */
  std::vector<std::pair<int, thread_info_ref>> thr_list_cpy;

  for (int i = 1; i <= data->task_list.size (); ++i)
    {
      ada_task_info &task = data->task_list[i - 1];
      if (!ada_task_is_alive (&task))
	continue;

      thread_info *tp = inf->find_thread (task.ptid);
      if (tp == nullptr)
	warning (_("Unable to compute thread ID for task %s.\n"
		   "Cannot switch to this task."),
		 task_to_str (i, &task).c_str ());
      else
	thr_list_cpy.emplace_back (i, thread_info_ref::new_reference (tp));
    }

  scoped_restore_current_thread restore_thread;

  for (const auto &info : thr_list_cpy)
    if (switch_to_thread_if_alive (info.second.get ()))
      thread_try_catch_cmd (info.second.get (), info.first, cmd,
			    from_tty, flags);
}

/* Implementation of 'task apply'.  */

static void
task_apply_command (const char *tidlist, int from_tty)
{

  if (tidlist == nullptr || *tidlist == '\0')
    error (_("Please specify a task ID list"));

  update_thread_list ();
  ada_build_task_list ();

  inferior *inf = current_inferior ();
  struct ada_tasks_inferior_data *data = get_ada_tasks_inferior_data (inf);

  /* Save a copy of the thread list and increment each thread's
     refcount while executing the command in the context of each
     thread, in case the command affects this.  */
  std::vector<std::pair<int, thread_info_ref>> thr_list_cpy;

  number_or_range_parser parser (tidlist);
  while (!parser.finished ())
    {
      int num = parser.get_number ();

      if (num < 1 || num - 1 >= data->task_list.size ())
	warning (_("no Ada Task with number %d"), num);
      else
	{
	  ada_task_info &task = data->task_list[num - 1];
	  if (!ada_task_is_alive (&task))
	    continue;

	  thread_info *tp = inf->find_thread (task.ptid);
	  if (tp == nullptr)
	    warning (_("Unable to compute thread ID for task %s.\n"
		       "Cannot switch to this task."),
		     task_to_str (num, &task).c_str ());
	  else
	    thr_list_cpy.emplace_back (num,
				       thread_info_ref::new_reference (tp));
	}
    }

  qcs_flags flags;
  const char *cmd = parser.cur_tok ();

  auto group = make_task_apply_options_def_group (&flags);
  gdb::option::process_options
    (&cmd, gdb::option::PROCESS_OPTIONS_UNKNOWN_IS_OPERAND, group);

  validate_flags_qcs ("task apply", &flags);

  if (*cmd == '\0')
    error (_("Please specify a command following the task ID list"));

  scoped_restore_current_thread restore_thread;

  for (const auto &info : thr_list_cpy)
    if (switch_to_thread_if_alive (info.second.get ()))
      thread_try_catch_cmd (info.second.get (), info.first, cmd,
			    from_tty, flags);
}

void _initialize_tasks ();
void
_initialize_tasks ()
{
  /* Attach various observers.  */
  gdb::observers::normal_stop.attach (ada_tasks_normal_stop_observer,
				      "ada-tasks");
  gdb::observers::new_objfile.attach (ada_tasks_new_objfile_observer,
				      "ada-tasks");
  gdb::observers::all_objfiles_removed.attach (ada_tasks_clear_pspace_data,
					       "ada-tasks");

  static struct cmd_list_element *task_cmd_list;
  static struct cmd_list_element *task_apply_list;


  /* Some new commands provided by this module.  */
  add_info ("tasks", info_tasks_command,
	    _("Provide information about all known Ada tasks."));

  add_prefix_cmd ("task", class_run, task_command,
		  _("Use this command to switch between Ada tasks.\n\
Without argument, this command simply prints the current task ID."),
		  &task_cmd_list, 1, &cmdlist);

#define TASK_APPLY_OPTION_HELP "\
Prints per-inferior task number followed by COMMAND output.\n\
\n\
By default, an error raised during the execution of COMMAND\n\
aborts \"task apply\".\n\
\n\
Options:\n\
%OPTIONS%"

  static const auto task_apply_opts
    = make_task_apply_options_def_group (nullptr);

  static std::string task_apply_help = gdb::option::build_help (_("\
Apply a command to a list of tasks.\n\
Usage: task apply ID... [OPTION]... COMMAND\n\
ID is a space-separated list of IDs of tasks to apply COMMAND on.\n"
TASK_APPLY_OPTION_HELP), task_apply_opts);

  add_prefix_cmd ("apply", class_run,
		  task_apply_command,
		  task_apply_help.c_str (),
		  &task_apply_list, 1,
		  &task_cmd_list);

  static const auto task_apply_all_opts
    = make_task_apply_all_options_def_group (nullptr);

  static std::string task_apply_all_help = gdb::option::build_help (_("\
Apply a command to all tasks in the current inferior.\n\
\n\
Usage: task apply all [OPTION]... COMMAND\n"
TASK_APPLY_OPTION_HELP), task_apply_all_opts);

  add_cmd ("all", class_run, task_apply_all_command,
	   task_apply_all_help.c_str (), &task_apply_list);
}
