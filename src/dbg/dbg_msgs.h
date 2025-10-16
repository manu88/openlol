
#include <stdint.h>

#define DBG_PORT 9000

typedef enum {
  DBGMsgType_Hello = 0,
  DBGMsgType_Goodbye = 0,

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
} DBGMsgType;

typedef struct {
  uint8_t type;
  uint32_t dataSize;
} DBGMsgHeader;

typedef struct {
  uint16_t currentBock;
} DBGMsgStatus;

typedef struct {
  uint16_t itemId;
} DBGMSGGiveItemRequest;

typedef struct {
  uint16_t response;
} DBGMSGGiveItemResponse;

typedef struct {
  uint16_t state;
} DBGMSGSetStateRequest;

typedef struct {
  uint16_t response;
} DBGMSGSetStateResponse;

typedef struct {
  uint16_t response;
} DBGMSGQuitResponse;
