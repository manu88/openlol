#include "xmi_sequencer.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define READ_U24BE(data, pos)                                                  \
  ((data[pos] << 16) | (data[pos + 1] << 8) | data[pos + 2])

const char *OPLPatch_names[256] = {
    "Acoustic Grand Piano",
    "Bright Acoustic Piano",
    "Electric Grand Piano",
    "Honky-tonk Piano",
    "Electric Piano 1",
    "Electric Piano 2",
    "Harpsichord",
    "Clavi",
    "Celesta",
    "Glockenspiel",
    "Music Box",
    "Vibraphone",
    "Marimba",
    "Xylophone",
    "Tubular Bells",
    "Dulcimer",
    "Drawbar Organ",
    "Percussive Organ",
    "Rock Organ",
    "Church Organ",
    "Reed Organ",
    "Accordion",
    "Harmonica",
    "Tango Accordion",
    "Acoustic Guitar (nylon)",
    "Acoustic Guitar (steel)",
    "Electric Guitar (jazz)",
    "Electric Guitar (clean)",
    "Electric Guitar (muted)",
    "Overdriven Guitar",
    "Distortion Guitar",
    "Guitar Harmonics",
    "Acoustic Bass",
    "Electric Bass (finger)",
    "Electric Bass (pick)",
    "Fretless Bass",
    "Slap Bass 1",
    "Slap Bass 2",
    "Synth Bass 1",
    "Synth Bass 2",
    "Violin",
    "Viola",
    "Cello",
    "Contrabass",
    "Tremolo Strings",
    "Pizzicato Strings",
    "Orchestral Harp",
    "Timpani",
    "String Ensemble 1",
    "String Ensemble 2",
    "SynthStrings 1",
    "SynthStrings 2",
    "Choir Aahs",
    "Voice Oohs",
    "Synth Voice",
    "Orchestra Hit",
    "Trumpet",
    "Trombone",
    "Tuba",
    "Muted Trumpet",
    "French Horn",
    "Brass Section",
    "SynthBrass 1",
    "SynthBrass 2",
    "Soprano Sax",
    "Alto Sax",
    "Tenor Sax",
    "Baritone Sax",
    "Oboe",
    "English Horn",
    "Bassoon",
    "Clarinet",
    "Piccolo",
    "Flute",
    "Recorder",
    "Pan Flute",
    "Blown Bottle",
    "Shakuhachi",
    "Whistle",
    "Ocarina",
    "Lead 1 (square)",
    "Lead 2 (sawtooth)",
    "Lead 3 (calliope)",
    "Lead 4 (chiff)",
    "Lead 5 (charang)",
    "Lead 6 (voice)",
    "Lead 7 (fifths)",
    "Lead 8 (bass + lead)",
    "Pad 1 (new age)",
    "Pad 2 (warm)",
    "Pad 3 (polysynth)",
    "Pad 4 (choir)",
    "Pad 5 (bowed)",
    "Pad 6 (metallic)",
    "Pad 7 (halo)",
    "Pad 8 (sweep)",
    "FX 1 (rain)",
    "FX 2 (soundtrack)",
    "FX 3 (crystal)",
    "FX 4 (atmosphere)",
    "FX 5 (brightness)",
    "FX 6 (goblins)",
    "FX 7 (echoes)",
    "FX 8 (sci-fi)",
    "Sitar",
    "Banjo",
    "Shamisen",
    "Koto",
    "Kalimba",
    "Bagpipe",
    "Fiddle",
    "Shanai",
    "Tinkle Bell",
    "Agogo",
    "Steel Drums",
    "Woodblock",
    "Taiko Drum",
    "Melodic Tom",
    "Synth Drum",
    "Reverse Cymbal",
    "Guitar Fret Noise",
    "Breath Noise",
    "Seashore",
    "Bird Tweet",
    "Telephone Ring",
    "Helicopter",
    "Applause",
    "Gunshot",

    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "Acoustic Bass Drum",
    "Bass Drum 1",
    "Side Stick",
    "Acoustic Snare",
    "Hand Clap",
    "Electric Snare",
    "Low Floor Tom",
    "Closed Hi Hat",
    "High Floor Tom",
    "Pedal Hi-Hat",
    "Low Tom",
    "Open Hi-Hat",
    "Low-Mid Tom",
    "Hi-Mid Tom",
    "Crash Cymbal 1",
    "High Tom",
    "Ride Cymbal 1",
    "Chinese Cymbal",
    "Ride Bell",
    "Tambourine",
    "Splash Cymbal",
    "Cowbell",
    "Crash Cymbal 2",
    "Vibraslap",
    "Ride Cymbal 2",
    "Hi Bongo",
    "Low Bongo",
    "Mute Hi Conga",
    "Open Hi Conga",
    "Low Conga",
    "High Timbale",
    "Low Timbale",
    "High Agogo",
    "Low Agogo",
    "Cabasa",
    "Maracas",
    "Short Whistle",
    "Long Whistle",
    "Short Guiro",
    "Long Guiro",
    "Claves",
    "Hi Wood Block",
    "Low Wood Block",
    "Mute Cuica",
    "Open Cuica",
    "Mute Triangle",
    "Open Triangle",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};

void XMISequencerInit(XMISequencer *seq) {
  memset(seq, 0, sizeof(XMISequencer));
  seq->ticksPerSec = 120.f;
}

typedef enum {
  MetaEventType_MIDI_PORT = 0X21,
  MetaEventType_END_OF_TRACK = 0X2F,
  MetaEventType_TEMPO = 0X51,
  MetaEventType_SMPTE_OFFSET = 0X54,
  MetaEventType_TIM_SIG = 0X58,
  MetaEventType_KEY_SIG = 0X59,
} MetaEventType;

