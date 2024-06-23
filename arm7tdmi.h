#include <stdint.h>
#define A_ADD 1
#define A_SUB 0
#define LOG 2
#define MOVS 3

#define BIT(a) ((1 >> a) & 1) 

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

    uint8_t saveMode;

    uint8_t carry_out;
    uint8_t InstOffset;
    uint32_t fetchcache[3];
    uint64_t cycle;
    uint16_t frame_cycle;
    uint64_t cycle_sum;
    uint32_t Cmode;
    uint32_t CurrentInst;
    uint32_t CurrentInstAddr[3];
    uint8_t Halt;
    uint8_t DebugFunc;
} Gba_Cpu;

typedef enum IRQ_VECTOR IrqVec;

enum IRQ_VECTOR {
    LCD_VBLANK = (1<<0),
    LCD_HBLANK = (1<<1),
    LCD_VCOUNT = (1<<2),
    TIMER0OF = (1<<3),
    TIMER1OF = (1<<4),
    TIMER2OF = (1<<5),
    TIMER3OF = (1<<6),
    SERIAL = (1<<7),
    DMA0 = (1<<8),
    DMA1 = (1<<9),
    DMA2 = (1<<10),
    DMA3 = (1<<11),
    KEYPAD = (1<<12),
    GAMEPAK = (1<<13)
};

static int arm_signed_mul_extra_cycles(uint32_t Rd)
{
    uint32_t temp = Rd;

    if (temp & BIT(31))
        temp = ~temp; // All 0 or all 1

    if (temp & 0xFFFFFF00)
    {
        if (temp & 0xFFFF0000)
        {
            if (temp & 0xFF000000){return 4;}
            else{return 3;}
        }
        else{return 2;}
    }
    else{return 1;}
}

static int arm_unsigned_mul_extra_cycles(uint32_t Rd)
{
    uint32_t temp = Rd; // All 0

    if (temp & 0xFFFFFF00)
    {
        if (temp & 0xFFFF0000)
        {
            if (temp & 0xFF000000){return 4;}
            else{return 3;}
        }
        else{return 2;}
    }
    else{return 1;}
}

uint8_t CheckCond();
void CPSRUpdate(uint8_t Opcode, uint32_t result, uint32_t parameterA, uint32_t parameterB);

void pipeline_flush(Gba_Cpu *cpu);
void ErrorHandler(Gba_Cpu *cpu);
void RecoverReg(uint8_t Cmode);
uint8_t ChkCPUMode();
void CpuExecuteArm(uint32_t cycles);
void CpuExecuteThumb(uint32_t cycles);
uint32_t CpuExecute(uint32_t cycles);
uint32_t CpuExecuteFrame(uint32_t clocks);
void CpuStatus();
void PreFetch(uint32_t Addr);
void ThumbModeDecode(uint16_t inst);
void ArmModeDecode(uint32_t inst);
void IRQ_checker(uint32_t CPSR);
void IRQ_handler();