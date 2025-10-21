#include "format_xxx.h"
#include <stdint.h>

int XXXHandleFromBuffer(XXXHandle *handle, uint8_t *buffer, size_t size) {
  handle->entries = (LegendEntry *)buffer;
  handle->numEntries = size / 12;
  return 1;
}
