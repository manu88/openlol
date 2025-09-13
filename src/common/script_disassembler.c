#include "script_disassembler.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void EMCDisassemblerInit(EMCDisassembler *disassembler) {
  disassembler->disasmBufferSize = 512;
  disassembler->disasmBuffer = malloc(disassembler->disasmBufferSize);
  disassembler->disasmBufferIndex = 0;
}

void EMCDisassemblerRelease(EMCDisassembler *disassembler) {
  free(disassembler->disasmBuffer);
}

void EMCDisassemblerEmitLine(EMCDisassembler *disasm, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  size_t bSize = disasm->disasmBufferSize - disasm->disasmBufferIndex;
  if (bSize < 64) {
    disasm->disasmBufferSize *= 2;
    disasm->disasmBuffer =
        realloc(disasm->disasmBuffer, disasm->disasmBufferSize);
    bSize = disasm->disasmBufferSize - disasm->disasmBufferIndex;
  }
  size_t writtenSize = vsnprintf(
      disasm->disasmBuffer + disasm->disasmBufferIndex, bSize, fmt, args);

  if (writtenSize > bSize) {
    printf("no more size to write line, writtenSize=%zu, got bSize=%zu\n",
           writtenSize, bSize);
    assert(0);
  }
  disasm->disasmBufferIndex += writtenSize;
  va_end(args);

  if (disasm->showDisamComment) {
    bSize = disasm->disasmBufferSize - writtenSize;
    writtenSize = snprintf(disasm->disasmBuffer + disasm->disasmBufferIndex,
                           bSize, "; 0X%04X\n", 1);

    if (writtenSize > bSize) {
      printf("no more size to write line, writtenSize=%zu, got bSize=%zu\n",
             writtenSize, bSize);
      assert(0);
    }
    disasm->disasmBufferIndex += writtenSize;
  } else {
    writtenSize =
        snprintf(disasm->disasmBuffer + disasm->disasmBufferIndex, bSize, "\n");

    disasm->disasmBufferIndex += writtenSize;
  }
}
