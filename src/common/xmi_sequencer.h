#pragma once

#include "formats/format_xmi.h"
#include <stddef.h>
#include <stdint.h>
#include <sys/_types/_ssize_t.h>

typedef struct {
  uint8_t done;
  double ticksPerSec;

  const XMISequence *currentSeq;
  uint8_t *head;

  ssize_t delay; // rename to timecode/tc
  

} XMISequencer;

void XMISequencerInit(XMISequencer *seq);
void XMISequencerStart(XMISequencer *seq, const XMISequence *sequence);
void XMISequencerUpdate(XMISequencer *seq);
