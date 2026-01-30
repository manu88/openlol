#include "format_tlc.h"
#include <assert.h>

int TLCHandleFromBuffer(TLCHandle *handle, uint8_t *buffer, size_t bufferSize) {
  assert(bufferSize == TABLE_1_SIZE + TABLE_2_SIZE);
  handle->table1 = buffer;
  handle->table2 = buffer + TABLE_1_SIZE;
  return 1;
}
