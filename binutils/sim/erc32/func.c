/* This file is part of SIS (SPARC instruction simulator)

   Copyright (C) 1995-2024 Free Software Foundation, Inc.
   Contributed by Jiri Gaisler, European Space Agency

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

/* This must come before any other includes.  */
#include "defs.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "sis.h"
#include <dis-asm.h>
#include "sim-config.h"
#include <inttypes.h>
#include <sys/time.h>

#define	VAL(x)	strtoul(x,(char **)NULL,0)

struct disassemble_info dinfo;
struct pstate   sregs;
extern struct estate ebase;
int             ctrl_c = 0;
int             sis_verbose = 0;
char           *sis_version = "2.7.5";
int             nfp = 0;
int             ift = 0;
int             wrp = 0;
int             rom8 = 0;
int             uben = 0;
int		termsave;
int             sparclite = 0;		/* emulating SPARClite instructions? */
int             sparclite_board = 0;	/* emulating SPARClite board RAM? */
char            uart_dev1[128] = "";
char            uart_dev2[128] = "";
extern	int	ext_irl;
uint32_t		last_load_addr = 0;

#ifdef ERRINJ
uint32_t		errcnt = 0;
uint32_t		errper = 0;
uint32_t		errtt = 0;
uint32_t		errftt = 0;
uint32_t		errmec = 0;
#endif

/* Forward declarations */

static int	batch (struct pstate *sregs, char *fname);
static void	set_rega (struct pstate *sregs, char *reg, uint32_t rval);
static void	disp_reg (struct pstate *sregs, char *reg);
static uint32_t	limcalc (float32 freq);
static void	int_handler (int32_t sig);
static void	init_event (void);
static int	disp_fpu (struct pstate  *sregs);
static void	disp_regs (struct pstate  *sregs, int cwp);
static void	disp_ctrl (struct pstate *sregs);
static void	disp_mem (uint32_t addr, uint32_t len);

static int 
batch(struct pstate *sregs, char *fname)
{
    FILE           *fp;
    char           *lbuf = NULL;
    size_t         len = 0;
    size_t         slen;

    if ((fp = fopen(fname, "r")) == NULL) {
	fprintf(stderr, "couldn't open batch file %s\n", fname);
	return 0;
    }
    while (getline(&lbuf, &len, fp) > -1) {
	slen = strlen(lbuf);
	if (slen && (lbuf[slen - 1] == '\n')) {
	    lbuf[slen - 1] = 0;
	    printf("sis> %s\n", lbuf);
	    exec_cmd(sregs, lbuf);
	}
    }
    free(lbuf);
    fclose(fp);
    return 1;
}

void
set_regi(struct pstate *sregs, int32_t reg, uint32_t rval)
{
    uint32_t          cwp;

    cwp = ((sregs->psr & 0x7) << 4);
    if ((reg > 0) && (reg < 8)) {
	sregs->g[reg] = rval;
    } else if ((reg >= 8) && (reg < 32)) {
	sregs->r[(cwp + reg) & 0x7f] = rval;
    } else if ((reg >= 32) && (reg < 64)) {
	sregs->fsi[reg - 32] = rval;
    } else {
	switch (reg) {
	case 64:
	    sregs->y = rval;
	    break;
	case 65:
	    sregs->psr = rval;
	    break;
	case 66:
	    sregs->wim = rval;
	    break;
	case 67:
	    sregs->tbr = rval;
	    break;
	case 68:
	    sregs->pc = rval;
	    break;
	case 69:
	    sregs->npc = rval;
	    break;
	case 70:
	    sregs->fsr = rval;
	    set_fsr(rval);
	    break;
    default:break;
	}
    }
}

void
get_regi(struct pstate * sregs, int32_t reg, unsigned char *buf)
{
    uint32_t          cwp;
    uint32_t          rval = 0;

    cwp = ((sregs->psr & 0x7) << 4);
    if ((reg >= 0) && (reg < 8)) {
	rval = sregs->g[reg];
    } else if ((reg >= 8) && (reg < 32)) {
	rval = sregs->r[(cwp + reg) & 0x7f];
    } else if ((reg >= 32) && (reg < 64)) {
	rval = sregs->fsi[reg - 32];
    } else {
	switch (reg) {
	case 64:
	    rval = sregs->y;
	    break;
	case 65:
	    rval = sregs->psr;
	    break;
	case 66:
	    rval = sregs->wim;
	    break;
	case 67:
	    rval = sregs->tbr;
	    break;
	case 68:
	    rval = sregs->pc;
	    break;
	case 69:
	    rval = sregs->npc;
	    break;
	case 70:
	    rval = sregs->fsr;
	    break;
    default:break;
	}
    }
    buf[0] = (rval >> 24) & 0x0ff;
    buf[1] = (rval >> 16) & 0x0ff;
    buf[2] = (rval >> 8) & 0x0ff;
    buf[3] = rval & 0x0ff;
}


