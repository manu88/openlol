#pragma once
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

  VOCHandle sequence[MAX_VOC_SEQ_ENTRIES];
  size_t sequenceSize;
  int currentSequence;
} AudioQueue;

void AudioQueueInit(AudioQueue *queue);

typedef struct {
  SDL_AudioDeviceID deviceID;
  SDL_AudioSpec audioSpec;

  uint8_t soundVol; // 0-10
  uint8_t musicVol; // 0-10
  uint8_t voiceVol; // 0-10

  AudioQueue voiceQueue;
} AudioSystem;

int AudioSystemInit(AudioSystem *audioSystem);
void AudioSystemRelease(AudioSystem *audioSystem);

void AudioSystemSetSoundVolume(AudioSystem *audioSystem, int8_t vol);
void AudioSystemSetMusicVolume(AudioSystem *audioSystem, int8_t vol);
void AudioSystemSetVoiceVolume(AudioSystem *audioSystem, int8_t vol);

void AudioSystemClear(AudioSystem *audioSystem);

void AudioSystemPlaySequence(AudioSystem *audioSystem, const PAKFile *pak,
                             int *sequence, size_t sequenceSize);
