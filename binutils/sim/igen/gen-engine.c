/* The IGEN simulator generator for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney.

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

#include "misc.h"
#include "lf.h"
#include "table.h"
#include "filter.h"

#include "igen.h"

#include "ld-insn.h"
#include "ld-decode.h"

#include "gen.h"

#include "gen-idecode.h"
#include "gen-engine.h"
#include "gen-icache.h"
#include "gen-semantics.h"


static void
print_engine_issue_prefix_hook (lf *file)
{
  lf_printf (file, "\n");
  lf_indent_suppress (file);
  lf_printf (file, "#if defined (ENGINE_ISSUE_PREFIX_HOOK)\n");
  lf_printf (file, "ENGINE_ISSUE_PREFIX_HOOK();\n");
  lf_indent_suppress (file);
  lf_printf (file, "#endif\n");
  lf_printf (file, "\n");
}

static void
print_engine_issue_postfix_hook (lf *file)
{
  lf_printf (file, "\n");
  lf_indent_suppress (file);
  lf_printf (file, "#if defined (ENGINE_ISSUE_POSTFIX_HOOK)\n");
  lf_printf (file, "ENGINE_ISSUE_POSTFIX_HOOK();\n");
  lf_indent_suppress (file);
  lf_printf (file, "#endif\n");
  lf_printf (file, "\n");
}


static void
print_run_body (lf *file, const gen_entry *table)
{
  /* Output the function to execute real code:

     Unfortunatly, there are multiple cases to consider vis:

     <icache> X <smp>

     Consequently this function is written in multiple different ways */

  lf_printf (file, "{\n");
  lf_indent (file, +2);
  if (!options.gen.smp)
    {
      lf_printf (file, "%sinstruction_address cia;\n",
		 options.module.global.prefix.l);
    }
  lf_printf (file, "int current_cpu = next_cpu_nr;\n");

  if (options.gen.icache)
    {
      lf_printf (file, "/* flush the icache of a possible break insn */\n");
      lf_printf (file, "{\n");
      lf_printf (file, "  int cpu_nr;\n");
      lf_printf (file, "  for (cpu_nr = 0; cpu_nr < nr_cpus; cpu_nr++)\n");
      lf_printf (file, "    cpu_flush_icache (STATE_CPU (sd, cpu_nr));\n");
      lf_printf (file, "}\n");
    }

  if (!options.gen.smp)
    {

      lf_putstr (file, "\
/* CASE 1: NO SMP (with or with out instruction cache).\n\
\n\
In this case, we can take advantage of the fact that the current\n\
instruction address (CIA) does not need to be read from / written to\n\
the CPU object after the execution of an instruction.\n\
\n\
Instead, CIA is only saved when the main loop exits.  This occures\n\
when either sim_engine_halt or sim_engine_restart is called.  Both of\n\
these functions save the current instruction address before halting /\n\
restarting the simulator.\n\
\n\
As a variation, there may also be support for an instruction cracking\n\
cache. */\n\
\n\
");

      lf_putstr (file, "\n");
      lf_putstr (file, "/* prime the main loop */\n");
      lf_putstr (file, "SIM_ASSERT (current_cpu == 0);\n");
      lf_putstr (file, "SIM_ASSERT (nr_cpus == 1);\n");
      lf_putstr (file, "cia = CPU_PC_GET (CPU);\n");

      lf_putstr (file, "\n");
      lf_putstr (file, "while (1)\n");
      lf_putstr (file, "  {\n");
      lf_indent (file, +4);

      lf_printf (file, "%sinstruction_address nia;\n",
		 options.module.global.prefix.l);

      lf_printf (file, "\n");
      if (!options.gen.icache)
	{
	  lf_printf (file,
		     "%sinstruction_word instruction_0 = IMEM%d (cia);\n",
		     options.module.global.prefix.l, options.insn_bit_size);
	  print_engine_issue_prefix_hook (file);
	  print_idecode_body (file, table, "nia = ");
	  print_engine_issue_postfix_hook (file);
	}
      else
	{
	  lf_putstr (file, "idecode_cache *cache_entry =\n");
	  lf_putstr (file, "  cpu_icache_entry (cpu, cia);\n");
	  lf_putstr (file, "if (cache_entry->address == cia)\n");
	  lf_putstr (file, "  {\n");
	  lf_indent (file, -4);
	  lf_putstr (file, "/* cache hit */\n");
	  lf_putstr (file,
		     "idecode_semantic *const semantic = cache_entry->semantic;\n");
	  lf_putstr (file, "cia = semantic (cpu, cache_entry, cia);\n");
	  /* tail */
	  lf_indent (file, -4);
	  lf_putstr (file, "  }\n");
	  lf_putstr (file, "else\n");
	  lf_putstr (file, "  {\n");
	  lf_indent (file, +4);
	  lf_putstr (file, "/* cache miss */\n");
	  if (!options.gen.semantic_icache)
	    {
	      lf_putstr (file, "idecode_semantic *semantic;\n");
	    }
	  lf_printf (file, "instruction_word instruction = IMEM%d (cia);\n",
		     options.insn_bit_size);
	  lf_putstr (file, "if (WITH_MON != 0)\n");
	  lf_putstr (file,
		     "  mon_event (mon_event_icache_miss, cpu, cia);\n");
	  if (options.gen.semantic_icache)
	    {
	      lf_putstr (file, "{\n");
	      lf_indent (file, +2);
	      print_engine_issue_prefix_hook (file);
	      print_idecode_body (file, table, "nia =");
	      print_engine_issue_postfix_hook (file);
	      lf_indent (file, -2);
	      lf_putstr (file, "}\n");
	    }
	  else
	    {
	      print_engine_issue_prefix_hook (file);
	      print_idecode_body (file, table, "semantic =");
	      lf_putstr (file, "nia = semantic (cpu, cache_entry, cia);\n");
	      print_engine_issue_postfix_hook (file);
	    }
	  lf_indent (file, -4);
	  lf_putstr (file, "  }\n");
	}

      /* update the cpu if necessary */
      switch (options.gen.nia)
	{
	case nia_is_cia_plus_one:
	  lf_printf (file, "\n");
	  lf_printf (file, "/* Update the instruction address */\n");
	  lf_printf (file, "cia = nia;\n");
	  break;
	case nia_is_void:
	case nia_is_invalid:
	  ERROR ("engine gen when NIA complex");
	}

      /* events */
      lf_putstr (file, "\n");
      lf_putstr (file, "/* process any events */\n");
      lf_putstr (file, "if (sim_events_tick (sd))\n");
      lf_putstr (file, "  {\n");
      lf_putstr (file, "    CPU_PC_SET (CPU, cia);\n");
      lf_putstr (file, "    sim_events_process (sd);\n");
      lf_putstr (file, "    cia = CPU_PC_GET (CPU);\n");
      lf_putstr (file, "  }\n");

      lf_indent (file, -4);
      lf_printf (file, "  }\n");
    }

  if (options.gen.smp)
    {

      lf_putstr (file, "\
/* CASE 2: SMP (With or without ICACHE)\n\
\n\
The complexity here comes from needing to correctly halt the simulator\n\
when it is aborted.  For instance, if cpu0 requests a restart then\n\
cpu1 will normally be the next cpu that is run.  Cpu0 being restarted\n\
after all the other CPU's and the event queue have been processed */\n\
\n\
");

      lf_putstr (file, "\n");
      lf_printf (file,
		 "/* have ensured that the event queue is NOT next */\n");
      lf_printf (file, "SIM_ASSERT (current_cpu >= 0);\n");
      lf_printf (file, "SIM_ASSERT (current_cpu <= nr_cpus - 1);\n");
      lf_printf (file, "SIM_ASSERT (nr_cpus <= MAX_NR_PROCESSORS);\n");

      lf_putstr (file, "\n");
      lf_putstr (file, "while (1)\n");
      lf_putstr (file, "  {\n");
      lf_indent (file, +4);
      lf_putstr (file, "sim_cpu *cpu = STATE_CPU (sd, current_cpu);\n");
      lf_putstr (file, "instruction_address cia = CPU_PC_GET (cpu);\n");
      lf_putstr (file, "\n");

      if (!options.gen.icache)
	{
	  lf_printf (file, "instruction_word instruction_0 = IMEM%d (cia);\n",
		     options.insn_bit_size);
	  print_engine_issue_prefix_hook (file);
	  print_idecode_body (file, table, "cia =");
	  lf_putstr (file, "CPU_PC_SET (cpu, cia);\n");
	  print_engine_issue_postfix_hook (file);
	}

      if (options.gen.icache)
	{
	  lf_putstr (file, "engine_cache *cache_entry =\n");
	  lf_putstr (file, "  cpu_icache_entry(processor, cia);\n");
	  lf_putstr (file, "\n");
	  lf_putstr (file, "if (cache_entry->address == cia) {\n");
	  {
	    lf_indent (file, +2);
	    lf_putstr (file, "\n");
	    lf_putstr (file, "/* cache hit */\n");
	    lf_putstr (file,
		       "engine_semantic *semantic = cache_entry->semantic;\n");
	    lf_putstr (file,
		       "cia = semantic(processor, cache_entry, cia);\n");
	    /* tail */
	    lf_putstr (file, "cpu_set_program_counter(processor, cia);\n");
	    lf_putstr (file, "\n");
	    lf_indent (file, -2);
	  }
	  lf_putstr (file, "}\n");
	  lf_putstr (file, "else {\n");
	  {
	    lf_indent (file, +2);
	    lf_putstr (file, "\n");
	    lf_putstr (file, "/* cache miss */\n");
	    if (!options.gen.semantic_icache)
	      {
		lf_putstr (file, "engine_semantic *semantic;\n");
	      }
	    lf_printf (file, "instruction_word instruction = IMEM%d (cia);\n",
		       options.insn_bit_size);
	    lf_putstr (file, "if (WITH_MON != 0)\n");
	    lf_putstr (file,
		       "  mon_event(mon_event_icache_miss, processors[current_cpu], cia);\n");
	    if (options.gen.semantic_icache)
	      {
		lf_putstr (file, "{\n");
		lf_indent (file, +2);
		print_engine_issue_prefix_hook (file);
		print_idecode_body (file, table, "cia =");
		print_engine_issue_postfix_hook (file);
		lf_indent (file, -2);
		lf_putstr (file, "}\n");
	      }
	    else
	      {
		print_engine_issue_prefix_hook (file);
		print_idecode_body (file, table, "semantic = ");
		lf_putstr (file,
			   "cia = semantic(processor, cache_entry, cia);\n");
		print_engine_issue_postfix_hook (file);
	      }
	    /* tail */
	    lf_putstr (file, "cpu_set_program_counter(processor, cia);\n");
	    lf_putstr (file, "\n");
	    lf_indent (file, -2);
	  }
	  lf_putstr (file, "}\n");
	}

      lf_putstr (file, "\n");
      lf_putstr (file, "current_cpu += 1;\n");
      lf_putstr (file, "if (current_cpu == nr_cpus)\n");
      lf_putstr (file, "  {\n");
      lf_putstr (file, "    if (sim_events_tick (sd))\n");
      lf_putstr (file, "      {\n");
      lf_putstr (file, "        sim_events_process (sd);\n");
      lf_putstr (file, "      }\n");
      lf_putstr (file, "    current_cpu = 0;\n");
      lf_putstr (file, "  }\n");

      /* tail */
      lf_indent (file, -4);
      lf_putstr (file, "  }\n");
    }


  lf_indent (file, -2);
  lf_putstr (file, "}\n");
}


