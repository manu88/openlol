#include "track_xmi.hpp"
#include "player.hpp"
#include "sequence_xmi.hpp"
#include <cassert>
#include <cstdint>
#include <stdio.h>

typedef enum {
  XMIEventType_POLY_KEY_PRES = 0XA,
  XMIEventType_CTL = 0XB,
  XMIEventType_PITCH_BEND = 0XE,
  XMIEventType_INSTR_CHANGE = 0XC,
  XMIEventType_CHAN_PRES = 0XD,
  XMIEventType_NOTE_ON = 0X9,
  XMIEventType_SYS_EX = 0XF,
} XMIEventType;

#define READ_U16BE(data, pos) ((data[pos] << 8) | data[pos + 1])
#define READ_U24BE(data, pos)                                                  \
  ((data[pos] << 16) | (data[pos + 1] << 8) | data[pos + 2])
#define READ_U32BE(data, pos)                                                  \
  ((data[pos] << 24) | (data[pos + 1] << 16) | (data[pos + 2] << 8) |          \
   data[pos + 3])

XMITrack::XMITrack(const uint8_t *data, size_t size, SequenceXMI *sequence) {
  m_data = new uint8_t[size];
  m_size = size;
  memcpy(m_data, data, size);
  m_sequence = sequence;

  reset();
}

XMITrack::~XMITrack() { delete[] m_data; }

uint32_t XMITrack::readXMIDelay() {
  uint32_t vlq = 0;
  uint8_t data = 0;

  do {
    data = m_data[m_pos++];
    vlq <<= 7;
    vlq |= (data & 0x7f);
  } while ((data & 0x80) && (m_pos < m_size));

  return vlq;
}

uint32_t XMITrack::readMidiDelay() {
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

int32_t XMITrack::minDelay() {
  int32_t delay = m_delay;

  for (auto const &note : m_notes) {
    delay = std::min(delay, note.delay);
  }

  return delay;
}

bool XMITrack::printMetaEvent() {
  uint32_t len;

  if (m_status != 0xFF) {
    len = readXMIDelay();
    if (m_pos + len >= m_size) {
      return false;
    }
    if (m_status == 0xf0) {
      printf("midiSysEx\n");
    }
  } else {
    uint8_t data = m_data[m_pos++];
    len = readXMIDelay();

    // end-of-track marker (or data just ran out)
    if (data == 0x2F || (m_pos + len >= m_size)) {
      return false;
    }
    // tempo change
    if (data == 0x51) {
      uint32_t usec = READ_U24BE(m_data, m_pos);
      printf("Tempo change usec=%X\n", usec);
      // m_sequence->setTimePerBeat(usec);
    } else {
      printf("Other Meta data %X\n", data);
    }
  }

  m_pos += len;
  return true;
}

void XMITrack::print() {
  while (true) {
    uint8_t data[2];
    MIDNote note;

    // make sure we have enough data left for one full event
    if (m_size - m_pos < 3) {
      m_atEnd = true;
      return;
    }

    if (m_data[m_pos] & 0x80) {
      m_status = m_data[m_pos++];

      switch ((XMIEventType)m_status >> 4) {
      case XMIEventType_NOTE_ON:
        data[0] = m_data[m_pos++];
        data[1] = m_data[m_pos++];

        // player.midiEvent(m_status, data[0], data[1]);

        note.channel = m_status & 15;
        note.note = data[0];
        note.delay = readXMIDelay();
        printf("NoteOn: note=0X%X channel=%X vel=0X%X dur=0X%X\n", note.note,
               note.channel, data[1], note.delay);

        // m_notes.push_back(note);

        break;
      case XMIEventType_POLY_KEY_PRES:
      case XMIEventType_CTL:
      case XMIEventType_PITCH_BEND:
        data[0] = m_data[m_pos++];
        data[1] = m_data[m_pos++];
        // player.midiEvent(m_status, data[0], data[1]);
        break;

      case XMIEventType_INSTR_CHANGE:
      case XMIEventType_CHAN_PRES: // (ignored)
        data[0] = m_data[m_pos++];
        // player.midiEvent(m_status, data[0]);
        break;

      case XMIEventType_SYS_EX:

        if (!printMetaEvent()) {
          m_atEnd = true;
          return;
        }
        break;
      default:
        assert(0);
      }

      uint32_t del = readMidiDelay();
      m_delay += del;
      if (del) {
        printf("Delay 0X%X\n", del);
      }
    }
  }
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

    if ((m_data[m_pos] & 0x80)) {
      m_status = m_data[m_pos++];

      switch ((XMIEventType)m_status >> 4) {
      case XMIEventType_NOTE_ON:
        data[0] = m_data[m_pos++];
        data[1] = m_data[m_pos++];
        player.midiEvent(m_status, data[0], data[1]);

        note.channel = m_status & 15;
        note.note = data[0];
        note.delay = readXMIDelay();
        m_notes.push_back(note);

        break;
      case XMIEventType_POLY_KEY_PRES:
      case XMIEventType_CTL:
      case XMIEventType_PITCH_BEND:
        data[0] = m_data[m_pos++];
        data[1] = m_data[m_pos++];
        player.midiEvent(m_status, data[0], data[1]);
        break;

      case XMIEventType_INSTR_CHANGE:
      case XMIEventType_CHAN_PRES: // (ignored)
        data[0] = m_data[m_pos++];
        player.midiEvent(m_status, data[0]);
        break;

      case XMIEventType_SYS_EX:
        if (!metaEvent(player)) {
          m_atEnd = true;
          return UINT_MAX;
        }
        break;
      default:
        assert(0);
      }

      m_delay += readMidiDelay();
    }
  }

  return minDelay();
}

bool XMITrack::metaEvent(OPLPlayer &player) {
  uint32_t len;

  if (m_status != 0xFF) {
    len = readXMIDelay();
    if (m_pos + len >= m_size) {
      return false;
    }
    if (m_status == 0xf0) {
      player.midiSysEx(m_data + m_pos, len);
    }
  } else {
    uint8_t data = m_data[m_pos++];
    len = readXMIDelay();

    // end-of-track marker (or data just ran out)
    if (data == 0x2F || (m_pos + len >= m_size)) {
      return false;
    }
    // tempo change
    if (data == 0x51) {
      m_sequence->setTimePerBeat(READ_U24BE(m_data, m_pos));
    } else {
      printf("Other Meta data %X\n", data);
    }
  }

  m_pos += len;
  return true;
}
