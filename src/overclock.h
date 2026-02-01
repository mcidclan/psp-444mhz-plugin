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

#define DEFAULT_FREQUENCY     333
#define THEORETICAL_FREQUENCY 444
#define PLL_MUL_MSB           0x0124
#define PLL_RATIO_INDEX       5

// Note: Only tested on Slim, should be called from SC side
// 444 ok - approaching the stability limit
// base * (num / den) * ratio, with base = 37 and ratio = 1
static inline void setOverclock() {
  
  // note: needs to be 333 to be able to reach 444mhz
  const int INITIAL_FREQUENCY = DEFAULT_FREQUENCY;
  
  scePowerSetClockFrequency(INITIAL_FREQUENCY, INITIAL_FREQUENCY, INITIAL_FREQUENCY/2);

  const u32 den = 19;
  const float base = 37;
  
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * den)) / base);
  u32 _num = (u32)(((float)(INITIAL_FREQUENCY * den)) / base);
  
  int intr;
  suspendCpuIntr(intr);

  // set clock domains to ratio 1:1
  hw(0xbc200000) = 511 << 16 | 511;
  hw(0xBC200004) = 511 << 16 | 511;
  hw(0xBC200008) = 511 << 16 | 511;
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

  // wait for clock stability, signal propagation and pipeline drain
  u32 i = 0x1fffff;
  while (--i) {
    delayPipeline();
  }
  
  resumeCpuIntr(intr);
}

static inline void cancelOverclock() {
  
  const int TARGET_FREQUENCY = DEFAULT_FREQUENCY;
  
  const u32 den = 19;
  const float base = 37;

  const u32 num = (u32)(((float)(TARGET_FREQUENCY * den)) / base);
  
  int intr;
  suspendCpuIntr(intr);

  const u32 pllMul = hw(0xbc1000fc);
  const u32 msb = pllMul & 0xffff;
  const u32 _den = msb & 0xff;
  u32 _num = msb >> 8;
  
  const int overclocked = (int)(_den && ((_num / _den) > 10));

  if (overclocked) {
    
    hw(0xbc200000) = 511 << 16 | 511;
    hw(0xBC200004) = 511 << 16 | 511;
    hw(0xBC200008) = 511 << 16 | 511;
    sync();
    
    while (_num > num) {
      _num--;
      const u32 lsb = _num << 8 | den;
      const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
      hw(0xbc1000fc) = multiplier;
      delayPipeline();
    }

    hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
    do {
      delayPipeline();
    } while (hw(0xbc100068) != PLL_RATIO_INDEX);

    u32 i = 0x1fffff;
    while (--i) {
      delayPipeline();
    }
  }
  
  resumeCpuIntr(intr);

  if (overclocked) {
    scePowerSetClockFrequency(TARGET_FREQUENCY, TARGET_FREQUENCY, TARGET_FREQUENCY/2);
  }
}


#endif
