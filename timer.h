#include <stdint.h>
//For Data Use
//儲存資料使用
#define TM0_D 0x4000100
#define TM1_D 0x4000104
#define TM2_D 0x4000108
#define TM3_D 0x400010C

//For Control Use
//控制使用
#define TM0CNT 0x4000102
#define TM1CNT 0x4000106
#define TM2CNT 0x400010A
#define TM3CNT 0x400010E

#define TMR_CASCADE (1 << 2)
#define TMR_IRQ (1 << 6)
#define TMR_ENB (1 << 7)

//16bit
struct TimerControl {
    uint8_t TM_FREQ_Y;
    uint8_t TM_CASCADE;
    uint8_t TM_IRQ;
    uint8_t TM_ENABLE;
};

void Timer_Clock(uint32_t cycle);