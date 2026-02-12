#ifndef H_OVERCLOCK_PLUGIN_MAIN
#define H_OVERCLOCK_PLUGIN_MAIN

#include "main.h"
//#include <pspge.h>

// Important:
// - Phat not supported at 444Mhz
// - tested on 2k and 3k
// - 407 - 444 MHz ok, approaching the stability limit

// m-c/d 2026, for more information on this project see:
// https://github.com/mcidclan/psp-undocumented-sorcery/tree/main/experimental-overclock

#define    DEFAULT_FREQUENCY        333
static int THEORETICAL_FREQUENCY  = 444;

#define PLL_MUL_MSB               0x0124
#define PLL_RATIO_INDEX           5
#define PLL_BASE_FREQ             37
#define PLL_DEN                   20
#define PLL_CUSTOM_FLAG           (27 - 16)

#define updatePLLMultiplier(num, msb)               \
{                                                   \
  const u32 lsb = (num) << 8 | PLL_DEN;             \
  const u32 multiplier = (msb << 16) | lsb;         \
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
  
  int defaultFreq = DEFAULT_FREQUENCY;
  const int freqStep = PLL_BASE_FREQ / 2;
  int theoreticalFreq = defaultFreq + freqStep;
  
  while (theoreticalFreq <= THEORETICAL_FREQUENCY) {
    
    int intr, state;
    state = sceKernelSuspendDispatchThread();
    suspendCpuIntr(intr);
    
    // clearTags();
    
    u32 _num = (u32)(((float)(defaultFreq * PLL_DEN)) / PLL_BASE_FREQ);
    const u32 num = (u32)(((float)(theoreticalFreq * PLL_DEN)) / PLL_BASE_FREQ);
    
    updatePLLControl();
    
    const u32 msb = PLL_MUL_MSB | (1 << PLL_CUSTOM_FLAG);
    while (_num <= num) {
      updatePLLMultiplier(_num, msb);
      _num++;
    }
    settle();
    
    defaultFreq += freqStep;
    theoreticalFreq = defaultFreq + freqStep;
    
    resumeCpuIntr(intr);
    sceKernelResumeDispatchThread(state);
  
    scePowerTick(PSP_POWER_TICK_ALL);
    sceKernelDelayThread(100);
  }
}

static inline int cancelOverclock() {
    
  u32 _num = (u32)(((float)(THEORETICAL_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  const u32 num = (u32)(((float)(DEFAULT_FREQUENCY * PLL_DEN)) / PLL_BASE_FREQ);
  
  int intr, state;
  state = sceKernelSuspendDispatchThread();
  suspendCpuIntr(intr);
  
  /*
  const u32 pllCtl = hw(0xbc100068);
  const u32 pllMul = hw(0xbc1000fc);
  sync();
  
  const float n = (float)((pllMul & 0xff00) >> 8);
  const float d = (float)((pllMul & 0x00ff));
  const float m = (d > 0.0f) ? (n / d) : 9.0f;
  const int overclocked = ((pllCtl & 5) && (m > 9.0f)) ? 1 : 0;
  */
  
  const u32 pllMul = hw(0xbc1000fc); sync();
  const int overclocked = pllMul & (1 << PLL_CUSTOM_FLAG);
  
  if (overclocked) {
    
    updatePLLControl();

    while (_num >= num) {
      updatePLLMultiplier(_num, PLL_MUL_MSB);
      _num--;
    }
    settle();
  }
  
  resumeCpuIntr(intr);
  sceKernelResumeDispatchThread(state);
  
  return overclocked;
}

static inline int readFreqConfig() {
  char buf[16] = {0};
  SceUID f = sceIoOpen("ms0:/overconfig.txt", PSP_O_RDONLY, 0777);
  if(f >= 0) {
    sceIoRead(f, buf, sizeof(buf) - 1);
    sceIoClose(f);
  } else {
    return -1;
  }
  u32 result = 0;
  for(int i = 0; buf[i] >= '0' && buf[i] <= '9'; i++) {
    result = result * 10 + (buf[i] - '0');
  }
  return result;
}

static inline void initOverclock() {
  sceKernelIcacheInvalidateAll();
  unlockMemory();
  
  const int freq = readFreqConfig();
  if (freq > 333 && freq < 466) {
    THEORETICAL_FREQUENCY = freq;
  }
  
  scePowerSetClockFrequency(DEFAULT_FREQUENCY, DEFAULT_FREQUENCY, DEFAULT_FREQUENCY/2);
  cancelOverclock();
}

#endif
