#include "arm7tdmi.h"

enum DataProcOPCode{
    AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN
};

enum ShiftType{
    LSL, LSR, ASR, ROR
};
uint32_t BarrelShifter(Gba_Cpu *cpu, uint32_t Opr, uint8_t Imm, uint32_t result);
void CPSRupdate(Gba_Cpu *cpu, uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB);
void PSRTransfer(Gba_Cpu *cpu, uint32_t inst);
void ArmDataProc(Gba_Cpu *cpu, uint32_t inst);
void ArmBranch(Gba_Cpu *cpu, uint32_t inst);
void ArmBX(Gba_Cpu *cpu, uint32_t inst);
void ArmSWP(Gba_Cpu *cpu, uint32_t inst);
void ArmMUL(Gba_Cpu *cpu, uint32_t inst);
void ArmMULL(Gba_Cpu *cpu, uint32_t inst);
void ArmSWI(Gba_Cpu *cpu, uint32_t inst);
void ArmSDT(Gba_Cpu *cpu, uint32_t inst);
void ArmSDTS(Gba_Cpu *cpu, uint32_t inst);
void ArmUDF(Gba_Cpu *cpu, uint32_t inst);
void ArmBDT(Gba_Cpu *cpu, uint32_t inst);
void ArmPSRT(Gba_Cpu *cpu, uint32_t inst);