#ifndef __CPU_H__
#define __CPU_H__
#include <stdint.h>

#define ORG 0x200

struct cpu_t {
  uint8_t v[16];
  uint16_t i, pc;
  uint8_t sp, dt, st;
  uint16_t stack[16];
  uint8_t memory[1<<12];
};

#endif
