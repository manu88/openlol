#include "format_inf.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Adapted from
// https://github.com/scummvm/scummvm-tools/blob/cd2a721f48460b7c8f69024dbe5f26971491599c/dekyra.cpp

uint16_t swap_uint16(uint16_t val) { return (val << 8) | (val >> 8); }

uint32_t swap_uint32(uint32_t val) {
  val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
  return (val << 16) | (val >> 16);
}

#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

typedef enum {
  kForm = 0,
  kEmc2Ordr = 1,
  kText = 2,
  kData = 3,
  kCountChunkTypes
} ScriptChunkTypes;

typedef struct {
  uint32_t _size;
  uint8_t *_data; // by TEXT used for count of texts, by EMC2ODRD it is used
                  // for a count of somewhat
  uint8_t *_additional; // currently only used for TEXT
} ScriptChunk;

ScriptChunk _chunks[kCountChunkTypes];
uint32_t _scriptSize;
uint32_t _currentPos; // current instruction pos

static void decodeTextArea(void) {
  printf("TEXT chunk:\n");
  // first is size
  for (uint32_t pos = 1; pos < (_chunks[kText]._size << 1); ++pos) {
    if (swap_uint16(((uint16_t *)_chunks[kText]._data)[pos]) >= _scriptSize) {
      break;
    }
    uint32_t startoffset = swap_uint16(((uint16_t *)_chunks[kText]._data)[pos]);
    printf("Index: %d Offset: %d:\n", pos, startoffset);
    /*uint32_t endoffset = TO_BE_16(((uint16_t*)_chunks[kText]._data)[pos+1]);
    printf("\nstartoffset = %d, endoffset = %d\n\n", startoffset, endoffset);
    for (; startoffset < endoffset; ++startoffset) {
            printf("%c", *(char*)(_chunks[kText]._additional + startoffset));
    }
    printf("\n");*/
    printf("%d(%d) : %s\n", pos, startoffset,
           (char *)(_chunks[kText]._data + startoffset));
  }
}

typedef struct {
  uint16_t command; // normally uint8_t
  const char *description;
  int usesArgument;
} CommandDesc;

typedef struct {
  int32_t script; // normally uint8_t
  const char *description;
} ScriptDesc;

typedef struct {
  uint16_t opcode; // normally uint8_t
  const char *description;
  const char *parameters;
} OpcodeDesc;

// function wich calls opcode procs
const uint16_t OPCODE_CALLER = 0x0E;

static uint32_t getNextScriptPos(uint32_t current_start) {
  uint32_t pos = 0xFFFFFFFE;

  for (uint32_t tmp = 0; tmp < _chunks[kEmc2Ordr]._size; ++tmp) {
    if ((uint32_t)((swap_uint16(_chunks[kEmc2Ordr]._data[tmp]) << 1) + 2) >
            current_start &&
        (uint32_t)((swap_uint16(_chunks[kEmc2Ordr]._data[tmp]) << 1) + 2) <
            pos) {
      pos = ((swap_uint16(_chunks[kEmc2Ordr]._data[tmp]) << 1) + 2);
    }
  }

  if (pos > _scriptSize) {
    pos = _scriptSize;
  }

  return pos;
}

