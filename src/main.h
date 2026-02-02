#define hw(addr)                      \
  (*((volatile unsigned int*)(addr)))

#define sync()          \
  asm volatile(         \
    "sync       \n"     \
  )

#define delayPipeline()                    \
  asm volatile(                            \
    ".set noreorder                    \n" \
    "sync                              \n" \
    "nop; nop; nop; nop; nop; nop; nop \n" \
    ".set reorder                      \n" \
  )

#define suspendCpuIntr(var) \
  asm volatile(             \
    ".set noreorder \n"     \
    "mfic  %0, $0   \n"     \
    "mtic  $0, $0   \n"     \
    "sync           \n"     \
    "nop            \n"     \
    "nop            \n"     \
    "nop            \n"     \
    ".set reorder   \n"     \
    : "=r"(var)             \
    :                       \
    : "$8", "memory"        \
  )

#define resumeCpuIntr(var)  \
  asm volatile(             \
    ".set noreorder \n"     \
    "mtic  %0, $0   \n"     \
    "sync           \n"     \
    "nop            \n"     \
    "nop            \n"     \
    "nop            \n"     \
    ".set reorder   \n"     \
    :                       \
    : "r"(var)              \
    : "memory"              \
  )

// Set clock domains to ratio 1:1
#define resetDomains()               \
  hw(0xbc200000) = 511 << 16 | 511;  \
  hw(0xBC200004) = 511 << 16 | 511;  \
  hw(0xBC200008) = 511 << 16 | 511;  \
  delayPipeline();

// Wait for clock stability, signal propagation and pipeline drain
#define settle()            \
  u32 i = 0x1fffff;         \
  while (--i) {             \
    delayPipeline();        \
  }                         

// Clear Exception Level bit in CP0 Status Register
#define cleanEXL()                \
  asm volatile(                   \
    ".set noreorder          \n"  \
    "mfc0 $8, $12            \n"  \
    "ins  $8, $zero, 1, 1    \n"  \
    "mtc0 $8, $12            \n"  \
    "sync                    \n"  \
    "nop                     \n"  \
    "nop                     \n"  \
    "nop                     \n"  \
    ".set reorder            \n"  \
    ::: "$8", "memory"            \
  )