static void parseMetaEvent(XMISequencer *seq, uint8_t eventType, uint8_t size,
                           const uint8_t *data) {
  switch ((MetaEventType)eventType) {
  case MetaEventType_MIDI_PORT:
    assert(size == 1);
    printf("Port 0x%x\n", data[0]);
    return;
  case MetaEventType_END_OF_TRACK:
    assert(size == 0);
    printf("END OF TRACK\n");
    seq->done = 1;
    return;
  case MetaEventType_TEMPO:
    assert(size == 3);
    uint32_t usec = READ_U24BE(data, 0);
    double usecPerTick = (double)usec / ((usec * 3.f) / 25000);
    seq->ticksPerSec = 1000000 / usecPerTick;
    printf("TEMPO %f\n", seq->ticksPerSec);
    return;
  case MetaEventType_SMPTE_OFFSET:
    assert(size == 5);
    printf("SMPTE OFFSET\n");
    return;
  case MetaEventType_TIM_SIG:
    assert(size == 4);
    printf("TIME SIG\n");
    return;
  case MetaEventType_KEY_SIG:
    assert(size == 2);
    printf("KEY SIG\n");
    return;
  }
  printf("Unhandled Meta event type=%X size=%X data=", eventType, size);
  for (int i = 0; i < size; i++) {
    printf("%X ", data[i]);
  }
  printf("\n");
  assert(0);
}

static size_t onMetaEvent(XMISequencer *seq, uint8_t *data, size_t remaining) {
  // List of meta events
  // https://www.mixagesoftware.com/en/midikit/help/HTML/meta_events.html
  uint8_t eventType = *data;
  data++;

  uint8_t eventSize = *data;
  data++;
  uint8_t *eventData = data;

  parseMetaEvent(seq, eventType, eventSize, eventData);

  return eventSize + 2;
}
static void onPitchBend(XMISequencer *seq, uint8_t channel, uint8_t lsb,
                        uint8_t msb) {
  printf("PitchBend: channel=0X%X lsb=%X msb=%X\n", channel, lsb, msb);
}
static void onController(XMISequencer *seq, uint8_t channel, uint8_t controller,
                         uint8_t value) {
  printf("CChange: channel=0X%X controller:0X%X value=0X%X\n", channel,
         controller, value);
}

static void onInstrumentChange(XMISequencer *seq, uint8_t channel,
                               uint8_t instrId) {

  printf("InsrChange: channel=0X%X instr=0X%X %s\n", channel, instrId,
         OPLPatch_names[instrId]);
}

static void onNoteOn(XMISequencer *seq, uint8_t channel, uint8_t note,
                     uint8_t vel, uint32_t dur) {
  printf("NoteOn: note=0X%X channel=%X vel=0X%X dur=0X%X\n", note, channel, vel,
         dur);
}

uint32_t readXMIDelay(XMISequencer *seq, uint8_t *data, size_t remaining,
                      uint32_t *vlqRet) {
  uint32_t vlq = 0;
  uint8_t b = 0;
  int pos = 0;
  do {
    b = data[pos++];
    remaining--;
    vlq <<= 7;
    vlq |= (b & 0x7f);
  } while ((b & 0x80) && (remaining));
  *vlqRet = vlq;
  return pos;
}

static uint32_t readMidiDelay(XMISequencer *seq, uint8_t *data,
                              size_t remaining, uint32_t *delayRet) {
  uint32_t delay = 0;
  uint8_t b = 0;
  int pos = 0;
  if (pos >= remaining || (data[pos] & 0x80))
    return 0;

  do {
    b = data[pos];
    if (!(b & 0x80)) {
      delay += b;
      pos++;
      remaining--;
    }
  } while ((b == 0x7f) && (remaining));
  *delayRet = delay;
  return pos;
}

typedef enum {
  EventType_CTL = 0XB0,
  EventType_PITCH_BEND = 0XE0,
  EventType_INSTR_CHANGE = 0XC0,
  EventType_NOTE_ON = 0X90,
} EventType;

void XMISequencerPlay(XMISequencer *seq, const XMISequence *sequence) {
  size_t remaining = sequence->events.dataSize;
  uint8_t *data = sequence->events.data;
  remaining--;
  while (seq->done == 0 && remaining) {
    uint8_t b = *data++;
    remaining -= 1;

    if (b & 0x80) { // MIDI CMD
      if (b == 0XFF) {
        size_t r = onMetaEvent(seq, data, remaining);
        data += r;
        remaining -= r;

      } else {
        uint8_t cmd = b & 0XF0;
        uint8_t chan = b & 0X0F;
        switch ((EventType)cmd) {

        case EventType_CTL:
          onController(seq, chan, data[0], data[1]);
          data += 2;
          remaining -= 2;
          break;
        case EventType_PITCH_BEND:
          onPitchBend(seq, chan, data[0], data[1]);
          data += 2;
          remaining -= 2;
          break;
        case EventType_INSTR_CHANGE:
          onInstrumentChange(seq, chan, data[0]);
          data += 1;
          remaining -= 1;
          break;
        case EventType_NOTE_ON: {
          uint8_t note = data[0];
          uint8_t vel = data[1];
          data += 2;
          remaining -= 2;
          uint32_t delay = 0;
          int r = readXMIDelay(seq, data, remaining, &delay);
          data += r;
          remaining -= r;
          onNoteOn(seq, chan, note, vel, delay);
          break;
        }
        default:
          assert(0);
        }
      }
      uint32_t delay = 0;
      int r = readMidiDelay(seq, data, remaining, &delay);
      data += r;
      remaining -= r;
      if (delay) {
        printf("Delay 0X%X\n", delay);
      }
    }
  }
}
