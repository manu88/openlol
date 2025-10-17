#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint16_t addrOffset;
  uint8_t disassemble;
  char *disasmBuffer;
  size_t disasmBufferIndex;
  size_t disasmBufferSize;
  uint8_t showDisamComment;
} EMCDisassembler;

#ifndef PRINTFLIKE
#define PRINTFLIKE(n, m) __attribute__((format(printf, n, m)))
#endif

void EMCDisassemblerInit(EMCDisassembler *disassembler);
void EMCDisassemblerRelease(EMCDisassembler *disassembler);
void EMCDisassemblerEmitLine(EMCDisassembler *disasm, uint32_t instOffset,
                             const char *fmt, ...) PRINTFLIKE(3, 4);