static void
set_rega(struct pstate *sregs, char *reg, uint32_t rval)
{
    uint32_t          cwp;
    int32_t           err = 0;

    cwp = ((sregs->psr & 0x7) << 4);
    if (strcmp(reg, "psr") == 0)
	sregs->psr = (rval = (rval & 0x00f03fff));
    else if (strcmp(reg, "tbr") == 0)
	sregs->tbr = (rval = (rval & 0xfffffff0));
    else if (strcmp(reg, "wim") == 0)
	sregs->wim = (rval = (rval & 0x0ff));
    else if (strcmp(reg, "y") == 0)
	sregs->y = rval;
    else if (strcmp(reg, "pc") == 0)
	sregs->pc = rval;
    else if (strcmp(reg, "npc") == 0)
	sregs->npc = rval;
    else if (strcmp(reg, "fsr") == 0) {
	sregs->fsr = rval;
	set_fsr(rval);
    } else if (strcmp(reg, "g0") == 0)
	err = 2;
    else if (strcmp(reg, "g1") == 0)
	sregs->g[1] = rval;
    else if (strcmp(reg, "g2") == 0)
	sregs->g[2] = rval;
    else if (strcmp(reg, "g3") == 0)
	sregs->g[3] = rval;
    else if (strcmp(reg, "g4") == 0)
	sregs->g[4] = rval;
    else if (strcmp(reg, "g5") == 0)
	sregs->g[5] = rval;
    else if (strcmp(reg, "g6") == 0)
	sregs->g[6] = rval;
    else if (strcmp(reg, "g7") == 0)
	sregs->g[7] = rval;
    else if (strcmp(reg, "o0") == 0)
	sregs->r[(cwp + 8) & 0x7f] = rval;
    else if (strcmp(reg, "o1") == 0)
	sregs->r[(cwp + 9) & 0x7f] = rval;
    else if (strcmp(reg, "o2") == 0)
	sregs->r[(cwp + 10) & 0x7f] = rval;
    else if (strcmp(reg, "o3") == 0)
	sregs->r[(cwp + 11) & 0x7f] = rval;
    else if (strcmp(reg, "o4") == 0)
	sregs->r[(cwp + 12) & 0x7f] = rval;
    else if (strcmp(reg, "o5") == 0)
	sregs->r[(cwp + 13) & 0x7f] = rval;
    else if (strcmp(reg, "o6") == 0)
	sregs->r[(cwp + 14) & 0x7f] = rval;
    else if (strcmp(reg, "o7") == 0)
	sregs->r[(cwp + 15) & 0x7f] = rval;
    else if (strcmp(reg, "l0") == 0)
	sregs->r[(cwp + 16) & 0x7f] = rval;
    else if (strcmp(reg, "l1") == 0)
	sregs->r[(cwp + 17) & 0x7f] = rval;
    else if (strcmp(reg, "l2") == 0)
	sregs->r[(cwp + 18) & 0x7f] = rval;
    else if (strcmp(reg, "l3") == 0)
	sregs->r[(cwp + 19) & 0x7f] = rval;
    else if (strcmp(reg, "l4") == 0)
	sregs->r[(cwp + 20) & 0x7f] = rval;
    else if (strcmp(reg, "l5") == 0)
	sregs->r[(cwp + 21) & 0x7f] = rval;
    else if (strcmp(reg, "l6") == 0)
	sregs->r[(cwp + 22) & 0x7f] = rval;
    else if (strcmp(reg, "l7") == 0)
	sregs->r[(cwp + 23) & 0x7f] = rval;
    else if (strcmp(reg, "i0") == 0)
	sregs->r[(cwp + 24) & 0x7f] = rval;
    else if (strcmp(reg, "i1") == 0)
	sregs->r[(cwp + 25) & 0x7f] = rval;
    else if (strcmp(reg, "i2") == 0)
	sregs->r[(cwp + 26) & 0x7f] = rval;
    else if (strcmp(reg, "i3") == 0)
	sregs->r[(cwp + 27) & 0x7f] = rval;
    else if (strcmp(reg, "i4") == 0)
	sregs->r[(cwp + 28) & 0x7f] = rval;
    else if (strcmp(reg, "i5") == 0)
	sregs->r[(cwp + 29) & 0x7f] = rval;
    else if (strcmp(reg, "i6") == 0)
	sregs->r[(cwp + 30) & 0x7f] = rval;
    else if (strcmp(reg, "i7") == 0)
	sregs->r[(cwp + 31) & 0x7f] = rval;
    else
	err = 1;
    switch (err) {
    case 0:
	printf("%s = %d (0x%08x)\n", reg, rval, rval);
	break;
    case 1:
	printf("no such regiser: %s\n", reg);
	break;
    case 2:
	printf("cannot set g0\n");
	break;
    default:
	break;
    }

}

