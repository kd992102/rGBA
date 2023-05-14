#include "memory.h"

enum DECODE_MODE{
    ARM_MODE = 0,THUMB_MODE
};

enum COND{
    EQ = 0,NE,CS,CC,MI,PL,VS,VC,HI,LS,GE,LT,GT,LE,AL
};

enum ARM_MODE_REG{
    R0 = 0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,SP,LR,PC,CPSR
};

enum CPU_MODE{
    USER = 0, FIQ, IRQ, SVC, ABT, UNDEF, SYSTEM
};

typedef enum boolean{
    false, true
} bool;

typedef struct arm7tdmi{
    enum CPU_MODE CpuMode;
    enum DECODE_MODE dMode;
    enum COND Cond;
    uint32_t Reg[15];
    uint32_t CPSR;
    uint32_t SPSR;
    uint32_t Ptr;
    uint8_t carry_out;
    uint8_t InstOffset;
    uint32_t fetchcache[3];
    Gba_Memory *GbaMem;
} Gba_Cpu;

//uint32_t FileSize(FILE *ptr);
void InitCpu(Gba_Cpu *cpu, uint32_t BaseAddr);
uint32_t MemRead(Gba_Cpu *cpu, uint32_t addr);
void MemWrite(Gba_Cpu *cpu, uint32_t addr, uint32_t data);
void PreFetch(Gba_Cpu *cpu, uint32_t Addr);
void PreFetchAbort(Gba_Cpu *cpu);
uint8_t CheckCond(Gba_Cpu *cpu);
uint16_t ThumbModeDecode(Gba_Cpu *cpu, uint16_t inst);
uint32_t ArmModeDecode(Gba_Cpu *cpu, uint32_t inst);
void ArmDataProc(Gba_Cpu *cpu, uint32_t inst);
uint32_t CpuDecode(Gba_Cpu *cpu, uint32_t inst);
void CpuStatus(Gba_Cpu *cpu);