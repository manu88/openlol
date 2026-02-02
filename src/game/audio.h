#pragma once
#include "config.h"
#include "formats/format_voc.h"
#include "pak_file.h"
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_VOC_SEQ_ENTRIES 6

typedef struct {
  VOCHandle vocHandle;
  const VOCBlock *currentBlock;
  size_t currentSample;

  int sequence[MAX_VOC_SEQ_ENTRIES]; // this one is not touched by the audio
                                     // thread!
  VOCHandle vocHandles[MAX_VOC_SEQ_ENTRIES];
  size_t sequenceSize;
  int currentSequenceIndex;
} AudioQueue;

void AudioQueueInit(AudioQueue *queue);
void AudioQueueReset(AudioQueue *queue, size_t sequenceSize);

typedef struct {
  SDL_AudioDeviceID deviceID;
  SDL_AudioSpec audioSpec;

  // don't access these directly, as they are shared with the audio callback!
  // use getter/setters below.
  uint8_t _soundVol; // 0-10
  uint8_t _musicVol; // 0-10
  uint8_t _voiceVol; // 0-10

  AudioQueue voiceQueue;
  AudioQueue soundQueue;
} AudioSystem;

int AudioSystemInit(AudioSystem *audioSystem, const GameConfig *conf);
void AudioSystemRelease(AudioSystem *audioSystem);

void AudioSystemSetSoundVolume(AudioSystem *audioSystem, int8_t vol);
uint8_t AudioSystemGetSoundVolume(const AudioSystem *audioSystem);

void AudioSystemSetMusicVolume(AudioSystem *audioSystem, int8_t vol);
uint8_t AudioSystemGetMusicVolume(const AudioSystem *audioSystem);

void AudioSystemSetVoiceVolume(AudioSystem *audioSystem, int8_t vol);
uint8_t AudioSystemGetVoiceVolume(const AudioSystem *audioSystem);

void AudioSystemClearVoiceQueue(AudioSystem *audioSystem);
void AudioSystemStopSpeech(AudioSystem *audioSystem);

void AudioSystemPlayVoiceSequence(AudioSystem *audioSystem, const PAKFile *pak,
                                  int *sequence, size_t sequenceSize);
int AudioSystemGetCurrentVoiceIndex(const AudioSystem *audioSystem);
void AudioSystemPlaySoundFX(AudioSystem *audioSystem, const PAKFile *pak,
                            const char *filename);