static void
disp_reg(struct pstate *sregs, char *reg)
{
    if (strncmp(reg, "w",1) == 0)
	disp_regs(sregs, VAL(&reg[1]));
}

#ifdef ERRINJ

void
errinj (int32_t arg ATTRIBUTE_UNUSED)
{
    int	err;

    switch (err = (random() % 12)) {
	case 0: errtt = 0x61; break;
	case 1: errtt = 0x62; break;
	case 2: errtt = 0x63; break;
	case 3: errtt = 0x64; break;
	case 4: errtt = 0x65; break;
	case 5: 
	case 6: 
	case 7: errftt = err; 
		break;
	case 8: errmec = 1; break;
	case 9: errmec = 2; break;
	case 10: errmec = 5; break;
	case 11: errmec = 6; break;
    }
    errcnt++;
    if (errper) event(errinj, 0, (random()%errper));
}

void
errinjstart (void)
{
    if (errper) event(errinj, 0, (random()%errper));
}

#endif

static uint32_t
limcalc (float32 freq)
{
    uint32_t          unit, lim;
    double	    flim;
    char           *cmd1, *cmd2;

    unit = 1;
    lim = -1;
    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
        lim = VAL(cmd1);
        if ((cmd2 = strtok(NULL, " \t\n\r")) != NULL) {
            if (strcmp(cmd2,"us")==0) unit = 1;
      	    if (strcmp(cmd2,"ms")==0) unit = 1000;
            if (strcmp(cmd2,"s")==0)  unit = 1000000;
        }
        flim = (double) lim * (double) unit * (double) freq + 
	   (double) ebase.simtime;
        if ((flim > ebase.simtime) && (flim < 4294967296.0)) {
            lim = (uint32_t) flim;
        } else  {
            printf("error in expression\n");
            lim = -1;
        }
    }
    return lim;
}

