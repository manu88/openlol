#pragma once
#include "pak_file.h"
#include <SDL2/SDL.h>

typedef struct {
  SDL_AudioDeviceID deviceID;
  SDL_AudioSpec audioSpec;
} AudioSystem;

int AudioSystemInit(AudioSystem *audioSystem);
void AudioSystemRelease(AudioSystem *audioSystem);

void AudioSystemClear(AudioSystem *audioSystem);

int AudioSystemQueueVoc(AudioSystem *audioSystem, const PAKFile *pak,
                        const char *vocName);
