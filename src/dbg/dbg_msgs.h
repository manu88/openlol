
#include <stdint.h>
typedef enum {
  DBGMsgType_Hello = 0,
} DBGMsgType;

typedef struct {
  uint8_t type;
  uint32_t dataSize;
} DBGMsgHeader;
