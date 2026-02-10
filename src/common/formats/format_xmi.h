#pragma once
#include <stddef.h>
#include <stdint.h>

/*
A few facts about format:
- XMI has no initial delay value
// from https://moddingwiki.shikadi.net/wiki/XMI_Format
- the "Note On" event contains 3 parameters - the note number, velocity level
(same as standard MIDI), and also duration in ticks. Duration is stored as
variable-length value in concatenated bits format. Since note events store
information about its duration, there are no "Note Off" events. ! Provide
example

The second difference is both store delays as variable-length values, but while
standard MIDI stores delays as a series of 7-bit values that are concatenated
together to produce the final number, XMI instead stores the values as a series
of 7-bit values that are summed to produce the final value.

To further complicate matters, if there is no delay at all, then the delay byte
is completely omitted. This means when reading in a byte, the high-bit will need
to be inspected to work out whether the byte is the first part of a delay value
(high-bit unset) or a MIDI event (high-bit set.) If the high bit is not set, the
values should be read and summed until a byte (possibly the first one) is not
127.

*/
typedef struct {
  uint8_t patch;
  uint8_t bank;
} XMIPatch;

typedef struct {
  uint8_t *data;
  size_t dataSize;
} XMIEvents;

typedef struct {
  XMIEvents events;
  XMIPatch *patches;
  size_t numPatches;
} XMISequence;

typedef struct {
  uint16_t seqCount;
  XMISequence *sequences;
} XMIHandle;

void XMIHandleRelease(XMIHandle *handle);
int XMIHandleFromBuffer(XMIHandle *handle, uint8_t *buffer, size_t bufferSize);