/****************************************************************/

void
print_engine_run_function_header (lf *file,
				  const char *processor,
				  function_decl_type decl_type)
{
  int indent;
  lf_printf (file, "\n");
  switch (decl_type)
    {
    case is_function_declaration:
      lf_print__function_type (file, "void", "INLINE_ENGINE", "\n");
      break;
    case is_function_definition:
      lf_print__function_type (file, "void", "INLINE_ENGINE", " ");
      break;
    case is_function_variable:
      lf_printf (file, "void (*");
      break;
    }
  indent = print_function_name (file, "run", NULL,	/* format name */
				processor, NULL,	/* expanded bits */
				function_name_prefix_engine);
  switch (decl_type)
    {
    case is_function_definition:
      lf_putstr (file, "\n(");
      indent = 1;
      break;
    case is_function_declaration:
      indent += lf_printf (file, " (");
      break;
    case is_function_variable:
      lf_putstr (file, ")\n(");
      indent = 1;
      break;
    }
  lf_indent (file, +indent);
  lf_printf (file, "SIM_DESC sd,\n");
  lf_printf (file, "int next_cpu_nr,\n");
  lf_printf (file, "int nr_cpus,\n");
  lf_printf (file, "int siggnal)");
  lf_indent (file, -indent);
  switch (decl_type)
    {
    case is_function_definition:
      lf_putstr (file, "\n");
      break;
    case is_function_variable:
    case is_function_declaration:
      lf_putstr (file, ";\n");
      break;
    }
}


