#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))

#define sync()          \
  asm volatile(         \
    "sync       \n"     \
  )

#define delayPipeline()                    \
  asm volatile(                            \
    ".set noreorder                    \n" \
    "nop; nop; nop; nop; nop; nop; nop \n" \
    ".set reorder                      \n" \
  )

#define suspendCpuIntr(var)    \
  asm volatile(                \
    ".set push             \n" \
    ".set noreorder        \n" \
    ".set noat             \n" \
    "mfc0  %0, $12         \n" \
    "sync                  \n" \
    "li    $k0, 0xFFFFFFFE \n" \
    "and   $k0, %0, $k0    \n" \
    "mtc0  $k0, $12        \n" \
    "sync                  \n" \
    "nop                   \n" \
    "nop                   \n" \
    "nop                   \n" \
    ".set pop              \n" \
    : "=r"(var)                \
    :                          \
    : "$k0", "memory"          \
  )

#define resumeCpuIntr(var) \
  asm volatile(            \
    ".set push      \n"    \
    ".set noreorder \n"    \
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

// Set clock domains to ratio 1:1
#define resetDomains()               \
  hw(0xbc200000) = 511 << 16 | 511;  \
  /*hw(0xBC200004) = 511 << 16 | 511;*/  \
  /*hw(0xBC200008) = 511 << 16 | 511;*/  \
  sync();

// Wait for clock stability, signal propagation and pipeline drain
#define settle()            \
  sync();                   \
  u32 i = 0x1fffff;         \
  while (--i) {             \
    delayPipeline();        \
  }                         
