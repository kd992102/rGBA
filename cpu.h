uint32_t CpuExecute(Gba_Cpu *cpu, uint32_t inst);
void CpuStatus(Gba_Cpu *cpu);
void PreFetch(Gba_Cpu *cpu, uint32_t Addr);
void ThumbModeDecode(Gba_Cpu *cpu, uint16_t inst, uint16_t inst2);
void ArmModeDecode(Gba_Cpu *cpu, uint32_t inst);