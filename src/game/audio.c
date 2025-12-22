#include "audio.h"
#include "SDL_audio.h"
#include "formats/format_voc.h"
#include "pak_file.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static inline uint16_t getAudioGain(uint8_t vol) {
  return INT16_MAX * vol / 10;
}

int AudioSystemInit(AudioSystem *audioSystem) {
  memset(audioSystem, 0, sizeof(AudioSystem));

  audioSystem->musicVol = 6;
  audioSystem->soundVol = 6;
  audioSystem->voiceVol = 6;
  printf("init audio\n");
  SDL_AudioSpec desiredSpec = {0};
  desiredSpec.freq = 22222;
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

  return 1;
}

void AudioSystemRelease(AudioSystem *audioSystem) {
  SDL_CloseAudioDevice(audioSystem->deviceID);
}

static inline uint8_t clampVol(int8_t vol) {
  if (vol > 10) {
    return 10;
  } else if (vol < 0) {
    return 0;
  }
  return vol;
}

void AudioSystemSetSoundVolume(AudioSystem *audioSystem, int8_t vol) {
  audioSystem->soundVol = clampVol(vol);
}

void AudioSystemSetMusicVolume(AudioSystem *audioSystem, int8_t vol) {
  audioSystem->musicVol = clampVol(vol);
}

void AudioSystemSetVoiceVolume(AudioSystem *audioSystem, int8_t vol) {
  audioSystem->voiceVol = clampVol(vol);
}

void AudioSystemClear(AudioSystem *audioSystem) {
  SDL_ClearQueuedAudio(audioSystem->deviceID);
}

int AudioSystemQueueVoc(AudioSystem *audioSystem, const PAKFile *pak,
                        const char *vocName) {
  int entryIndex = PakFileGetEntryIndex(pak, vocName);
  const uint8_t *buffer = PakFileGetEntryData(pak, entryIndex);
  size_t bufferSize = PakFileGetEntrySize(pak, entryIndex);
  VOCHandle handle = {0};
  if (!VOCHandleFromBuffer(&handle, buffer, bufferSize)) {
    printf("Unable to read voc file '%s'\n", vocName);
    return 0;
  }
  const VOCBlock *block = handle.firstBlock;
  do {

    if (block->type != VOCBlockType_SoundDataTyped) {
      printf("AudioSystemQueueVoc: skipping unsupported audio type %i\n",
             block->type);
      continue;
    }
    const VOCSoundDataTyped *data =
        (const VOCSoundDataTyped *)VOCBlockGetData(block);
    int sampleRate = 1000000 / (256 - data->freqDivisor);
    if (sampleRate != audioSystem->audioSpec.freq) {
      printf("AudioSystemQueueVoc: different samplerate %i %i\n", sampleRate,
             audioSystem->audioSpec.freq);
      continue;
    }
    const int8_t *inSamples = ((int8_t *)data) + 2;
    size_t numSamples = VOCBlockGetSize(block) - 2;
    printf("Block numSamples=%zu sampleRate=%i\n", numSamples, sampleRate);
    Sint16 *samples = malloc(numSamples * sizeof(Sint16));

    int32_t vol = getAudioGain(audioSystem->soundVol);
    for (size_t i = 0; i < numSamples; i++) {
      samples[i] = (inSamples[i] - 0X80) << 8;
      samples[i] = (int16_t)(vol * (samples[i]) / (INT32_C(1) << 15));
    }
    printf("SDL_QueueAudio voc %s\n", vocName);
    SDL_QueueAudio(audioSystem->deviceID, samples, numSamples * sizeof(Sint16));
    free(samples);

  } while ((block = VOCHandleGetNextBlock(&handle, block)));
  return 1;
}
