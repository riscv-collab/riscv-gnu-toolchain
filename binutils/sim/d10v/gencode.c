#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include "ansidecl.h"
#include "opcode/d10v.h"

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
  struct d10v_opcode *opcode;

  for (opcode = (struct d10v_opcode *)d10v_opcodes; opcode->name; opcode++)
    if (opcode->format != OPCODE_FAKE)
      printf ("void OP_%lX (SIM_DESC, SIM_CPU *);\t\t/* %s */\n", opcode->opcode, opcode->name);
}


/* write_template creates a file all required functions, ready */
/* to be filled out */

static void
write_template (void)
{
  struct d10v_opcode *opcode;
  int i,j;

  printf ("#include \"d10v-sim.h\"\n");
  printf ("#include \"simops.h\"\n");

  for (opcode = (struct d10v_opcode *)d10v_opcodes; opcode->name; opcode++)
    {
      if (opcode->format != OPCODE_FAKE)
	{
	  printf("/* %s */\nvoid\nOP_%lX ()\n{\n", opcode->name, opcode->opcode);
	  
	  /* count operands */
	  j = 0;
	  for (i=0;i<6;i++)
	    {
	      int flags = d10v_operands[opcode->operands[i]].flags;
	      if ((flags & OPERAND_REG) || (flags & OPERAND_NUM) || (flags & OPERAND_ADDR))
		j++;
	    }
	  switch (j)
	    {
	    case 0:
	      printf ("printf(\"   %s\\n\");\n",opcode->name);
	      break;
	    case 1:
	      printf ("printf(\"   %s\\t%%x\\n\",OP[0]);\n",opcode->name);
	      break;
	    case 2:
	      printf ("printf(\"   %s\\t%%x,%%x\\n\",OP[0],OP[1]);\n",opcode->name);
	      break;
	    case 3:
	      printf ("printf(\"   %s\\t%%x,%%x,%%x\\n\",OP[0],OP[1],OP[2]);\n",opcode->name);
	      break;
	    default:
	      fprintf (stderr,"Too many operands: %d\n",j);
	    }
	  printf ("}\n\n");
	}
    }
}


long Opcodes[512];
static int curop=0;

static void
check_opcodes( long op)
{
  int i;

  for (i=0;i<curop;i++)
    if (Opcodes[i] == op)
      fprintf(stderr,"DUPLICATE OPCODES: %lx\n", op);
}

static void
write_opcodes (void)
{
  struct d10v_opcode *opcode;
  int i, j;
  
  /* write out opcode table */
  printf ("#include \"sim-main.h\"\n");
  printf ("#include \"d10v-sim.h\"\n");
  printf ("#include \"simops.h\"\n\n");
  printf ("struct simops Simops[] = {\n");
  
  for (opcode = (struct d10v_opcode *)d10v_opcodes; opcode->name; opcode++)
    {
      if (opcode->format != OPCODE_FAKE)
	{
	  printf ("  { %ld,%d,%ld,%d,%d,%d,%d,OP_%lX,", opcode->opcode,
		  (opcode->format & LONG_OPCODE) ? 1 : 0, opcode->mask, opcode->format, 
		  opcode->cycles, opcode->unit, opcode->exec_type, opcode->opcode);
      
	  /* REMOVE ME */
	  check_opcodes (opcode->opcode);
	  Opcodes[curop++] = opcode->opcode;

	  j = 0;
	  for (i=0;i<6;i++)
	    {
	      int flags = d10v_operands[opcode->operands[i]].flags;
	      if ((flags & OPERAND_REG) || (flags & OPERAND_NUM) || (flags & OPERAND_ADDR))
		j++;
	    }
	  printf ("%d,",j);
	  
	  j = 0;
	  for (i=0;i<6;i++)
	    {
	      int flags = d10v_operands[opcode->operands[i]].flags;
	      int shift = d10v_operands[opcode->operands[i]].shift;
	      if ((flags & OPERAND_REG) || (flags & OPERAND_NUM)|| (flags & OPERAND_ADDR))
		{
		  if (j == 0)
		    printf ("{");
		  else
		    printf (", ");
		  if ((flags & OPERAND_REG) && (opcode->format == LONG_L))
		    shift += 15;
		  printf ("%d,%d,%d",shift,d10v_operands[opcode->operands[i]].bits,flags);
		  j = 1;
		}
	    }
	  if (j)
	    printf ("}");
	  printf ("},\n");
	}
    }
  printf ("{ 0,0,0,0,0,0,0,(void (*)())0,0,{0,0,0}},\n};\n");
}
