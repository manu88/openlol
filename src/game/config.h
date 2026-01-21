#pragma once
#include <stdint.h>

#define CONF_KEY_SOUND_VOL "soundVol"
#define CONF_KEY_MUSIC_VOL "musicVol"
#define CONF_KEY_VOICE_VOL "voiceVol"
#define CONF_KEY_TICK_DURATION "tickDuration"
#define CONF_KEY_AUTOMAP_MOVE "moveInAutomap"
#define CONF_KEY_NO_CLIP "noClip"
#define CONF_KEY_AUTOMAP_SHOW_MONSTERS "monstersInAutomap"
#define CONF_KEY_DEBUG "debug"

typedef struct {
  uint8_t soundVol;
  uint8_t voiceVol;
  uint8_t musicVol;
  int tickLength;

  int moveInAutomap; // default 0
  int noClip;
  int showMonstersInMap;
  int debug;
} GameConfig;

int GameConfigFromFile(GameConfig *config, const char *filepath);
int GameConfigCreateDefault(GameConfig *config);
int GameConfigWriteFile(const GameConfig *config, const char *filepath);
