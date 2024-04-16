/* s390-mkopc.c -- Generates opcode table out of s390-opc.txt
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
   Contributed by Martin Schwidefsky (schwidefsky@de.ibm.com).

   This file is part of the GNU opcodes library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   It is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this file; see the file COPYING.  If not, write to the
   Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "opcode/s390.h"

#define STRINGIFY(x) _STRINGIFY(x)
#define _STRINGIFY(x) #x

/* Length of strings without terminating '\0' character.  */
#define MAX_OPCODE_LEN 15
#define MAX_MNEMONIC_LEN 15
#define MAX_FORMAT_LEN 15
#define MAX_DESCRIPTION_LEN 127

#define MAX_CPU_LEN 15
#define MAX_MODES_LEN 15
#define MAX_FLAGS_LEN 79

/* Return code.  */
int return_code = EXIT_SUCCESS;

/* Helper to print an error message and set the return code.  */
static void __attribute__ ((format (printf, 1, 2)))
print_error (const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  fprintf(stderr, "Error: ");
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  return_code = EXIT_FAILURE;
}

struct op_struct
  {
    char  opcode[MAX_OPCODE_LEN + 1];
    char  mnemonic[MAX_MNEMONIC_LEN + 1];
    char  format[MAX_FORMAT_LEN + 1];
    int   mode_bits;
    int   min_cpu;
    int   flags;
    char  description[MAX_DESCRIPTION_LEN + 1];

    unsigned long long sort_value;
    int   no_nibbles;
  };

struct op_struct *op_array;
int max_ops;
int no_ops;

static void
createTable (void)
{
  max_ops = 256;
  op_array = malloc (max_ops * sizeof (struct op_struct));
  no_ops = 0;
}

/* `insertOpcode': insert an op_struct into sorted opcode array.  */

static void
insertOpcode (char *opcode, char *mnemonic, char *format,
	      int min_cpu, int mode_bits, int flags, char* description)
{
  char *str;
  unsigned long long sort_value;
  int no_nibbles;
  int ix, k;

  while (no_ops >= max_ops)
    {
      max_ops = max_ops * 2;
      op_array = realloc (op_array, max_ops * sizeof (struct op_struct));
    }

  sort_value = 0;
  str = opcode;
  for (ix = 0; ix < 16; ix++)
    {
      if (*str >= '0' && *str <= '9')
	sort_value = (sort_value << 4) + (*str - '0');
      else if (*str >= 'a' && *str <= 'f')
	sort_value = (sort_value << 4) + (*str - 'a' + 10);
      else if (*str >= 'A' && *str <= 'F')
	sort_value = (sort_value << 4) + (*str - 'A' + 10);
      else if (*str == '?')
	sort_value <<= 4;
      else
	break;
      str ++;
    }
  sort_value <<= 4*(16 - ix);
  sort_value += (min_cpu << 8) + mode_bits;
  no_nibbles = ix;
  for (ix = 0; ix < no_ops; ix++)
    if (sort_value > op_array[ix].sort_value)
      break;
  for (k = no_ops; k > ix; k--)
    op_array[k] = op_array[k-1];
  strncpy (op_array[ix].opcode, opcode, MAX_OPCODE_LEN);
  op_array[ix].opcode[MAX_OPCODE_LEN] = '\0';
  strncpy (op_array[ix].mnemonic, mnemonic, MAX_MNEMONIC_LEN);
  op_array[ix].mnemonic[MAX_MNEMONIC_LEN] = '\0';
  strncpy (op_array[ix].format, format, MAX_FORMAT_LEN);
  op_array[ix].format[MAX_FORMAT_LEN] = '\0';
  op_array[ix].sort_value = sort_value;
  op_array[ix].no_nibbles = no_nibbles;
  op_array[ix].min_cpu = min_cpu;
  op_array[ix].mode_bits = mode_bits;
  op_array[ix].flags = flags;
  strncpy (op_array[ix].description, description, MAX_DESCRIPTION_LEN);
  op_array[ix].description[MAX_DESCRIPTION_LEN] = '\0';
  no_ops++;
}

