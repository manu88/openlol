#include "audio.h"
#include "SDL_audio.h"
#include "config.h"
#include "formats/format_voc.h"
#include "pak_file.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void AudioQueueInit(AudioQueue *queue) {
  memset(queue, 0, sizeof(AudioQueue));
  queue->vocHandle.header = NULL;
}

void AudioQueueReset(AudioQueue *queue, size_t sequenceSize) {
  queue->currentBlock = NULL;
  queue->currentSample = 0;
  queue->currentSequence = 0;
  queue->sequenceSize = sequenceSize;
}

static inline uint16_t getAudioGain(uint8_t vol) {
  return INT16_MAX * vol / 10;
}

static void _audioCallbackQueue(AudioQueue *queue, int16_t *samples,
                                size_t numSamplesOut, int32_t vol) {
  if (queue->sequenceSize == 0) {
    return;
  }
  if (queue->currentBlock == NULL) {
    queue->currentBlock = queue->sequence[queue->currentSequence].firstBlock;
  }
  assert(queue->currentBlock);

  const VOCSoundDataTyped *data =
      (const VOCSoundDataTyped *)VOCBlockGetData(queue->currentBlock);
  size_t numBLockSamples = VOCBlockGetSize(queue->currentBlock) - 2;
  size_t remainingBlockSamples = numBLockSamples - queue->currentSample;

  if (remainingBlockSamples == 0) {
    if (VOCBlockIsLast(queue->currentBlock)) {
      if (queue->currentSequence == queue->sequenceSize - 1) {
        queue->sequenceSize = 0;
        return;
      } else {
        queue->currentSequence++;
        queue->currentSample = 0;
        queue->currentBlock = NULL;
      }
    } else {
      queue->currentBlock = VOCHandleGetNextBlock(
          &queue->sequence[queue->currentSequence], queue->currentBlock);
      queue->currentSample = 0;
    }
  } else {
    size_t numSamplesToCopy = remainingBlockSamples > numSamplesOut
                                  ? numSamplesOut
                                  : remainingBlockSamples;
    assert(numSamplesToCopy <= numSamplesOut);
    const int8_t *inSamples = ((int8_t *)data) + 2;
    for (size_t i = 0; i < numSamplesToCopy; i++) {
      int16_t v = (inSamples[i + queue->currentSample] - 0X80) << 8;
      samples[i] += (int16_t)(vol * v / (INT32_C(1) << 15));
    }
    queue->currentSample += numSamplesToCopy;
  }
}

static void _audioCallback(void *userdata, Uint8 *stream, int len) {
  AudioSystem *audioSystem = (AudioSystem *)userdata;
  assert(audioSystem);
  int16_t *samples = (int16_t *)stream;
  size_t numSamplesOut = len / 2;

  memset(stream, 0, len);

  _audioCallbackQueue(&audioSystem->soundQueue, samples, numSamplesOut,
                      getAudioGain(audioSystem->soundVol));
  _audioCallbackQueue(&audioSystem->voiceQueue, samples, numSamplesOut,
                      getAudioGain(audioSystem->voiceVol));
}

int AudioSystemInit(AudioSystem *audioSystem, const ConfigHandle *conf) {
  memset(audioSystem, 0, sizeof(AudioSystem));

  audioSystem->musicVol =
      (int)ConfigHandleGetValueFloat(conf, CONF_KEY_MUSIC_VOL, 6);
  audioSystem->soundVol =
      (int)ConfigHandleGetValueFloat(conf, CONF_KEY_SOUND_VOL, 6);
  audioSystem->voiceVol =
      (int)ConfigHandleGetValueFloat(conf, CONF_KEY_VOICE_VOL, 6);
  printf("init audio\n");
  SDL_AudioSpec desiredSpec = {0};
  desiredSpec.freq = 22222;
  desiredSpec.format = AUDIO_S16SYS;
  desiredSpec.channels = 1;
  desiredSpec.samples = 1024;
  desiredSpec.callback = _audioCallback;
  desiredSpec.userdata = audioSystem;

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

  AudioQueueInit(&audioSystem->voiceQueue);
  AudioQueueInit(&audioSystem->soundQueue);
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

void AudioSystemPlaySequence(AudioSystem *audioSystem, const PAKFile *pak,
                             int *sequence, size_t sequenceSize) {
  SDL_LockAudioDevice(audioSystem->deviceID);
  for (int i = 0; i < sequenceSize; i++) {
    int entryIndex = sequence[i];
    const uint8_t *buffer = PakFileGetEntryData(pak, entryIndex);
    size_t bufferSize = PakFileGetEntrySize(pak, entryIndex);

    if (!VOCHandleFromBuffer(&audioSystem->voiceQueue.sequence[i], buffer,
                             bufferSize)) {
      printf("Unable to read voc file index %i\n", entryIndex);
      continue;
    }
  }

  AudioQueueReset(&audioSystem->voiceQueue, sequenceSize);
  SDL_UnlockAudioDevice(audioSystem->deviceID);
}

void AudioSystemPlaySoundFX(AudioSystem *audioSystem, const PAKFile *pak,
                            const char *filename) {
  int entryIndex = PakFileGetEntryIndex(pak, filename);
  if (entryIndex == -1) {
    printf("AudioSystemPlaySoundFX: no such sfx file '%s'\n", filename);
    return;
  }

  SDL_LockAudioDevice(audioSystem->deviceID);

  const uint8_t *buffer = PakFileGetEntryData(pak, entryIndex);
  size_t bufferSize = PakFileGetEntrySize(pak, entryIndex);

  if (VOCHandleFromBuffer(&audioSystem->soundQueue.sequence[0], buffer,
                          bufferSize)) {

    AudioQueueReset(&audioSystem->soundQueue, 1);
  } else {
    printf("VOCHandleFromBuffer error for file '%s'\n", filename);
  }

  SDL_UnlockAudioDevice(audioSystem->deviceID);
}
