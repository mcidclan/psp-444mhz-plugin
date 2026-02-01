#ifndef H_OVERCLOCK_PLUGIN_MAIN
#define H_OVERCLOCK_PLUGIN_MAIN

#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))

#define sync()      \
  asm volatile(     \
    "sync \n"       \
  )

#define delayPipeline()                    \
  asm volatile(                            \
    "nop; nop; nop; nop; nop; nop; nop \n" \
    "sync                              \n" \
  )

#define suspendCpuIntr(var) \
  asm volatile(             \
    "mfic  %0, $0 \n"       \
    "mtic  $0, $0 \n"       \
    "sync         \n"       \
    : "=r"(var)             \
    :                       \
    : "$8"                  \
  )

#define resumeCpuIntr(var)  \
  asm volatile(             \
    "mtic  %0, $0 \n"       \
    "sync         \n"       \
    :                       \
    : "r"(var)              \
  )

#define THEORETICAL_FREQUENCY 444
#define PLL_MUL_MSB           0x0124
#define PLL_RATIO_INDEX       5

// Note: Only tested on Slim, should be called from SC side
// 444 ok - approaching the stability limit
// base * (num / den) * ratio, with base = 37 and ratio = 1
static inline void setOverclock() {
  
  // note: needs to be 333 to be able to reach 444mhz
  const int INITIAL_FREQUENCY = 333;
  
  scePowerSetClockFrequency(INITIAL_FREQUENCY, INITIAL_FREQUENCY, INITIAL_FREQUENCY/2);

  const u32 den = 19;
  const float base = 37;
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * den)) / base);
  u32 _num = (u32)(((float)(INITIAL_FREQUENCY * den)) / base);
  
  int intr;
  suspendCpuIntr(intr);

  // throttle all clock domains before overclocking PLL
  hw(0xbc200000) = 32 << 16 | 511;
  hw(0xBC200004) = 32 << 16 | 511;
  hw(0xBC200008) = 32 << 16 | 511;
  sync();
  
  // set bit bit 7 to apply index, wait until hardware clears it
  hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != PLL_RATIO_INDEX);

  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  while (_num <= num) {
    const u32 lsb = _num << 8 | den;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline();
    _num++;
  }

  // unthrottle all clock domains after overclocking PLL
  hw(0xbc200000) = 511 << 16 | 511;
  hw(0xBC200004) = 511 << 16 | 511;
  hw(0xBC200008) = 511 << 16 | 511;
  sync();

  // wait for clock stability, signal propagation and pipeline drain
  u32 i = 0xfffff;
  while (--i) {
    delayPipeline();
  }
  
  resumeCpuIntr(intr);
}

static inline void cancelOverclock() {
  
  const int TARGET_FREQUENCY = 333;
  
  const u32 den = 19;
  const float base = 37;

  const u32 num = (u32)(((float)(TARGET_FREQUENCY * den)) / base);
  u32 _num = (u32)(((float)(THEORETICAL_FREQUENCY * den)) / base);
  
  int intr;
  suspendCpuIntr(intr);
  
  const u32 index = 2;
  hw(0xbc200000) = 32 << 16 | 511;
  hw(0xBC200004) = 32 << 16 | 511;
  hw(0xBC200008) = 32 << 16 | 511;
  sync();
  
  hw(0xbc100068) = 0x80 | index;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != index);
  
  while (_num > num) {
    const u32 lsb = _num << 8 | den;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline();
    _num--;
  }

  hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != PLL_RATIO_INDEX);

  hw(0xbc200000) = 511 << 16 | 511;
  hw(0xBC200004) = 511 << 16 | 511;
  hw(0xBC200008) = 511 << 16 | 511;
  sync();

  u32 i = 0xfffff;
  while (--i) {
    delayPipeline();
  }
  
  resumeCpuIntr(intr);
  
  scePowerSetClockFrequency(TARGET_FREQUENCY, TARGET_FREQUENCY, TARGET_FREQUENCY/2);
}


#endif
