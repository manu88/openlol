#include "audio.h"
#include "SDL_audio.h"
#include <stdio.h>
#include <string.h>

int AudioSystemInit(AudioSystem *audioSystem) {
  memset(audioSystem, 0, sizeof(AudioSystem));
  printf("init audio\n");
  SDL_AudioSpec desiredSpec = {0};
  desiredSpec.freq = 44100;
  desiredSpec.format = AUDIO_S16SYS;
  desiredSpec.channels = 1;
  desiredSpec.samples = 4096;

  audioSystem->deviceID =
      SDL_OpenAudioDevice(NULL, 0, &desiredSpec, &audioSystem->audioSpec, 0);
  if (audioSystem->deviceID == 0) {
    printf("SDL_OpenAudioDevice error\n");
    return 0;
  }

  printf("audio config:\n");
  printf("freq: %i\n", audioSystem->audioSpec.freq);
  printf("samples: %i\n", audioSystem->audioSpec.samples);
  SDL_PauseAudioDevice(audioSystem->deviceID, 0);

  // AudioSystemPlayTest(audioSystem);
  return 1;
}

void AudioSystemRelease(AudioSystem *audioSystem) {
  SDL_CloseAudioDevice(audioSystem->deviceID);
}

int AudioSystemPlayTest(AudioSystem *audioSystem) {
  int sample_count = 5 * audioSystem->audioSpec.freq; // 5 seconds
  Sint16 *samples = malloc(sample_count * sizeof(Sint16));
  for (int i = 0; i < sample_count; ++i) {
    samples[i] = rand() % 30000; // white noise
  }

  SDL_QueueAudio(audioSystem->deviceID, samples, sample_count * sizeof(Sint16));
  return 1;
}