struct s390_cond_ext_format
{
  char nibble;
  char extension[4];
  char *description_suffix;

};

/* The mnemonic extensions for conditional jumps used to replace
   the '*' tag.  */
#define NUM_COND_EXTENSIONS 20
const struct s390_cond_ext_format s390_cond_extensions[NUM_COND_EXTENSIONS] =
{ { '1', "o", "on overflow / if ones"},		/* jump on overflow / if ones */
  { '2', "h", "on A high"},			/* jump on A high */
  { '2', "p", "on plus" },			/* jump on plus */
  { '3', "nle", "on not low or equal" },	/* jump on not low or equal */
  { '4', "l", "on A low" },			/* jump on A low */
  { '4', "m", "on minus / if mixed" },		/* jump on minus / if mixed */
  { '5', "nhe", "on not high or equal" },	/* jump on not high or equal */
  { '6', "lh", "on low or high" },		/* jump on low or high */
  { '7', "ne", "on A not equal B" },		/* jump on A not equal B */
  { '7', "nz", "on not zero / if not zeros" },	/* jump on not zero / if not zeros */
  { '8', "e", "on A equal B" },			/* jump on A equal B */
  { '8', "z", "on zero / if zeros" },		/* jump on zero / if zeros */
  { '9', "nlh", "on not low or high " },	/* jump on not low or high */
  { 'a', "he", "on high or equal" },		/* jump on high or equal */
  { 'b', "nl", "on A not low" },		/* jump on A not low */
  { 'b', "nm", "on not minus / if not mixed" },	/* jump on not minus / if not mixed */
  { 'c', "le", "on low or equal" },		/* jump on low or equal */
  { 'd', "nh", "on A not high" },		/* jump on A not high */
  { 'd', "np", "on not plus" },			/* jump on not plus */
  { 'e', "no", "on not overflow / if not ones" },/* jump on not overflow / if not ones */
};

/* The mnemonic extensions for conditional branches used to replace
   the '$' tag.  */
#define NUM_CRB_EXTENSIONS 12
const struct s390_cond_ext_format s390_crb_extensions[NUM_CRB_EXTENSIONS] =
{ { '2', "h", "on A high" },			/* jump on A high */
  { '2', "nle", "on not low or equal" },	/* jump on not low or equal */
  { '4', "l", "on A low" },			/* jump on A low */
  { '4', "nhe", "on not high or equal" },	/* jump on not high or equal */
  { '6', "ne", "on A not equal B" },		/* jump on A not equal B */
  { '6', "lh", "on low or high" },		/* jump on low or high */
  { '8', "e", "on A equal B" },			/* jump on A equal B */
  { '8', "nlh", "on not low or high" },		/* jump on not low or high */
  { 'a', "nl", "on A not low" },		/* jump on A not low */
  { 'a', "he", "on high or equal" },		/* jump on high or equal */
  { 'c', "nh", "on A not high" },		/* jump on A not high */
  { 'c', "le", "on low or equal" },		/* jump on low or equal */
};

/* As with insertOpcode instructions are added to the sorted opcode
   array.  Additionally mnemonics containing the '*<number>' tag are
   expanded to the set of conditional instructions described by
   s390_cond_extensions with the tag replaced by the respective
   mnemonic extensions.  */

