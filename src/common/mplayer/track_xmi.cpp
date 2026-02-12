#include "track_xmi.hpp"
#include "player.hpp"
#include "sequence_xmi.hpp"
#include <stdio.h>

// ----------------------------------------------------------------------------

#define READ_U16BE(data, pos) ((data[pos] << 8) | data[pos + 1])
#define READ_U24BE(data, pos)                                                  \
  ((data[pos] << 16) | (data[pos + 1] << 8) | data[pos + 2])
#define READ_U32BE(data, pos)                                                  \
  ((data[pos] << 24) | (data[pos + 1] << 16) | (data[pos + 2] << 8) |          \
   data[pos + 3])

// ----------------------------------------------------------------------------
XMITrack::XMITrack(const uint8_t *data, size_t size, SequenceXMI *sequence) {
  m_data = new uint8_t[size];
  m_size = size;
  memcpy(m_data, data, size);
  m_sequence = sequence;

  reset();
}

XMITrack::~XMITrack() { delete[] m_data; }

uint32_t XMITrack::readDelay() {
  uint32_t delay = 0;
  uint8_t data = 0;

  if (m_pos >= m_size || (m_data[m_pos] & 0x80))
    return 0;

  do {
    data = m_data[m_pos];
    if (!(data & 0x80)) {
      delay += data;
      m_pos++;
    }
  } while ((data == 0x7f) && (m_pos < m_size));

  return delay;
}

void XMITrack::reset() {
  m_pos = m_delay = 0;
  m_atEnd = false;
  m_status = 0x00;
}

void XMITrack::advance(uint32_t time) {
  if (m_atEnd)
    return;

  m_delay -= time;
  for (auto &note : m_notes) {
    note.delay -= time;
  }
}

uint32_t XMITrack::readVLQ() {
  uint32_t vlq = 0;
  uint8_t data = 0;

  do {
    data = m_data[m_pos++];
    vlq <<= 7;
    vlq |= (data & 0x7f);
  } while ((data & 0x80) && (m_pos < m_size));

  return vlq;
}

int32_t XMITrack::minDelay() {
  int32_t delay = m_delay;

  for (auto const &note : m_notes) {
    delay = std::min(delay, note.delay);
  }

  return delay;
}

uint32_t XMITrack::update(OPLPlayer &player) {

  for (int i = 0; i < m_notes.size();) {
    if (m_notes[i].delay <= 0) {
      player.midiNoteOff(m_notes[i].channel, m_notes[i].note);
      m_notes[i] = m_notes.back();
      m_notes.pop_back();
    } else {
      i++;
    }
  }

  while (m_delay <= 0) {
    uint8_t data[2];
    MIDNote note;

    // make sure we have enough data left for one full event
    if (m_size - m_pos < 3) {
      m_atEnd = true;
      return UINT_MAX;
    }

    if ((m_data[m_pos] & 0x80))
      m_status = m_data[m_pos++];

    switch (m_status >> 4) {
    case 9: // note on
      data[0] = m_data[m_pos++];
      data[1] = m_data[m_pos++];
      player.midiEvent(m_status, data[0], data[1]);

      note.channel = m_status & 15;
      note.note = data[0];
      note.delay = readVLQ();
      m_notes.push_back(note);

      break;

    case 8:  // note off
    case 10: // polyphonic pressure
    case 11: // controller change
    case 14: // pitch bend
      data[0] = m_data[m_pos++];
      data[1] = m_data[m_pos++];
      player.midiEvent(m_status, data[0], data[1]);
      break;

    case 12: // program change
    case 13: // channel pressure (ignored)
      data[0] = m_data[m_pos++];
      player.midiEvent(m_status, data[0]);
      break;

    case 15: // sysex / meta event
      if (!metaEvent(player)) {
        m_atEnd = true;
        return UINT_MAX;
      }
      break;
    }

    m_delay += readDelay();
  }

  return minDelay();
}

bool XMITrack::metaEvent(OPLPlayer &player) {
  uint32_t len;

  if (m_status != 0xFF) {
    len = readVLQ();
    if (m_pos + len < m_size) {
      if (m_status == 0xf0)
        player.midiSysEx(m_data + m_pos, len);
    } else {
      return false;
    }
  } else {
    uint8_t data = m_data[m_pos++];
    len = readVLQ();

    // end-of-track marker (or data just ran out)
    if (data == 0x2F || (m_pos + len >= m_size)) {
      return false;
    }
    // tempo change
    if (data == 0x51) {
      m_sequence->setTimePerBeat(READ_U24BE(m_data, m_pos));
    }
  }

  m_pos += len;
  return true;
}
