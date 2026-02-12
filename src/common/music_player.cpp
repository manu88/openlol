#include "music_player.h"
#include "SDL_audio.h"
#include "mplayer/player.hpp"
#include <stdio.h>

#include <stdlib.h>
#include <ymfm_opl.h>

extern "C" {
#include <SDL2/SDL.h>

typedef struct _MusicPlayer {
  OPLPlayer *player;
} MusicPlayer;

MusicPlayer *MusicPlayerCreate(void) {
  auto player = new OPLPlayer(2);

  const char *patchPath = "GENMIDI.wopl";
  if (!player->loadPatches(patchPath)) {
    printf("Unable to load patches\n");
    delete player;
    return NULL;
  }

  player->setSampleRate(22222);
  player->setStereo(true);

  return reinterpret_cast<MusicPlayer *>(player);
}

void MusicPlayerRelease(MusicPlayer *_player) {
  OPLPlayer *player = reinterpret_cast<OPLPlayer *>(_player);
  delete player;
}

int MusicPlayerLoadSequence(MusicPlayer *player, const XMIHandle *handle) {
  auto *_player = reinterpret_cast<OPLPlayer *>(player);
  if (!_player->loadSequence(handle->data, handle->dataSize)) {
    return 0;
  }
  return 1;
}

void MusicPlayerSetTrackId(MusicPlayer *player, int trackId) {
  OPLPlayer *_player = reinterpret_cast<OPLPlayer *>(player);
  _player->setSongNum(trackId);
}

void MusicPlayerGenerate(MusicPlayer *player, int16_t *stream,
                         unsigned numSamples) {
  auto *_player = reinterpret_cast<OPLPlayer *>(player);
  _player->generate(stream, numSamples);
}

static void audioCallback(void *data, uint8_t *stream, int len) {
  memset(stream, 0, len);

  MusicPlayer *player = reinterpret_cast<MusicPlayer *>(data);
  MusicPlayerGenerate(player, reinterpret_cast<int16_t *>(stream),
                      len / (2 * sizeof(int16_t)));
}

static int running = 1;
static void quit(int) {
  running = 0;
  SDL_PauseAudio(1);
}

int MusicMainLoop(const XMIHandle *handle, int trackId) {
  MusicPlayer *player = MusicPlayerCreate();

  if (!MusicPlayerLoadSequence(player, handle)) {
    printf("Unable to load sequence\n");
    MusicPlayerRelease(player);
    return 1;
  }
  MusicPlayerSetTrackId(player, trackId);

  SDL_SetMainReady();
  SDL_Init(SDL_INIT_AUDIO);

  SDL_AudioSpec desiredSpec = {0};
  desiredSpec.freq = 22222;
  desiredSpec.format = AUDIO_S16SYS;
  desiredSpec.channels = 2;
  desiredSpec.samples = 1024;
  desiredSpec.callback = audioCallback;
  desiredSpec.userdata = player;

  SDL_AudioSpec obtainedSpec = {0};
  if (SDL_OpenAudio(&desiredSpec, &obtainedSpec)) {
    fprintf(stderr, "couldn't open audio device\n");
    MusicPlayerRelease(player);
    return 1;
  }

  signal(SIGINT, quit);
  printf("Start playback, ctl+C to stop\n");
  SDL_PauseAudio(0);
  while (running) {
    SDL_Delay(100);
  }
  SDL_Quit();

  MusicPlayerRelease(player);
  return 0;
}

} // extern "C"