void
gen_engine_h (lf *file,
	      const gen_table *gen,
	      const insn_table *isa,
	      cache_entry *cache_rules)
{
  gen_list *entry;
  for (entry = gen->tables; entry != NULL; entry = entry->next)
    {
      print_engine_run_function_header (file,
					(options.gen.multi_sim
					 ? entry->model->name
					 : NULL), is_function_declaration);
    }
}


void
gen_engine_c (lf *file,
	      const gen_table *gen,
	      const insn_table *isa,
	      cache_entry *cache_rules)
{
  const gen_list *entry;
  /* the intro */
  print_includes (file);
  print_include_inline (file, options.module.semantics);
  print_include (file, options.module.engine);
  lf_printf (file, "\n");
  lf_printf (file, "#include \"sim-assert.h\"\n");
  lf_printf (file, "\n");
  print_idecode_globals (file);
  lf_printf (file, "\n");

  for (entry = gen->tables; entry != NULL; entry = entry->next)
    {
      switch (options.gen.code)
	{
	case generate_calls:
	  print_idecode_lookups (file, entry->table, cache_rules);

	  /* output the main engine routine */
	  print_engine_run_function_header (file,
					    (options.gen.multi_sim
					     ? entry->model->name
					     : NULL), is_function_definition);
	  print_run_body (file, entry->table);
	  break;

	case generate_jumps:
	  ERROR ("Jumps currently unimplemented");
	  break;
	}
    }
}
