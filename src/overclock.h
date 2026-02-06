#ifndef H_OVERCLOCK_PLUGIN_MAIN
#define H_OVERCLOCK_PLUGIN_MAIN

#include "main.h"

// Important:
// - Phat not supported at 444Mhz
// - tested on 2k and 3k
// - 444 MHz ok, approaching the stability limit

// m-c/d 2026, for more information on this project see:
// https://github.com/mcidclan/psp-undocumented-sorcery/tree/main/experimental-overclock

#define DEFAULT_FREQUENCY         333
#define THEORETICAL_FREQUENCY     444
#define PLL_MUL_MSB               0x0124
#define PLL_RATIO_INDEX           5
#define PLL_BASE_FREQ             37
#define PLL_DEN                   20
#define PLL_CUSTOM_FLAG           27 /*16*/

#define updatePLLMultiplier(var)                    \
{                                                   \
  const u32 lsb = (var) << 8 | PLL_DEN;             \
  const u32 multiplier = (PLL_MUL_MSB << 16) | lsb; \
  hw(0xbc1000fc) = multiplier;                      \
  sync();                                           \
}

#define updatePLLControl()                          \
{                                                   \
  if ((hw(0xbc100068) & 0xff) != PLL_RATIO_INDEX) { \
    hw(0xbc100068) = 0x80 | PLL_RATIO_INDEX;        \
    sync();                                         \
    do {                                            \
      delayPipeline();                              \
    } while (hw(0xbc100068) != PLL_RATIO_INDEX);    \
  }                                                 \
}

static inline void setOverclock() {
  
  scePowerSetClockFrequency(DEFAULT_FREQUENCY, DEFAULT_FREQUENCY, DEFAULT_FREQUENCY/2);
  resetDomainRatios();
  
  u32 _num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  const u32 num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);

  vfpuSync();
  sync();

  int intr, state;
  state = sceKernelSuspendDispatchThread();
  suspendCpuIntr(intr);
    
  updatePLLControl();
  
  while (_num <= num) {
    updatePLLMultiplier(_num);
    _num++;
  }
  settle();

  hw(0xbc1000fc) |= (1 << PLL_CUSTOM_FLAG);
  sync();
  
  resumeCpuIntr(intr);
  sceKernelResumeDispatchThread(state);
}

static inline void cancelOverclock() {
    
  u32 _num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  const u32 num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  
  vfpuSync();
  sync();

  int intr, state;
  state = sceKernelSuspendDispatchThread();  
  suspendCpuIntr(intr);
  
  const u32 pllMul = hw(0xbc1000fc); sync();
  const int overclocked = pllMul & (1 << PLL_CUSTOM_FLAG);
  
  if (overclocked) {
    
    updatePLLControl();

    while (_num >= num) {
      updatePLLMultiplier(_num);
      _num--;
    }
    settle();

    hw(0xbc1000fc) &= ~((1 << PLL_CUSTOM_FLAG));
    sync();
  }
  
  resumeCpuIntr(intr);
  sceKernelResumeDispatchThread(state);
}

static inline void initOverclock() {
  sceKernelIcacheInvalidateAll();
  unlockMemory();  
  cancelOverclock();
}

#endif
