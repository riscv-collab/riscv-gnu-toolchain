#as: -march=rv64i
#name: Lx/Sx macro insns
#objdump: -dwr

.*:[ 	]+file format .*


Disassembly of section .text:

0+ <L>:
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+bval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00050503[ 	]+lb[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+bval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00054503[ 	]+lbu[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+hval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00051503[ 	]+lh[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+hval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00055503[ 	]+lhu[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+wval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00052503[ 	]+lw[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+wval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00056503[ 	]+lwu[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000517[ 	]+auipc[ 	]+a0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+dval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00053503[ 	]+ld[ 	]+a0,0\(a0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_I[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*

[0-9a-f]+ <S>:
[ 	]+[0-9a-f]+:[ 	]+00000297[ 	]+auipc[ 	]+t0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+bval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00a28023[ 	]+sb[ 	]+a0,0\(t0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_S[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000297[ 	]+auipc[ 	]+t0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+hval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00a29023[ 	]+sh[ 	]+a0,0\(t0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_S[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000297[ 	]+auipc[ 	]+t0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+wval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00a2a023[ 	]+sw[ 	]+a0,0\(t0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_S[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00000297[ 	]+auipc[ 	]+t0,0x0[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_HI20[ 	]+dval
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
[ 	]+[0-9a-f]+:[ 	]+00a2b023[ 	]+sd[ 	]+a0,0\(t0\) # [0-9a-f]+( <.*>)?[ 	]+[0-9a-f]+:[ 	]+R_RISCV_PCREL_LO12_S[ 	]+.*
[ 	]+[0-9a-f]+:[ 	]+R_RISCV_RELAX.*
