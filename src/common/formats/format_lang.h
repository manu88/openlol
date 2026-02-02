#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define STR_TAKEN_INDEX 0X403E            // "%s taken"
#define STR_CANT_GO_THAT_WAY_INDEX 0X403F // "You can't go that way!"
#define STR_EXIT_INDEX 0X4033             // "EXIT"
#define STR_YES_INDEX 0X4007              // "Yes"
#define STR_NO_INDEX 0X4008               // "No"

#define STR_FIST_LEVEL_NAME_INDEX 0X4211 // this is level 1: Keep

typedef enum {
  Language_EN = 0,
  Language_FR,
  Language_GR,
} Language;

const char *LanguageGetExtension(Language lang);

typedef struct {
  uint16_t count;
  uint8_t *originalBuffer;
} LangHandle;

int LangHandleFromBuffer(LangHandle *handle, uint8_t *buffer,
                         size_t bufferSize);
void LangHandleShow(LangHandle *handle);
uint16_t LangHandleGetString(const LangHandle *handle, uint16_t index,
                             char *outBuffer, size_t outBufferSize);

// -1 means invalid id, otherwise cast to uint16_t
int LangGetString(uint16_t id, uint8_t *useLevelFile);
