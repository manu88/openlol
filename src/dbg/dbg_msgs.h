
#include <stdint.h>

#define DBG_PORT 9000

typedef enum {
  DBGMsgType_Hello = 0,
  DBGMsgType_Goodbye,

  DBGMsgType_StatusRequest,
  DBGMsgType_StatusResponse,

  DBGMsgType_GiveItemRequest,
  DBGMsgType_GiveItemResponse,

  DBGMsgType_SetStateRequest,
  DBGMsgType_SetStateResponse,

  DBGMsgType_QuitRequest,
  DBGMsgType_QuitResponse,

  DBGMsgType_NoClipRequest,
  DBGMsgType_NoClipResponse,

  DBGMsgType_SetLoggerRequest,
  DBGMsgType_SetLoggerResponse,

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
