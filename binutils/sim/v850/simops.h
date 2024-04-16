#ifndef SIMOPS_H
#define SIMOPS_H

#include "sim-fpu.h"

int OP_380 (void);
int OP_480 (void);
int OP_501 (void);
int OP_700 (void);
int OP_720 (void);
int OP_10720 (void);
int OP_740 (void);
int OP_760 (void);
int OP_10760 (void);
int OP_1C0 (void);
int OP_240 (void);
int OP_600 (void);
int OP_1A0 (void);
int OP_180 (void);
int OP_E0 (void);
int OP_2E0 (void);
int OP_6E0 (void);
int OP_1E0 (void);
int OP_260 (void);
int OP_7E0 (void);
int OP_C0 (void);
int OP_220 (void);
int OP_A0 (void);
int OP_660 (void);
int OP_80 (void);
int OP_160 (void);
int OP_200 (void);
int OP_640 (void);
int OP_2A0 (void);
int OP_A007E0 (void);
int OP_2C0 (void);
int OP_C007E0 (void);
int OP_280 (void);
int OP_8007E0 (void);
int OP_100 (void);
int OP_680 (void);
int OP_140 (void);
int OP_6C0 (void);
int OP_120 (void);
int OP_6A0 (void);
int OP_20 (void);
int OP_7C0 (void);
int OP_47C0 (void);
int OP_87C0 (void);
int OP_C7C0 (void);
int OP_16007E0 (void);
int OP_16087E0 (void);
int OP_12007E0 (void);
int OP_10007E0 (void);
int OP_E607E0 (void);
int OP_22207E0 (void);
int OP_E407E0 (void);
int OP_E207E0 (void);
int OP_E007E0 (void);
int OP_20007E0 (void);
int OP_1C207E0 (void);
int OP_1C007E0 (void);
int OP_18207E0 (void);
int OP_18007E0 (void);
int OP_2C207E0 (void);
int OP_2C007E0 (void);
int OP_28207E0 (void);
int OP_28007E0 (void);
int OP_24207E0 (void);
int OP_24007E0 (void);
int OP_107E0 (void);
int OP_10780 (void);
int OP_1B0780 (void);
int OP_130780 (void);
int OP_B0780 (void);
int OP_30780 (void);
int OP_22007E0 (void);
int OP_307F0 (void);
int OP_107F0 (void);
int OP_307E0 (void);

int v850_float_compare(SIM_DESC sd, int cmp, sim_fpu wop1, sim_fpu wop2, int double_op_p);

/* MEMORY ACCESS */
uint32_t load_data_mem(SIM_DESC sd, address_word addr, int len);
void store_data_mem(SIM_DESC sd, address_word addr, int len, uint32_t data);

unsigned long Add32 (unsigned long a1, unsigned long a2, int * carry);

/* FPU */

/*
  FPU: update FPSR flags
  invalid, inexact, overflow, underflow
 */

extern void check_invalid_snan (SIM_DESC sd, sim_fpu_status, unsigned int);

#define check_cvt_fi(sd, status, double_op_p)                   \
        update_fpsr (sd, status, FPSR_XEV | FPSR_XEI, double_op_p)

#define check_cvt_if(sd, status, double_op_p)                   \
        update_fpsr (sd, status, FPSR_XEI, double_op_p)

#define check_cvt_ff(sd, status, double_op_p)                   \
        update_fpsr (sd, status, FPSR_XEV | FPSR_XEI | FPSR_XEO | FPSR_XEU, double_op_p)

extern void update_fpsr (SIM_DESC sd, sim_fpu_status, unsigned int, unsigned int);


/*
  Exception 
 */
void  SignalException (SIM_DESC sd);
void  SignalExceptionFPE (SIM_DESC sd, unsigned int double_op_p);

int mpu_load_mem_test (SIM_DESC sd, unsigned int addr, int len, int base_reg);
int mpu_store_mem_test (SIM_DESC sd, unsigned int addr, int len, int base_reg);

void v850_sar (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p);
void v850_shl (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p);
void v850_rotl (SIM_DESC sd, unsigned int, unsigned int, unsigned int *);
void v850_bins (SIM_DESC sd, unsigned int, unsigned int, unsigned int, unsigned int *);
void v850_shr (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p);
void v850_satadd (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p);
void v850_satsub (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p);
void v850_div (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p, unsigned int *op3p);
void v850_divu (SIM_DESC sd, unsigned int op0, unsigned int op1, unsigned int *op2p, unsigned int *op3p);

#endif
