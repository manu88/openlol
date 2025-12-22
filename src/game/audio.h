#pragma once
#include "pak_file.h"
#include <SDL2/SDL.h>
#include <stdint.h>

typedef struct {
  SDL_AudioDeviceID deviceID;
  SDL_AudioSpec audioSpec;

  uint8_t soundVol; // 0-10
  uint8_t musicVol; // 0-10
  uint8_t voiceVol; // 0-10
} AudioSystem;

int AudioSystemInit(AudioSystem *audioSystem);
void AudioSystemRelease(AudioSystem *audioSystem);

void AudioSystemSetSoundVolume(AudioSystem *audioSystem, int8_t vol);
void AudioSystemSetMusicVolume(AudioSystem *audioSystem, int8_t vol);
void AudioSystemSetVoiceVolume(AudioSystem *audioSystem, int8_t vol);

void AudioSystemClear(AudioSystem *audioSystem);

int AudioSystemQueueVoc(AudioSystem *audioSystem, const PAKFile *pak,
                        const char *vocName);
