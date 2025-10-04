
#include <stdint.h>

#define DBG_PORT 9000

typedef enum {
  DBGMsgType_Hello = 0,
  DBGMsgType_Goodbye = 0,
  DBGMsgType_StatusRequest,
  DBGMsgType_StatusResponse,
} DBGMsgType;

typedef struct {
  uint8_t type;
  uint32_t dataSize;
} DBGMsgHeader;

typedef struct {
  uint16_t currentBock;
} DBGMsgStatus;
