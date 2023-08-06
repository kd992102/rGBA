#include <stdint.h>
#define A_ADD 1
#define A_SUB 0
#define LOG 2

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
    USER = 16,FIQ = 17,IRQ = 18,SVC = 19,ABT = 23,UNDEF = 27, SYSTEM = 31
};

typedef enum boolean{
    false, true
} bool;

typedef struct arm7tdmi{
    enum CPU_MODE CpuMode;
    enum DECODE_MODE dMode;
    enum COND Cond;

    //System & User
    uint32_t Reg[15];
    //FIQ
    uint32_t Reg_fiq[7];//8-14
    uint32_t SPSR_fiq;
    //Supervisor
    uint32_t Reg_svc[2];//13、14
    uint32_t SPSR_svc;
    //Abort
    uint32_t Reg_abt[2];//13、14
    uint32_t SPSR_abt;
    //IRQ
    uint32_t Reg_irq[2];//13、14
    uint32_t SPSR_irq;
    //Undefined
    uint32_t Reg_und[2];//13、14
    uint32_t SPSR_und;

    uint32_t CPSR;
    uint32_t SPSR;

    uint32_t Regbk[8];

    uint8_t carry_out;
    uint8_t InstOffset;
    uint32_t fetchcache[3];
    uint64_t cycle;
} Gba_Cpu;

uint8_t CheckCond();
void CPSRUpdate(uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB);


void RecoverReg(uint8_t Cmode);
uint8_t ChkCPUMode();
uint32_t CpuExecute(uint32_t inst);
void CpuStatus();
void PreFetch(uint32_t Addr);
void ThumbModeDecode(uint16_t inst);
void ArmModeDecode(uint32_t inst);