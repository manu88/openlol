#pragma once

#include "formats/format_xmi.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t done;
  double ticksPerSec;

  const XMISequence *currentSeq;
  uint8_t *head;

} XMISequencer;

void XMISequencerInit(XMISequencer *seq);
void XMISequencerReset(XMISequencer *seq);
void XMISequencerPlay(XMISequencer *seq, const XMISequence *sequence);
