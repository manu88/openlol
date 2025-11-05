
#include <stdint.h>

#define DBG_PORT 9000

// Keep in sync with tools/dbg.py!
typedef enum {
  DBGMsgType_Hello = 0,
  DBGMsgType_Goodbye = 1,

  DBGMsgType_StatusRequest = 2,
  DBGMsgType_StatusResponse = 3,

  DBGMsgType_GiveItemRequest = 4,
  DBGMsgType_GiveItemResponse = 5,

  DBGMsgType_SetStateRequest = 6,
  DBGMsgType_SetStateResponse = 7,

  DBGMsgType_QuitRequest = 8,
  DBGMsgType_QuitResponse = 9,

  DBGMsgType_NoClipRequest = 10,
  DBGMsgType_NoClipResponse = 11,

  DBGMsgType_SetLoggerRequest = 12,
  DBGMsgType_SetLoggerResponse = 13,

  DBGMsgType_SetVarRequest = 14,
  DBGMsgType_SetVarResponse = 15,

} DBGMsgType;

typedef struct {
  uint8_t type;
  uint32_t dataSize;
} DBGMsg_Header;

typedef struct {
  uint16_t currentBock;
} DBGMsg_Status;

typedef struct {
  uint16_t itemId;
} DBGMSG_GiveItemRequest;

typedef struct {
  uint16_t response;
} DBGMSG_GiveItemResponse;

typedef struct {
  uint16_t state;
} DBGMSG_SetStateRequest;

typedef struct {
  uint16_t response;
} DBGMSG_SetStateResponse;

typedef struct {
  uint16_t response;
} DBGMSG_QuitResponse;

typedef struct {
  char prefix[32]; // '*' means enable disable log output
  uint8_t enable;  // enable/disable the prefix
} DBGMSG_EnableLoggerRequest;

typedef struct {
  uint16_t response;
} DBGMSG_EnableLoggerResponse;
