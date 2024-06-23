#include <stdint.h>
//uint32_t FileSize(FILE *ptr);
typedef enum InterruptMask IntMask;

void InitCpu(uint32_t BaseAddr);
//Exeception
void Reset();

void RunFrame();//in ppu.c