static void
insertExpandedMnemonic (char *opcode, char *mnemonic, char *format,
			int min_cpu, int mode_bits, int flags, char *description)
{
  char *tag;
  char prefix[MAX_MNEMONIC_LEN + 1];
  char suffix[MAX_MNEMONIC_LEN + 1];
  char number[MAX_MNEMONIC_LEN + 1];
  int mask_start, i = 0, tag_found = 0, reading_number = 0;
  int number_p = 0, suffix_p = 0, prefix_p = 0;
  const struct s390_cond_ext_format *ext_table;
  int ext_table_length;

  if (!(tag = strpbrk (mnemonic, "*$")))
    {
      insertOpcode (opcode, mnemonic, format, min_cpu, mode_bits, flags, description);
      return;
    }

  while (mnemonic[i] != '\0')
    {
      if (mnemonic[i] == *tag)
	{
	  if (tag_found)
	    goto malformed_mnemonic;

	  tag_found = 1;
	  reading_number = 1;
	}
      else
	switch (mnemonic[i])
	  {
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9':
	    if (!tag_found || !reading_number)
	      goto malformed_mnemonic;

	    number[number_p++] = mnemonic[i];
	    break;

	  default:
	    if (reading_number)
	      {
		if (!number_p)
		  goto malformed_mnemonic;
		else
		  reading_number = 0;
	      }

	    if (tag_found)
	      suffix[suffix_p++] = mnemonic[i];
	    else
	      prefix[prefix_p++] = mnemonic[i];
	  }
      i++;
    }

  prefix[prefix_p] = '\0';
  suffix[suffix_p] = '\0';
  number[number_p] = '\0';

  if (sscanf (number, "%d", &mask_start) != 1)
    goto malformed_mnemonic;

  if (mask_start & 3)
    {
      print_error ("Mnemonic \"%s\": Conditional mask not at nibble boundary\n", mnemonic);
      return;
    }

  mask_start >>= 2;

  switch (*tag)
    {
    case '*':
      ext_table = s390_cond_extensions;
      ext_table_length = NUM_COND_EXTENSIONS;
      break;
    case '$':
      ext_table = s390_crb_extensions;
      ext_table_length = NUM_CRB_EXTENSIONS;
      break;
    default:
      abort ();			/* Should be unreachable.  */
    }

  for (i = 0; i < ext_table_length; i++)
    {
      char new_mnemonic[MAX_MNEMONIC_LEN + 1];
      char new_description[MAX_DESCRIPTION_LEN + 1];

      opcode[mask_start] = ext_table[i].nibble;

      if (snprintf (new_mnemonic, sizeof (new_mnemonic), "%s%s%s", prefix,
		    ext_table[i].extension, suffix) >= sizeof (new_mnemonic))
	{
	  print_error ("Mnemonic: \"%s\": Concatenated mnemonic exceeds max. length\n", mnemonic);
	  return;
	}

      if (snprintf (new_description, sizeof (new_description), "%s %s", description,
		    ext_table[i].description_suffix) >= sizeof (new_description))
	{
	  print_error ("Mnemonic \"%s\": Concatenated description exceeds max. length\n", mnemonic);
	  return;
	}

      insertOpcode (opcode, new_mnemonic, format, min_cpu, mode_bits, flags, new_description);
    }
  return;

 malformed_mnemonic:
  print_error ("Malformed mnemonic: %s\n", mnemonic);
}

static const char file_header[] =
  "/* The opcode table. This file was generated by s390-mkopc.\n\n"
  "   The format of the opcode table is:\n\n"
  "   NAME	     OPCODE	MASK	OPERANDS\n\n"
  "   Name is the name of the instruction.\n"
  "   OPCODE is the instruction opcode.\n"
  "   MASK is the opcode mask; this is used to tell the disassembler\n"
  "     which bits in the actual opcode must match OPCODE.\n"
  "   OPERANDS is the list of operands.\n\n"
  "   The disassembler reads the table in order and prints the first\n"
  "   instruction which matches.\n"
  "   MODE_BITS - zarch or esa\n"
  "   MIN_CPU - number of the min cpu level required\n"
  "   FLAGS - instruction flags.\n"
  "   DESCRIPTION - description of the instruction.  */\n\n"
  "const struct s390_opcode s390_opcodes[] =\n  {\n";

/* `dumpTable': write opcode table.  */