int decodeSpecialScript(int32_t script) {
  static CommandDesc commandDesc[] = {{0x00, "c1_goToLine", 1},
                                      {0x01, "c1_setReturn", 1},
                                      {0x02, "c1_pushRetRec", 1},
                                      {0x03, "c1_push", 1},
                                      {0x04, "c1_push", 1},
                                      {0x05, "c1_pushVar", 1},
                                      {0x06, "c1_pushFrameNeg", 1},
                                      {0x07, "c1_pushFramePos", 1},
                                      {0x08, "c1_popRetRec", 1},
                                      {0x09, "c1_popVar", 1},
                                      {0x0A, "c1_popFrameNeg", 1},
                                      {0x0B, "c1_popFramePos", 1},
                                      {0x0C, "c1_addToSP", 1},
                                      {0x0D, "c1_subFromSP", 1},
                                      {0x0E, "c1_execOpcode", 1},
                                      {0x0F, "c1_ifNotGoTo", 1},
                                      {0x10, "c1_negate", 1},
                                      {0x11, "c1_evaluate", 1},
                                      // normaly only untils 0xFF
                                      {0xFFFF, 0, 0}};

  static ScriptDesc scriptDesc[] = {
      {0, "kSetupScene"},        {1, "kClickEvent"}, {2, "kActorEvent"},
      {4, "kEnterEvent"},        {5, "kExitEvent"},  {7, "kLoadResources"},
      {0xFFFF, "unknown script"}};

  static OpcodeDesc opcodeDesc[] = {{0x68, "0x68", "int"}};

  if ((uint32_t)script >= _chunks[kEmc2Ordr]._size || script < 0) {
    return 0;
  }

  int gotScriptName = 0;

  printf("Script %i: ", script);

  for (uint32_t pos = 0; pos < ARRAYSIZE(scriptDesc) - 1; ++pos) {
    if (scriptDesc[pos].script == script) {
      printf("%s:\n", scriptDesc[pos].description);
      gotScriptName = 1;
      break;
    }
  }

  uint8_t _currentCommand = 0;
  uint8_t _argument = 0;
  _currentPos =
      (swap_uint16(((uint16_t *)_chunks[kEmc2Ordr]._data)[script]) << 1) + 2;
  uint8_t *script_start = _chunks[kData]._data;

  int gotArgument = 1;

  uint32_t nextScriptStartsAt = getNextScriptPos(_currentPos);

  if (nextScriptStartsAt < _currentPos) {
    return 1;
  }
  if (!gotScriptName) {
    printf("%s:\n", scriptDesc[ARRAYSIZE(scriptDesc) - 1].description);
  }

  while (1) {
    if ((uint32_t)_currentPos > _chunks[kData]._size - 1 ||
        (uint32_t)_currentPos >= nextScriptStartsAt) {
      break;
    }

    gotArgument = 1;
    _currentCommand = *(script_start + _currentPos++);

    // gets out
    if (_currentCommand & 0x80) {
      _argument =
          ((_currentCommand & 0x0F) << 8) | *(script_start + _currentPos++);
      _currentCommand &= 0xF0;
    } else if (_currentCommand & 0x40) {
      _argument = *(script_start + _currentPos++);
    } else if (_currentCommand & 0x20) {
      _currentPos++;

      uint16_t tmp = *(uint16_t *)(script_start + _currentPos);
      tmp &= 0xFF7F;

      _argument = swap_uint16(tmp);
      _currentPos += 2;
    } else {
      gotArgument = 0;
    }

    _currentCommand &= 0x1f;

    // prints the offset
    printf("0x%x:\t", _currentPos);

    int gotCommand = 0;

    // lets get out what the command means
    for (uint32_t pos = 0; pos < ARRAYSIZE(commandDesc) - 1; ++pos) {
      if (commandDesc[pos].command == _currentCommand) {
        gotCommand = 1;
        printf("%s", commandDesc[pos].description);

        if (commandDesc[pos].usesArgument &&
            commandDesc[pos].command != OPCODE_CALLER) {
          printf("(0x%x)", _argument);
        } else if (commandDesc[pos].usesArgument &&
                   commandDesc[pos].command == OPCODE_CALLER) {
          int gotOpcode = 0;
          // lets look for our opcodes
          for (uint32_t pos2 = 0; pos2 < ARRAYSIZE(opcodeDesc); ++pos2) {
            if (opcodeDesc[pos2].opcode == _argument) {
              printf("(%s(%s))", opcodeDesc[pos2].description,
                     opcodeDesc[pos2].parameters);
              gotOpcode = 1;
              break;
            }
          }

          if (!gotOpcode) {
            printf("(0x%x(unknown))", _argument);
          }
        }

        break;
      }
    }

    // prints our command number + arg
    if (!gotCommand) {
      printf("0x%x(0x%x)", _currentCommand, _argument);
    }

    if (!gotArgument) {
      printf("\t; couldn't get argument! maybe command is wrong.");
    }

    printf("\n");
  }

  printf("\n-------------\n");

  return 1;
}

static void decodeScript(void) {
  int c = 0;
  for (uint32_t script = 0; decodeSpecialScript(script); ++script) {
    c++;
  }
  printf("Total %i scripts\n", c);
}

void TestINF(const uint8_t *buffer, size_t bufferSize) {
  printf("TestINF\n");
  _scriptSize = bufferSize;
  printf("_scriptSize = %u\n", _scriptSize);
  uint8_t chunkName[sizeof("EMC2ORDR") + 1];

  size_t readSize = 0;
  uint8_t *buff = (uint8_t *)buffer;

  while (readSize < bufferSize) {
    memcpy(chunkName, buff, 4);
    chunkName[4] = '\0';
    buff += 4;
    readSize += 4;
    if (strcmp((char *)chunkName, "FORM") == 0) {
      // looks like 'FORM' is the beginning of the file.
      // size if the whole file size
      _chunks[kForm]._size = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      printf("Got a FORM chunk size %u\n", _chunks[kForm]._size);
    } else if (strcmp((char *)chunkName, "TEXT") == 0) {
      uint32_t textSize = swap_uint32(*(uint32_t *)buff);
      textSize += textSize % 2 != 0 ? 1 : 0;
      buff += 4;
      readSize += 4;
      printf("Got a TEXT chunk size %u\n", textSize);
      _chunks[kText]._data = buff;
      _chunks[kText]._size = swap_uint32(*(uint32_t *)buff) >> 1;
      _chunks[kText]._additional =
          _chunks[kText]._data + (_chunks[kText]._size << 1);
      buff += textSize;
      readSize += textSize;
    } else if (strcmp((char *)chunkName, "DATA") == 0) {
      uint32_t dataSize = swap_uint32(*(uint32_t *)buff);
      buff += 4;
      readSize += 4;
      _chunks[kData]._size = dataSize;
      _chunks[kData]._data = buff;
      printf("Got a DATA chunk size %u\n", dataSize);
      // mostly it will be the end of the file because all files should end
      // with a 'DATA' chunk
      buff += dataSize;
      readSize += dataSize;
    } else {
      memcpy(&chunkName[4], buff, 4);
      chunkName[8] = '\0';
      buff += 4;
      readSize += 4;
      if (strcmp((char *)chunkName, "EMC2ORDR") == 0) {
        uint32_t chunkSize = swap_uint32(*(uint32_t *)buff);
        _chunks[kEmc2Ordr]._size = chunkSize >> 1;
        buff += 4;
        readSize += 4;
        _chunks[kEmc2Ordr]._data = buff;
        printf("Got a EMC2ORDR chunk size %u\n", chunkSize);
        buff += chunkSize;
        readSize += chunkSize;
      } else {
        printf("unknown chunk '%s'\n", chunkName);
      }
    }
  }

  decodeTextArea();
  decodeScript();
}
