#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))


#define updateSeg(addr)         \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
    "la $t0, %0             \n" \
    "li $t1, 0x80000000     \n" \
    "or $t0, $t0, $t1       \n" \
                                \
    "cache 0x8, 0($t0)      \n" \
    "sync                   \n" \
    ".set pop               \n" \
    :                           \
    : "i" (addr)                \
    : "$t0", "$t1", "memory"    \
  )
    
#define sync()          \
  asm volatile(         \
    "sync       \n"     \
  )

#define delayPipeline()                    \
  asm volatile(                            \
    "nop; nop; nop; nop; nop; nop; nop \n" \
  )

#define suspendCpuIntr(var)    \
  asm volatile(                \
    ".set push             \n" \
    ".set noreorder        \n" \
    ".set volatile         \n" \
    ".set noat             \n" \
    "mfc0  %0, $12         \n" \
    "sync                  \n" \
    "li    $t0, 0xfffffffe \n" \
    "and   $t0, %0, $t0    \n" \
    "mtc0  $t0, $12        \n" \
    "sync                  \n" \
    "nop                   \n" \
    "nop                   \n" \
    "nop                   \n" \
    ".set pop              \n" \
    : "=r"(var)                \
    :                          \
    : "$t0", "memory"          \
  )

#define resumeCpuIntr(var) \
  asm volatile(            \
    ".set push      \n"    \
    ".set noreorder \n"    \
    ".set volatile  \n"    \
    ".set noat      \n"    \
    "mtc0  %0, $12  \n"    \
    "sync           \n"    \
    "nop            \n"    \
    "nop            \n"    \
    "nop            \n"    \
    ".set pop       \n"    \
    :                      \
    : "r"(var)             \
    : "memory"             \
  )

#define settle()                \
  asm volatile(                 \
    ".set push              \n" \
    ".set noreorder         \n" \
    ".set nomacro           \n" \
    ".set volatile          \n" \
    ".set noat              \n" \
                                \
    "sync                   \n" \
    "lui  $t0, 0x05         \n" \
    "ori  $t0, $t0, 0x5555  \n" \
                                \
    "1:                     \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  nop                  \n" \
    "  addiu $t0, $t0, -1   \n" \
    "  bnez  $t0, 1b        \n" \
    "  nop                  \n" \
                                \
    ".set pop               \n" \
    :                           \
    :                           \
    : "$t0", "memory"           \
  )

// Set clock domains to ratio 1:1
#define resetDomainRatios()          \
  sync();                            \
  hw(0xbc200000) = 511 << 16 | 511;  \
  hw(0xBC200004) = 511 << 16 | 511;  \
  hw(0xBC200008) = 511 << 16 | 511;  \
  sync();
  
static inline void _unlockMemory() {
  const u32 start = 0xbc000000;
  const u32 end   = 0xbc00002c;
  for (u32 reg = start; reg <= end; reg += 4) {
    hw(reg) = -1;
  }
  sync();
}

#define setOverclock()                                                         \
  updateSeg(_setOverclock);                                                    \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_setOverclock));

#define cancelOverclock()                                                      \
  updateSeg(_cancelOverclock);                                                 \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_cancelOverclock));

#define unlockMemory()                                                         \
  updateSeg(_unlockMemory);                                                    \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_unlockMemory));

#define dump()                                                                 \
  updateSeg(_dump);                                                            \
  kcall((int (*)(void))(0x80000000 | (unsigned int)_dump));