int
exec_cmd(struct pstate *sregs, const char *cmd)
{
    char           *cmd1, *cmd2;
    int32_t           stat;
    uint32_t          len, i, clen, j;
    static uint32_t   daddr = 0;
    char           *cmdsave, *cmdsave2 = NULL;

    stat = OK;
    cmdsave = strdup(cmd);
    cmdsave2 = strdup (cmd);
    if ((cmd1 = strtok (cmdsave2, " \t")) != NULL) {
	clen = strlen(cmd1);
	if (strncmp(cmd1, "bp", clen) == 0) {
	    for (i = 0; i < sregs->bptnum; i++) {
		printf("  %d : 0x%08x\n", i + 1, sregs->bpts[i]);
	    }
	} else if (strncmp(cmd1, "+bp", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		sregs->bpts[sregs->bptnum] = VAL(cmd1) & ~0x3;
		printf("added breakpoint %d at 0x%08x\n",
		       sregs->bptnum + 1, sregs->bpts[sregs->bptnum]);
		sregs->bptnum += 1;
	    }
	} else if (strncmp(cmd1, "-bp", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		i = VAL(cmd1) - 1;
		if ((i >= 0) && (i < sregs->bptnum)) {
		    printf("deleted breakpoint %d at 0x%08x\n", i + 1,
			   sregs->bpts[i]);
		    for (; i < sregs->bptnum - 1; i++) {
			sregs->bpts[i] = sregs->bpts[i + 1];
		    }
		    sregs->bptnum -= 1;
		}
	    }
	} else if (strncmp(cmd1, "batch", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) == NULL) {
		printf("no file specified\n");
	    } else {
		batch(sregs, cmd1);
	    }
	} else if (strncmp(cmd1, "cont", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) == NULL) {
		stat = run_sim(sregs, UINT64_MAX, 0);
	    } else {
		stat = run_sim(sregs, VAL(cmd1), 0);
	    }
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "debug", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		sis_verbose = VAL(cmd1);
	    }
	    printf("Debug level = %d\n",sis_verbose);
	} else if (strncmp(cmd1, "dis", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		daddr = VAL(cmd1);
	    }
	    if ((cmd2 = strtok(NULL, " \t\n\r")) != NULL) {
		len = VAL(cmd2);
	    } else
		len = 16;
	    printf("\n");
	    dis_mem(daddr, len, &dinfo);
	    printf("\n");
	    daddr += len * 4;
	} else if (strncmp(cmd1, "echo", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		printf("%s\n", (&cmdsave[clen+1]));
	    }
#ifdef ERRINJ
	} else if (strncmp(cmd1, "error", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		errper = VAL(cmd1);
	        if (errper) {
		    event(errinj, 0, (len = (random()%errper)));
		    printf("Error injection started with period %d\n",len);
	        } 
	     } else printf("Injected errors: %d\n",errcnt);
#endif
	} else if (strncmp(cmd1, "float", clen) == 0) {
	    stat = disp_fpu(sregs);
	} else if (strncmp(cmd1, "go", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) == NULL) {
		len = last_load_addr;
	    } else {
		len = VAL(cmd1);
	    }
	    sregs->pc = len & ~3;
	    sregs->npc = sregs->pc + 4;
	    if ((sregs->pc != 0) && (ebase.simtime == 0))
	        boot_init();
	    printf("resuming at 0x%08x\n",sregs->pc);
	    if ((cmd2 = strtok(NULL, " \t\n\r")) != NULL) {
		stat = run_sim(sregs, VAL(cmd2), 0);
	    } else {
		stat = run_sim(sregs, UINT64_MAX, 0);
	    }
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "help", clen) == 0) {
	    gen_help();
	} else if (strncmp(cmd1, "history", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		sregs->histlen = VAL(cmd1);
		if (sregs->histbuf != NULL)
		    free(sregs->histbuf);
		sregs->histbuf = (struct histype *) calloc(sregs->histlen, sizeof(struct histype));
		printf("trace history length = %d\n\r", sregs->histlen);
		sregs->histind = 0;

	    } else {
		j = sregs->histind;
		for (i = 0; i < sregs->histlen; i++) {
		    if (j >= sregs->histlen)
			j = 0;
		    printf(" %8d ", sregs->histbuf[j].time);
		    dis_mem(sregs->histbuf[j].addr, 1, &dinfo);
		    j++;
		}
	    }

	} else if (strncmp(cmd1, "load", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		last_load_addr = bfd_load(cmd1);
		while ((cmd1 = strtok(NULL, " \t\n\r")) != NULL)
		    last_load_addr = bfd_load(cmd1);
	    } else {
		printf("load: no file specified\n");
	    }
	} else if (strncmp(cmd1, "mem", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL)
		daddr = VAL(cmd1);
	    if ((cmd2 = strtok(NULL, " \t\n\r")) != NULL)
		len = VAL(cmd2);
	    else
		len = 64;
	    disp_mem(daddr, len);
	    daddr += len;
	} else if (strncmp(cmd1, "perf", clen) == 0) {
	    cmd1 = strtok(NULL, " \t\n\r");
	    if ((cmd1 != NULL) &&
		(strncmp(cmd1, "reset", strlen(cmd1)) == 0)) {
		reset_stat(sregs);
	    } else
		show_stat(sregs);
	} else if (strncmp(cmd1, "quit", clen) == 0) {
	    exit(0);
	} else if (strncmp(cmd1, "reg", clen) == 0) {
	    cmd1 = strtok(NULL, " \t\n\r");
	    cmd2 = strtok(NULL, " \t\n\r");
	    if (cmd2 != NULL)
		set_rega(sregs, cmd1, VAL(cmd2));
	    else if (cmd1 != NULL)
		disp_reg(sregs, cmd1);
	    else {
		disp_regs(sregs,sregs->psr);
		disp_ctrl(sregs);
	    }
	} else if (strncmp(cmd1, "reset", clen) == 0) {
	    ebase.simtime = 0;
	    reset_all();
	    reset_stat(sregs);
	} else if (strncmp(cmd1, "run", clen) == 0) {
	    ebase.simtime = 0;
	    reset_all();
	    reset_stat(sregs);
	    if ((cmd1 = strtok(NULL, " \t\n\r")) == NULL) {
		stat = run_sim(sregs, UINT64_MAX, 0);
	    } else {
		stat = run_sim(sregs, VAL(cmd1), 0);
	    }
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "shell", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) != NULL) {
		if (system(&cmdsave[clen])) {
		    /* Silence unused return value warning.  */
		}
	    }
	} else if (strncmp(cmd1, "step", clen) == 0) {
	    stat = run_sim(sregs, 1, 1);
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "tcont", clen) == 0) {
	    sregs->tlimit = limcalc(sregs->freq);
	    stat = run_sim(sregs, UINT64_MAX, 0);
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "tgo", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) == NULL) {
		len = last_load_addr;
	    } else {
		len = VAL(cmd1);
	        sregs->tlimit = limcalc(sregs->freq);
	    }
	    sregs->pc = len & ~3;
	    sregs->npc = sregs->pc + 4;
	    printf("resuming at 0x%08x\n",sregs->pc);
	    stat = run_sim(sregs, UINT64_MAX, 0);
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "tlimit", clen) == 0) {
	   sregs->tlimit = limcalc(sregs->freq);
	   if (sregs->tlimit != (uint32_t) -1)
              printf("simulation limit = %u (%.3f ms)\n",(uint32_t) sregs->tlimit,
		sregs->tlimit / sregs->freq / 1000);
	} else if (strncmp(cmd1, "tra", clen) == 0) {
	    if ((cmd1 = strtok(NULL, " \t\n\r")) == NULL) {
		stat = run_sim(sregs, UINT64_MAX, 1);
	    } else {
		stat = run_sim(sregs, VAL(cmd1), 1);
	    }
	    printf("\n");
	    daddr = sregs->pc;
	    sim_halt();
	} else if (strncmp(cmd1, "trun", clen) == 0) {
	    ebase.simtime = 0;
	    reset_all();
	    reset_stat(sregs);
	    sregs->tlimit = limcalc(sregs->freq);
	    stat = run_sim(sregs, UINT64_MAX, 0);
	    daddr = sregs->pc;
	    sim_halt();
	} else
	    printf("syntax error\n");
    }
    if (cmdsave2 != NULL)
	free(cmdsave2);
    if (cmdsave != NULL)
	free(cmdsave);
    return stat;
}


