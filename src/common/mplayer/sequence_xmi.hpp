#ifndef __SEQUENCE_XMI_H
#define __SEQUENCE_XMI_H

#include "sequence.hpp"

class XMITrack;

class SequenceXMI : public Sequence {
public:
  SequenceXMI();
  ~SequenceXMI();

  void setTimePerBeat(uint32_t usec);

  static bool isValid(const uint8_t *data, size_t size);

  uint32_t update(OPLPlayer &player) override;

  unsigned numSongs() const override { return m_tracks.size(); }
  void reset() override;

private:
  void setDefaults();
  void read(const uint8_t *data, size_t size) override;
  uint32_t readRootChunk(const uint8_t *data, size_t size);

  std::vector<XMITrack *> m_tracks;

  double m_ticksPerSec;
};

#endif // __SEQUENCE_XMI_H
