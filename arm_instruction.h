#include <stdint.h>
enum DataProcOPCode{
    AND, EOR, SUB, RSB, ADD, ADC, SBC, RSC, TST, TEQ, CMP, CMN, ORR, MOV, BIC, MVN
};

enum ShiftType{
    LSL, LSR, ASR, ROR
};
uint32_t BarrelShifter(uint32_t Opr, uint8_t Imm, uint32_t result);
void ArmDataProc(uint32_t inst);
void ArmBranch(uint32_t inst);
void ArmBX(uint32_t inst);
void ArmSWP(uint32_t inst);
void ArmMUL(uint32_t inst);
void ArmMULL(uint32_t inst);
void ArmSWI(uint32_t inst);
void ArmSDT(uint32_t inst);
void ArmSDTS(uint32_t inst);
void ArmUDF(uint32_t inst);
void ArmBDT(uint32_t inst);
void ArmPSRT(uint32_t inst);