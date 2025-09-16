#pragma once

#include <stddef.h>
#include <stdint.h>

int EMC_AssembleFile(const char *srcFilepath, const char *outFilePath);
int EMC_DisassembleFile(const char *binFilepath, const char *outFilePath);
uint16_t *EMC_Assemble(const char *script, size_t *outBufferSize);

int EMC_Exec(const char *scriptFilepath, int blockAddr);

int EMC_Tests(void);
