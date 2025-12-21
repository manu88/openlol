#pragma once
#include <SDL2/SDL.h>

typedef struct {
  SDL_AudioDeviceID deviceID;
  SDL_AudioSpec audioSpec;
} AudioSystem;

int AudioSystemInit(AudioSystem *audioSystem);
void AudioSystemRelease(AudioSystem *audioSystem);
int AudioSystemPlayTest(AudioSystem *audioSystem);