void
reset_stat(struct pstate *sregs)
{
    sregs->tottime = 0.0;
    sregs->pwdtime = 0;
    sregs->ninst = 0;
    sregs->fholdt = 0;
    sregs->holdt = 0;
    sregs->icntt = 0;
    sregs->finst = 0;
    sregs->nstore = 0;
    sregs->nload = 0;
    sregs->nbranch = 0;
    sregs->simstart = ebase.simtime;

}

void
show_stat(struct pstate *sregs)
{
#ifdef STAT
    uint32_t          iinst;
#endif
    uint32_t          stime;

    if (sregs->tottime == 0.0)
        sregs->tottime += 1E-6;
    stime = ebase.simtime - sregs->simstart;	/* Total simulated time */
#ifdef STAT

    iinst = sregs->ninst - sregs->finst - sregs->nload - sregs->nstore -
	sregs->nbranch;
#endif

    printf("\n Cycles       : %9" PRIu64 "\n\r", ebase.simtime - sregs->simstart);
    printf(" Instructions : %9" PRIu64 "\n", sregs->ninst);

#ifdef STAT
    printf("   integer    : %9.2f %%\n", 100.0 * (float) iinst / (float) sregs->ninst);
    printf("   load       : %9.2f %%\n",
	   100.0 * (float) sregs->nload / (float) sregs->ninst);
    printf("   store      : %9.2f %%\n",
	   100.0 * (float) sregs->nstore / (float) sregs->ninst);
    printf("   branch     : %9.2f %%\n",
	   100.0 * (float) sregs->nbranch / (float) sregs->ninst);
    printf("   float      : %9.2f %%\n",
	   100.0 * (float) sregs->finst / (float) sregs->ninst);
    printf(" Integer CPI  : %9.2f\n",
	   ((float) (stime - sregs->pwdtime - sregs->fholdt - sregs->finst))
	   /
	   (float) (sregs->ninst - sregs->finst));
    printf(" Float CPI    : %9.2f\n",
	   ((float) sregs->fholdt / (float) sregs->finst) + 1.0);
#endif
    printf(" Overall CPI  : %9.2f\n",
	   (float) (stime - sregs->pwdtime) / (float) sregs->ninst);
    printf("\n ERC32 performance (%4.1f MHz): %5.2f MOPS (%5.2f MIPS, %5.2f MFLOPS)\n",
	   sregs->freq, sregs->freq * (float) sregs->ninst / (float) (stime - sregs->pwdtime),
	   sregs->freq * (float) (sregs->ninst - sregs->finst) /
	   (float) (stime - sregs->pwdtime),
     sregs->freq * (float) sregs->finst / (float) (stime - sregs->pwdtime));
    printf(" Simulated ERC32 time        : %.2f s\n",
        (float) (ebase.simtime - sregs->simstart) / 1000000.0 / sregs->freq);
    printf(" Processor utilisation       : %.2f %%\n",
        100.0 * (1.0 - ((float) sregs->pwdtime / (float) stime)));
    printf(" Real-time performance       : %.2f %%\n",
        100.0 / (sregs->tottime / ((double) (stime) / (sregs->freq * 1.0E6))));
    printf(" Simulator performance       : %.2f MIPS\n",
        (double)(sregs->ninst) / sregs->tottime / 1E6);
    printf(" Used time (sys + user)      : %.2f s\n\n", sregs->tottime);
}



