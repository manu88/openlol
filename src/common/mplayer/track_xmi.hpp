#ifndef __SEQUENCE_MID_H
#define __SEQUENCE_MID_H

#include <stdint.h>
#include <sys/types.h>
#include <vector>

class SequenceXMI;
class OPLPlayer;

class XMITrack {
public:
  XMITrack(const uint8_t *data, size_t size, SequenceXMI *sequence);
  virtual ~XMITrack();

  void reset();
  void advance(uint32_t time);
  uint32_t update(OPLPlayer &player);

  bool atEnd() const { return m_atEnd; }

protected:
  uint32_t readVLQ();
  uint32_t readDelay();
  int32_t minDelay();
  virtual bool metaEvent(OPLPlayer &player);

  SequenceXMI *m_sequence;
  uint8_t *m_data;
  uint32_t m_pos, m_size;
  int32_t m_delay;
  bool m_atEnd;
  uint8_t m_status; // for MIDI running status

  struct MIDNote {
    uint8_t channel, note;
    int32_t delay;
  };
  std::vector<MIDNote> m_notes;
};

#endif // __SEQUENCE_MUS_H
