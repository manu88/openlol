#include "format_wll.h"
#include <stdint.h>
#include <stdio.h>

void TestWLL(const uint8_t *buffer, size_t size) {
  printf("WLL file has %zi bytes\n", size);

  const uint8_t *dataStart = buffer + 14;
  size -= 14;
  int count = 0;
  while (size) {
    const uint16_t *chunk = (const uint16_t *)dataStart;
    // WMI - wall mapping index
    for (int i = 0; i < 6; i++) {
      printf("0X%04X ", chunk[i]);
    }
    printf("\n");

    dataStart += 12;
    size -= 12;
    count++;
  }
  printf("Got %i entries\n", count);
}