void
init_bpt(struct pstate *sregs)
{
    sregs->bptnum = 0;
    sregs->histlen = 0;
    sregs->histind = 0;
    sregs->histbuf = NULL;
    sregs->tlimit = -1;
}

static void
int_handler(int32_t sig)
{
    if (sig != 2)
	printf("\n\n Signal handler error  (%d)\n\n", sig);
    ctrl_c = 1;
}

void
init_signals(void)
{
    signal(SIGTERM, int_handler);
    signal(SIGINT, int_handler);
}


extern struct disassemble_info dinfo;

struct estate   ebase;
struct evcell   evbuf[EVENT_MAX];
struct irqcell  irqarr[16];

static int
disp_fpu(struct pstate *sregs)
{

    int         i;

    printf("\n fsr: %08X\n\n", sregs->fsr);

#ifdef HOST_LITTLE_ENDIAN
    for (i = 0; i < 32; i++)
      sregs->fdp[i ^ 1] = sregs->fs[i];
#endif

    for (i = 0; i < 32; i++) {
	printf(" f%02d  %08x  %14e  ", i, sregs->fsi[i], sregs->fs[i]);
	if (!(i & 1))
	    printf("%14e\n", sregs->fd[i >> 1]);
	else
	    printf("\n");
    }
    printf("\n");
    return OK;
}

static void
disp_regs(struct pstate *sregs, int cwp)
{

    int           i;

    cwp = ((cwp & 0x7) << 4);
    printf("\n\t  INS       LOCALS      OUTS     GLOBALS\n");
    for (i = 0; i < 8; i++) {
	printf("   %d:  %08X   %08X   %08X   %08X\n", i,
	       sregs->r[(cwp + i + 24) & 0x7f],
	    sregs->r[(cwp + i + 16) & 0x7f], sregs->r[(cwp + i + 8) & 0x7f],
	       sregs->g[i]);
    }
}

static void print_insn_sparc_sis(uint32_t addr, struct disassemble_info *info)
{
    unsigned char           i[4];

    sis_memory_read(addr, i, 4);
    dinfo.buffer_vma = addr;
    dinfo.buffer_length = 4;
    dinfo.buffer = i;
    print_insn_sparc(addr, info);
}

static void
disp_ctrl(struct pstate *sregs)
{

    uint32_t           i;

    printf("\n psr: %08X   wim: %08X   tbr: %08X   y: %08X\n",
	   sregs->psr, sregs->wim, sregs->tbr, sregs->y);
    sis_memory_read (sregs->pc, (char *) &i, 4);
    printf ("\n  pc: %08X = %08X    ", sregs->pc, i);
    print_insn_sparc_sis(sregs->pc, &dinfo);
    sis_memory_read (sregs->npc, (char *) &i, 4);
    printf ("\n npc: %08X = %08X    ", sregs->npc, i);
    print_insn_sparc_sis(sregs->npc, &dinfo);
    if (sregs->err_mode)
	printf("\n IU in error mode");
    printf("\n\n");
}

