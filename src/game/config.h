#pragma once
#include <stdint.h>

#define CONF_KEY_SOUND_VOL "soundVol"
#define CONF_KEY_MUSIC_VOL "musicVol"
#define CONF_KEY_VOICE_VOL "voiceVol"
#define CONF_KEY_TICK_DURATION "tickDuration"
#define CONF_KEY_AUTOMAP_MOVE "moveInAutomap"

typedef struct {
  uint8_t soundVol;
  uint8_t voiceVol;
  uint8_t musicVol;
  int tickLength;

  int moveInAutomap; // default 0
} GameConfig;

int GameConfigFromFile(GameConfig *config, const char *filepath);
int GameConfigCreateDefault(GameConfig *config);
int GameConfigWriteFile(const GameConfig *config, const char *filepath);
