#ifndef H_OVERCLOCK_PLUGIN_MAIN
#define H_OVERCLOCK_PLUGIN_MAIN

#include "main.h"

// Important:
// Only tested on 2k and 3k
// should be called from SC side
// 444 MHz ok, approaching the stability limit
// THEORETICAL_FREQUENCY = base * (num / den) * ratio

#define DEFAULT_FREQUENCY     333
#define THEORETICAL_FREQUENCY 444
#define PLL_MUL_MSB           0x0124
#define PLL_RATIO_INDEX       5
#define PLL_BASE_FREQ         37
#define PLL_DEN               19

static inline void setOverclock() {
  
  // note: needs to be 333 to be able to reach 444mhz
  const int INITIAL_FREQUENCY = DEFAULT_FREQUENCY;  
  scePowerSetClockFrequency(INITIAL_FREQUENCY, INITIAL_FREQUENCY, INITIAL_FREQUENCY/2);
  
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  u32 _num = (u32)(((float)(INITIAL_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  
  int intr;
  suspendCpuIntr(intr);
  resetDomains();
  
  // set bit bit 7 to apply index, wait until hardware clears it
  hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
  do {
    delayPipeline();
  } while (hw(0xbc100068) != PLL_RATIO_INDEX);

  // loop until the numerator reaches the target value,
  // and so, progressively increasing clock frequencies
  while (_num <= num) {
    const u32 lsb = _num << 8 | PLL_DEN;
    const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
    hw(0xbc1000fc) = multiplier;
    delayPipeline();
    _num++;
  }

  settle();
  resetDomains();
  resumeCpuIntr(intr);
}

static inline void cancelOverclock() {
  
  const int TARGET_FREQUENCY = DEFAULT_FREQUENCY;
  const u32 num = (u32)(((float)(TARGET_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  
  int intr;
  suspendCpuIntr(intr);

  const u32 pllIdx = hw(0xbc100068);
  const u32 pllMul = hw(0xbc1000fc);
  sync();
  
  const u32 msb = pllMul & 0xffff;
  const u32 _den = msb & 0xff;
  u32 _num = msb >> 8;
  
  const int overclocked = (int)(_den && ((_num / _den) > 9));
  
  if (overclocked && pllIdx == PLL_RATIO_INDEX) {
    
    resetDomains();
    
    while (_num > num) {
      _num--;
      const u32 lsb = _num << 8 | PLL_DEN;
      const u32 multiplier = (PLL_MUL_MSB << 16) | lsb;
      hw(0xbc1000fc) = multiplier;
      delayPipeline();
    }

    hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;
    do {
      delayPipeline();
    } while (hw(0xbc100068) != PLL_RATIO_INDEX);

    settle();
    resetDomains();
  }
  
  resumeCpuIntr(intr);

  //if (overclocked) {
  //  scePowerSetClockFrequency(TARGET_FREQUENCY, TARGET_FREQUENCY, TARGET_FREQUENCY/2);
  //}
}

#endif