static void
disp_mem(uint32_t addr, uint32_t len)
{

    uint32_t          i;
    union {
	    unsigned char u8[4];
	    uint32_t u32;
    } data;
    uint32_t          mem[4], j;
    char           *p;

    for (i = addr & ~3; i < ((addr + len) & ~3); i += 16) {
	printf("\n %8X  ", i);
	for (j = 0; j < 4; j++) {
	    sis_memory_read ((i + (j * 4)), data.u8, 4);
	    printf ("%08x  ", data.u32);
	    mem[j] = data.u32;
	}
	printf("  ");
	p = (char *) mem;
	for (j = 0; j < 16; j++) {
	    if (isprint (p[j ^ EBT]))
		putchar (p[j ^ EBT]);
	    else
		putchar('.');
	}
    }
    printf("\n\n");
}

void
dis_mem(uint32_t addr, uint32_t len, struct disassemble_info *info)
{
    uint32_t          i;
    union {
	    unsigned char u8[4];
	    uint32_t u32;
    } data;

    for (i = addr & -3; i < ((addr & -3) + (len << 2)); i += 4) {
	sis_memory_read (i, data.u8, 4);
	printf (" %08x  %08x  ", i, data.u32);
	print_insn_sparc_sis(i, info);
        if (i >= 0xfffffffc) break;
	printf("\n");
    }
}

/* Add event to event queue */

void
event(void (*cfunc) (int32_t), int32_t arg, uint64_t delta)
{
    struct evcell  *ev1, *evins;

    if (ebase.freeq == NULL) {
	printf("Error, too many events in event queue\n");
	return;
    }
    ev1 = &ebase.eq;
    delta += ebase.simtime;
    while ((ev1->nxt != NULL) && (ev1->nxt->time <= delta)) {
	ev1 = ev1->nxt;
    }
    if (ev1->nxt == NULL) {
	ev1->nxt = ebase.freeq;
	ebase.freeq = ebase.freeq->nxt;
	ev1->nxt->nxt = NULL;
    } else {
	evins = ebase.freeq;
	ebase.freeq = ebase.freeq->nxt;
	evins->nxt = ev1->nxt;
	ev1->nxt = evins;
    }
    ev1->nxt->time = delta;
    ev1->nxt->cfunc = cfunc;
    ev1->nxt->arg = arg;
}

#if 0	/* apparently not used */
void
stop_event(void)
{
}
#endif

void
init_event(void)
{
    int32_t           i;

    ebase.eq.nxt = NULL;
    ebase.freeq = evbuf;
    for (i = 0; i < EVENT_MAX; i++) {
	evbuf[i].nxt = &evbuf[i + 1];
    }
    evbuf[EVENT_MAX - 1].nxt = NULL;
}

void
set_int(int32_t level, void (*callback) (int32_t), int32_t arg)
{
    irqarr[level & 0x0f].callback = callback;
    irqarr[level & 0x0f].arg = arg;
}

/* Advance simulator time */

void
advance_time(struct pstate *sregs)
{

    struct evcell  *evrem;
    void            (*cfunc) (int32_t);
    int32_t           arg;
    uint64_t          endtime;

#ifdef STAT
    sregs->fholdt += sregs->fhold;
    sregs->holdt += sregs->hold;
    sregs->icntt += sregs->icnt;
#endif

    endtime = ebase.simtime + sregs->icnt + sregs->hold + sregs->fhold;

    while ((ebase.eq.nxt->time <= (endtime)) && (ebase.eq.nxt != NULL)) {
	ebase.simtime = ebase.eq.nxt->time;
	cfunc = ebase.eq.nxt->cfunc;
	arg = ebase.eq.nxt->arg;
	evrem = ebase.eq.nxt;
	ebase.eq.nxt = ebase.eq.nxt->nxt;
	evrem->nxt = ebase.freeq;
	ebase.freeq = evrem;
	cfunc(arg);
    }
    ebase.simtime = endtime;

}

uint32_t
now(void)
{
    return ebase.simtime;
}


/* Advance time until an external interrupt is seen */

