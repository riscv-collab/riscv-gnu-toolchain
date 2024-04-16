/* Simulation code for the CR16 processor.
   Copyright (C) 2008-2024 Free Software Foundation, Inc.
   Contributed by M Ranga Swami Reddy <MR.Swami.Reddy@nsc.com>

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.  */

/* This must come before any other includes.  */
#include "defs.h"

#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "ansidecl.h"
#include "opcode/cr16.h"

static void write_header (void);
static void write_opcodes (void);
static void write_template (void);

int
main (int argc, char *argv[])
{
  if ((argc > 1) && (strcmp (argv[1],"-h") == 0))
    write_header();
  else if ((argc > 1) && (strcmp (argv[1],"-t") == 0))
    write_template ();
  else
    write_opcodes();
  return 0;
}


static void
write_header (void)
{
  int i = 0; 

  /* Loop over instruction table until a full match is found.  */
  for ( ; i < NUMOPCODES; i++)
    printf("void OP_%lX_%X (SIM_DESC, SIM_CPU *);\t\t/* %s */\n",
	   cr16_instruction[i].match, (32 - cr16_instruction[i].match_bits),
	   cr16_instruction[i].mnemonic);
}


/* write_template creates a file all required functions, 
   ready to be filled out.  */

static void
write_template (void)
{
  int i = 0, j, k;

  printf ("#include \"defs.h\"\n");
  printf ("#include \"sim-main.h\"\n");
  printf ("#include \"cr16-sim.h\"\n");
  printf ("#include \"simops.h\"\n\n");

  for ( ; i < NUMOPCODES; i++)
    {
      if (cr16_instruction[i].size != 0)
{
  printf ("/* %s */\nvoid\nOP_%lX_%X (SIM_DESC sd, SIM_CPU *cpu)\n{\n",
	  cr16_instruction[i].mnemonic, cr16_instruction[i].match,
	  (32 - cr16_instruction[i].match_bits));
  
  /* count operands.  */
  j = 0;
  for (k=0;k<5;k++)
    {
      if (cr16_instruction[i].operands[k].op_type == dummy)
                break;
              else
                j++;
    }
  switch (j)
    {
    case 0:
      printf ("printf(\"   %s\\n\");\n",cr16_instruction[i].mnemonic);
      break;
    case 1:
      printf ("printf(\"   %s\\t%%x\\n\",OP[0]);\n",cr16_instruction[i].mnemonic);
      break;
    case 2:
      printf ("printf(\"   %s\\t%%x,%%x\\n\",OP[0],OP[1]);\n",cr16_instruction[i].mnemonic);
      break;
    case 3:
      printf ("printf(\"   %s\\t%%x,%%x,%%x\\n\",OP[0],OP[1],OP[2]);\n",cr16_instruction[i].mnemonic);
      break;
    default:
      fprintf (stderr,"Too many operands: %d\n",j);
    }
  printf ("}\n\n");
}
    }
}


long Opcodes[512];

#if 0
static int curop=0;

static void
check_opcodes( long op)
{
  int i;

  for (i=0;i<curop;i++)
    if (Opcodes[i] == op)
      fprintf(stderr,"DUPLICATE OPCODES: %lx\n", op);
}
#endif

static void
write_opcodes (void)
{
  int i = 0, j = 0, k;
  
  /* write out opcode table.  */
  printf ("#include \"defs.h\"\n");
  printf ("#include \"cr16-sim.h\"\n");
  printf ("#include \"simops.h\"\n\n");
  printf ("struct simops Simops[] = {\n");
  
  for (i = NUMOPCODES-1; i >= 0; --i)
    {
      if (cr16_instruction[i].size != 0)
{
           printf ("  { \"%s\", %u, %d, %ld, %u, \"OP_%lX_%X\", OP_%lX_%X, ", 
                    cr16_instruction[i].mnemonic, cr16_instruction[i].size, 
                    cr16_instruction[i].match_bits, cr16_instruction[i].match,
                     cr16_instruction[i].flags, ((BIN(cr16_instruction[i].match, cr16_instruction[i].match_bits))>>(cr16_instruction[i].match_bits)),
             (32 - cr16_instruction[i].match_bits),
                     ((BIN(cr16_instruction[i].match, cr16_instruction[i].match_bits))>>(cr16_instruction[i].match_bits)), (32 - cr16_instruction[i].match_bits));
      
  j = 0;
  for (k=0;k<5;k++)
    {
      if (cr16_instruction[i].operands[k].op_type == dummy)
                break;
              else
                j++;
    }
  printf ("%d, ",j);
  
  j = 0;
  for (k=0;k<4;k++)
    {
      int optype = cr16_instruction[i].operands[k].op_type;
      int shift = cr16_instruction[i].operands[k].shift;
      if (j == 0)
        printf ("{");
      else
        printf (", ");
      printf ("{");
      printf ("%d,%d",optype, shift);
      printf ("}");
      j = 1;
   }
 if (j)
  printf ("}");
 printf ("},\n");
        }
    }
  printf (" { \"NULL\",1,8,0,0,\"OP_0_20\",OP_0_20,0,{{0,0},{0,0},{0,0},{0,0}}},\n};\n");
}