static void
dumpTable (void)
{
  char *str;
  int  ix;

  /*  Write hash table entries (slots).  */
  printf ("%s", file_header);

  for (ix = 0; ix < no_ops; ix++)
    {
      printf ("  { \"%s\", ", op_array[ix].mnemonic);
      for (str = op_array[ix].opcode; *str != 0; str++)
	if (*str == '?')
	  *str = '0';
      printf ("OP%i(0x%sLL), ",
	      op_array[ix].no_nibbles*4, op_array[ix].opcode);
      printf ("MASK_%s, INSTR_%s, ",
	      op_array[ix].format, op_array[ix].format);
      printf ("%i, ", op_array[ix].mode_bits);
      printf ("%i, ", op_array[ix].min_cpu);
      printf ("%i, ", op_array[ix].flags);
      printf ("\"%s\" }", op_array[ix].description);
      if (ix < no_ops-1)
	printf (",\n");
      else
	printf ("\n");
    }
  printf ("};\n\n");
  printf ("const int s390_num_opcodes =\n");
  printf ("  sizeof (s390_opcodes) / sizeof (s390_opcodes[0]);\n\n");
}

int
main (void)
{
  char currentLine[256];

  createTable ();

  /*  Read opcode descriptions from `stdin'.  For each mnemonic,
      make an entry into the opcode table.  */
  while (fgets (currentLine, sizeof (currentLine), stdin) != NULL)
    {
      char  opcode[MAX_OPCODE_LEN + 1];
      char  mnemonic[MAX_MNEMONIC_LEN + 1];
      char  format[MAX_FORMAT_LEN + 1];
      char  description[MAX_DESCRIPTION_LEN + 1];
      char  cpu_string[MAX_CPU_LEN + 1];
      char  modes_string[MAX_MODES_LEN + 1];
      char  flags_string[MAX_FLAGS_LEN + 1];
      int   min_cpu;
      int   mode_bits;
      int   flag_bits;
      int   num_matched;
      char  *str;

      if (currentLine[0] == '#' || currentLine[0] == '\n')
	continue;
      memset (opcode, '\0', sizeof(opcode));
      num_matched = sscanf (currentLine,
			    "%" STRINGIFY (MAX_OPCODE_LEN) "s "
			    "%" STRINGIFY (MAX_MNEMONIC_LEN) "s "
			    "%" STRINGIFY (MAX_FORMAT_LEN) "s "
			    "\"%" STRINGIFY (MAX_DESCRIPTION_LEN) "[^\"]\" "
			    "%" STRINGIFY (MAX_CPU_LEN) "s "
			    "%" STRINGIFY (MAX_MODES_LEN) "s "
			    "%" STRINGIFY (MAX_FLAGS_LEN) "[^\n]",
			    opcode, mnemonic, format, description,
			    cpu_string, modes_string, flags_string);
      if (num_matched != 6 && num_matched != 7)
	{
	  print_error ("Couldn't scan line %s\n", currentLine);
	  exit (EXIT_FAILURE);
	}

      if (strcmp (cpu_string, "g5") == 0
	  || strcmp (cpu_string, "arch3") == 0)
	min_cpu = S390_OPCODE_G5;
      else if (strcmp (cpu_string, "g6") == 0)
	min_cpu = S390_OPCODE_G6;
      else if (strcmp (cpu_string, "z900") == 0
	       || strcmp (cpu_string, "arch5") == 0)
	min_cpu = S390_OPCODE_Z900;
      else if (strcmp (cpu_string, "z990") == 0
	       || strcmp (cpu_string, "arch6") == 0)
	min_cpu = S390_OPCODE_Z990;
      else if (strcmp (cpu_string, "z9-109") == 0)
	min_cpu = S390_OPCODE_Z9_109;
      else if (strcmp (cpu_string, "z9-ec") == 0
	       || strcmp (cpu_string, "arch7") == 0)
	min_cpu = S390_OPCODE_Z9_EC;
      else if (strcmp (cpu_string, "z10") == 0
	       || strcmp (cpu_string, "arch8") == 0)
	min_cpu = S390_OPCODE_Z10;
      else if (strcmp (cpu_string, "z196") == 0
	       || strcmp (cpu_string, "arch9") == 0)
	min_cpu = S390_OPCODE_Z196;
      else if (strcmp (cpu_string, "zEC12") == 0
	       || strcmp (cpu_string, "arch10") == 0)
	min_cpu = S390_OPCODE_ZEC12;
      else if (strcmp (cpu_string, "z13") == 0
	       || strcmp (cpu_string, "arch11") == 0)
	min_cpu = S390_OPCODE_Z13;
      else if (strcmp (cpu_string, "z14") == 0
	       || strcmp (cpu_string, "arch12") == 0)
	min_cpu = S390_OPCODE_ARCH12;
      else if (strcmp (cpu_string, "z15") == 0
	       || strcmp (cpu_string, "arch13") == 0)
	min_cpu = S390_OPCODE_ARCH13;
      else if (strcmp (cpu_string, "z16") == 0
	       || strcmp (cpu_string, "arch14") == 0)
	min_cpu = S390_OPCODE_ARCH14;
      else {
	print_error ("Mnemonic \"%s\": Couldn't parse CPU string: %s\n",
		     mnemonic, cpu_string);
	goto continue_loop;
      }

      str = modes_string;
      mode_bits = 0;
      do {
	if (strncmp (str, "esa", 3) == 0
	    && (str[3] == 0 || str[3] == ',')) {
	  mode_bits |= 1 << S390_OPCODE_ESA;
	  str += 3;
	} else if (strncmp (str, "zarch", 5) == 0
		   && (str[5] == 0 || str[5] == ',')) {
	  mode_bits |= 1 << S390_OPCODE_ZARCH;
	  str += 5;
	} else {
	  print_error ("Mnemonic \"%s\": Couldn't parse modes string: %s\n",
		       mnemonic, modes_string);
	  goto continue_loop;
	}
	if (*str == ',')
	  str++;
      } while (*str != 0);

      flag_bits = 0;

      if (num_matched == 7)
	{
	  str = flags_string;
	  do {
	    if (strncmp (str, "optparm", 7) == 0
		&& (str[7] == 0 || str[7] == ',')) {
	      flag_bits |= S390_INSTR_FLAG_OPTPARM;
	      str += 7;
	    } else if (strncmp (str, "optparm2", 8) == 0
		       && (str[8] == 0 || str[8] == ',')) {
	      flag_bits |= S390_INSTR_FLAG_OPTPARM2;
	      str += 8;
	    } else if (strncmp (str, "htm", 3) == 0
		       && (str[3] == 0 || str[3] == ',')) {
	      flag_bits |= S390_INSTR_FLAG_HTM;
	      str += 3;
	    } else if (strncmp (str, "vx", 2) == 0
		       && (str[2] == 0 || str[2] == ',')) {
	      flag_bits |= S390_INSTR_FLAG_VX;
	      str += 2;
	    } else if (strncmp (str, "jump", 7) == 0
		&& (str[4] == 0 || str[4] == ',')) {
	      flag_bits |= S390_INSTR_FLAGS_CLASS_JUMP;
	      str += 4;
	    } else if (strncmp (str, "condjump", 7) == 0
		&& (str[8] == 0 || str[8] == ',')) {
	      flag_bits |= S390_INSTR_FLAGS_CLASS_CONDJUMP;
	      str += 8;
	    } else if (strncmp (str, "jumpsr", 7) == 0
		&& (str[6] == 0 || str[6] == ',')) {
	      flag_bits |= S390_INSTR_FLAGS_CLASS_JUMPSR;
	      str += 6;
	    } else {
	      print_error ("Mnemonic \"%s\": Couldn't parse flags string: %s\n",
			   mnemonic, flags_string);
	      goto continue_loop;
	    }
	    if (*str == ',')
	      str++;
	  } while (*str != 0);
	}
      insertExpandedMnemonic (opcode, mnemonic, format, min_cpu, mode_bits, flag_bits, description);

 continue_loop:
      ;
    }

  dumpTable ();
  return return_code;
}
