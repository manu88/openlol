#pragma once

#include "formats/format_xmi.h"
#include <stdint.h>

typedef struct {
  uint8_t done;
  double ticksPerSec;
} XMISequencer;

void XMISequencerInit(XMISequencer *seq);
void XMISequencerPlay(XMISequencer *seq, const XMISequence *sequence);
