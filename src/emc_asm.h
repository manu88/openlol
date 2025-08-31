#pragma once

#include <stdint.h>
#include <stddef.h>

int EMC_AssembleFile(const char *srcFilepath, const char *outFilePath);
uint16_t *EMC_Assemble(const char *script, size_t *outBufferSize);

int EMC_Exec(const char *scriptFilepath);

int EMC_Tests(void);
