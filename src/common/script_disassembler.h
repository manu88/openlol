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

#define PRINTFLIKE(n, m) __attribute__((format(printf, n, m)))

void EMCDisassemblerInit(EMCDisassembler *disassembler);
void EMCDisassemblerRelease(EMCDisassembler *disassembler);
void EMCDisassemblerEmitLine(EMCDisassembler *disasm, const char *fmt, ...)
    PRINTFLIKE(2, 3);