int
wait_for_irq(void)
{
    struct evcell  *evrem;
    void            (*cfunc) (int32_t);
    int32_t           arg;
    uint64_t          endtime;

    if (ebase.eq.nxt == NULL)
	printf("Warning: event queue empty - power-down mode not entered\n");
    endtime = ebase.simtime;
    while (!ext_irl && (ebase.eq.nxt != NULL)) {
	ebase.simtime = ebase.eq.nxt->time;
	cfunc = ebase.eq.nxt->cfunc;
	arg = ebase.eq.nxt->arg;
	evrem = ebase.eq.nxt;
	ebase.eq.nxt = ebase.eq.nxt->nxt;
	evrem->nxt = ebase.freeq;
	ebase.freeq = evrem;
	cfunc(arg);
	if (ctrl_c) {
	    printf("\bwarning: power-down mode interrupted\n");
	    break;
	}
    }
    sregs.pwdtime += ebase.simtime - endtime;
    return ebase.simtime - endtime;
}

int
check_bpt(struct pstate *sregs)
{
    int32_t           i;

    if ((sregs->bphit) || (sregs->annul))
	return 0;
    for (i = 0; i < (int32_t) sregs->bptnum; i++) {
	if (sregs->pc == sregs->bpts[i])
	    return BPT_HIT;
    }
    return 0;
}

void
reset_all(void)
{
    init_event();		/* Clear event queue */
    init_regs(&sregs);
    reset();
#ifdef ERRINJ
    errinjstart();
#endif
}

void
sys_reset(void)
{
    reset_all();
    sregs.trap = 256;		/* Force fake reset trap */
}

void
sys_halt(void)
{
    sregs.trap = 257;           /* Force fake halt trap */
}

#include "ansidecl.h"

#include <stdarg.h>

#include "libiberty.h"
#include "bfd.h"

#ifndef min
#define min(A, B) (((A) < (B)) ? (A) : (B))
#endif
#define LOAD_ADDRESS 0

int
bfd_load (const char *fname)
{
    asection       *section;
    bfd            *pbfd;
    int            i;

    pbfd = bfd_openr(fname, 0);

    if (pbfd == NULL) {
	printf("open of %s failed\n", fname);
	return -1;
    }
    if (!bfd_check_format(pbfd, bfd_object)) {
	printf("file %s  doesn't seem to be an object file\n", fname);
	return -1;
    }

    if (sis_verbose)
	printf("loading %s:", fname);
    for (section = pbfd->sections; section; section = section->next) {
	if (bfd_section_flags (section) & SEC_ALLOC) {
	    bfd_vma         section_address;
	    unsigned long   section_size;
	    const char     *section_name;

	    section_name = bfd_section_name (section);

	    section_address = bfd_section_vma (section);
	    /*
	     * Adjust sections from a.out files, since they don't carry their
	     * addresses with.
	     */
	    if (bfd_get_flavour(pbfd) == bfd_target_aout_flavour) {
		if (strcmp (section_name, ".text") == 0)
		    section_address = bfd_get_start_address (pbfd);
		else if (strcmp (section_name, ".data") == 0) {
		    /* Read the first 8 bytes of the data section.
		       There should be the string 'DaTa' followed by
		       a word containing the actual section address. */
		    struct data_marker
		    {
			char signature[4];	/* 'DaTa' */
			unsigned char sdata[4];	/* &sdata */
		    } marker;
		    bfd_get_section_contents (pbfd, section, &marker, 0,
					      sizeof (marker));
		    if (strncmp (marker.signature, "DaTa", 4) == 0)
		      {
			section_address = bfd_getb32 (marker.sdata);
		      }
		}
	    }

	    section_size = bfd_section_size (section);

	    if (sis_verbose)
		printf("\nsection %s at 0x%08" PRIx64 " (0x%lx bytes)",
		       section_name, (uint64_t) section_address, section_size);

	    /* Text, data or lit */
	    if (bfd_section_flags (section) & SEC_LOAD) {
		file_ptr        fptr;

		fptr = 0;

		while (section_size > 0) {
		    char            buffer[1024];
		    int             count;

		    count = min(section_size, 1024);

		    bfd_get_section_contents(pbfd, section, buffer, fptr, count);

		    for (i = 0; i < count; i++)
			sis_memory_write ((section_address + i) ^ EBT, &buffer[i], 1);

		    section_address += count;
		    fptr += count;
		    section_size -= count;
		}
	    } else		/* BSS */
		if (sis_verbose)
		    printf("(not loaded)");
	}
    }
    if (sis_verbose)
	printf("\n");

    return bfd_get_start_address (pbfd);
}

double get_time (void)
{
    double usec;

    struct timeval tm;

    gettimeofday (&tm, NULL);
    usec = ((double) tm.tv_sec) * 1E6 + ((double) tm.tv_usec);
    return usec / 1E6;
}
