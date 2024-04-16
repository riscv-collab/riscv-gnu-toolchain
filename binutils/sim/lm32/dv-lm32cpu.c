/*  Lattice Mico32 CPU model.
    Contributed by Jon Beniston <jon@beniston.com>

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#define WANT_CPU lm32bf
#define WANT_CPU_LM32BF

/* This must come before any other includes.  */
#include "defs.h"

#include "hw-main.h"
#include "sim-main.h"


struct lm32cpu
{
  struct hw_event *event;
};

/* input port ID's.  */

enum
{
  INT0_PORT,
  INT1_PORT,
  INT2_PORT,
  INT3_PORT,
  INT4_PORT,
  INT5_PORT,
  INT6_PORT,
  INT7_PORT,
  INT8_PORT,
  INT9_PORT,
  INT10_PORT,
  INT11_PORT,
  INT12_PORT,
  INT13_PORT,
  INT14_PORT,
  INT15_PORT,
  INT16_PORT,
  INT17_PORT,
  INT18_PORT,
  INT19_PORT,
  INT20_PORT,
  INT21_PORT,
  INT22_PORT,
  INT23_PORT,
  INT24_PORT,
  INT25_PORT,
  INT26_PORT,
  INT27_PORT,
  INT28_PORT,
  INT29_PORT,
  INT30_PORT,
  INT31_PORT,
};

static const struct hw_port_descriptor lm32cpu_ports[] = {
  /* interrupt inputs.  */
  {"int0", INT0_PORT, 0, input_port,},
  {"int1", INT1_PORT, 0, input_port,},
  {"int2", INT2_PORT, 0, input_port,},
  {"int3", INT3_PORT, 0, input_port,},
  {"int4", INT4_PORT, 0, input_port,},
  {"int5", INT5_PORT, 0, input_port,},
  {"int6", INT6_PORT, 0, input_port,},
  {"int7", INT7_PORT, 0, input_port,},
  {"int8", INT8_PORT, 0, input_port,},
  {"int9", INT9_PORT, 0, input_port,},
  {"int10", INT10_PORT, 0, input_port,},
  {"int11", INT11_PORT, 0, input_port,},
  {"int12", INT12_PORT, 0, input_port,},
  {"int13", INT13_PORT, 0, input_port,},
  {"int14", INT14_PORT, 0, input_port,},
  {"int15", INT15_PORT, 0, input_port,},
  {"int16", INT16_PORT, 0, input_port,},
  {"int17", INT17_PORT, 0, input_port,},
  {"int18", INT18_PORT, 0, input_port,},
  {"int19", INT19_PORT, 0, input_port,},
  {"int20", INT20_PORT, 0, input_port,},
  {"int21", INT21_PORT, 0, input_port,},
  {"int22", INT22_PORT, 0, input_port,},
  {"int23", INT23_PORT, 0, input_port,},
  {"int24", INT24_PORT, 0, input_port,},
  {"int25", INT25_PORT, 0, input_port,},
  {"int26", INT26_PORT, 0, input_port,},
  {"int27", INT27_PORT, 0, input_port,},
  {"int28", INT28_PORT, 0, input_port,},
  {"int29", INT29_PORT, 0, input_port,},
  {"int30", INT30_PORT, 0, input_port,},
  {"int31", INT31_PORT, 0, input_port,},
  {NULL,},
};



/*
 * Finish off the partially created hw device.  Attach our local
 * callbacks.  Wire up our port names etc.  
 */
static hw_port_event_method lm32cpu_port_event;


static void
lm32cpu_finish (struct hw *me)
{
  struct lm32cpu *controller;

  controller = HW_ZALLOC (me, struct lm32cpu);
  set_hw_data (me, controller);
  set_hw_ports (me, lm32cpu_ports);
  set_hw_port_event (me, lm32cpu_port_event);

  /* Initialize the pending interrupt flags.  */
  controller->event = NULL;
}


/* An event arrives on an interrupt port.  */
static unsigned int s_ui_ExtIntrs = 0;


static void
deliver_lm32cpu_interrupt (struct hw *me, void *data)
{
  static unsigned int ip, im, im_and_ip_result;
  struct lm32cpu *controller = hw_data (me);
  SIM_DESC sd = hw_system (me);
  sim_cpu *cpu = STATE_CPU (sd, 0);	/* NB: fix CPU 0.  */


  HW_TRACE ((me, "interrupt-check event"));


  /*
   * Determine if an external interrupt is active 
   * and needs to cause an exception.
   */
  im = lm32bf_h_csr_get (cpu, LM32_CSR_IM);
  ip = lm32bf_h_csr_get (cpu, LM32_CSR_IP);
  im_and_ip_result = im & ip;


  if ((lm32bf_h_csr_get (cpu, LM32_CSR_IE) & 1) && (im_and_ip_result != 0))
    {
      /* Save PC in exception address register.  */
      lm32bf_h_gr_set (cpu, 30, lm32bf_h_pc_get (cpu));
      /* Restart at interrupt offset in handler exception table.  */
      lm32bf_h_pc_set (cpu,
		       lm32bf_h_csr_get (cpu,
					 LM32_CSR_EBA) +
		       LM32_EID_INTERRUPT * 32);
      /* Save interrupt enable and then clear.  */
      lm32bf_h_csr_set (cpu, LM32_CSR_IE, 0x2);
    }

  /* reschedule soon.  */
  if (controller->event != NULL)
    hw_event_queue_deschedule (me, controller->event);
  controller->event = NULL;


  /* if there are external interrupts, schedule an interrupt-check again.
   * NOTE: THIS MAKES IT VERY INEFFICIENT. INSTEAD, TRIGGER THIS
   * CHECk_EVENT WHEN THE USER ENABLES IE OR USER MODIFIES IM REGISTERS.
   */
  if (s_ui_ExtIntrs != 0)
    controller->event =
      hw_event_queue_schedule (me, 1, deliver_lm32cpu_interrupt, data);
}



/* Handle an event on one of the CPU's ports.  */
static void
lm32cpu_port_event (struct hw *me,
		    int my_port,
		    struct hw *source, int source_port, int level)
{
  struct lm32cpu *controller = hw_data (me);
  SIM_DESC sd = hw_system (me);
  sim_cpu *cpu = STATE_CPU (sd, 0);	/* NB: fix CPU 0.  */


  HW_TRACE ((me, "interrupt event on port %d, level %d", my_port, level));



  /* 
   * Activate IP if the interrupt's activated; don't do anything if
   * the interrupt's deactivated.
   */
  if (level == 1)
    {
      /*
       * save state of external interrupt.
       */
      s_ui_ExtIntrs |= (1 << my_port);

      /* interrupt-activated so set IP.  */
      lm32bf_h_csr_set (cpu, LM32_CSR_IP,
			lm32bf_h_csr_get (cpu, LM32_CSR_IP) | (1 << my_port));

      /* 
       * Since interrupt is activated, queue an immediate event
       * to check if this interrupt is serviceable.
       */
      if (controller->event != NULL)
	hw_event_queue_deschedule (me, controller->event);


      /* 
       * Queue an immediate event to check if this interrupt must be serviced;
       * this will happen after the current instruction is complete.
       */
      controller->event = hw_event_queue_schedule (me,
						   0,
						   deliver_lm32cpu_interrupt,
						   0);
    }
  else
    {
      /*
       * save state of external interrupt.
       */
      s_ui_ExtIntrs &= ~(1 << my_port);
    }
}


const struct hw_descriptor dv_lm32cpu_descriptor[] = {
  {"lm32cpu", lm32cpu_finish,},
  {NULL},
};
