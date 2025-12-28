#include "config.h"
#include "formats/format_config.h"
#include <assert.h>
#include <string.h>

int GameConfigFromFile(GameConfig *config, const char *filepath) {
  ConfigHandle h = {0};
  if (!ConfigHandleFromFile(&h, filepath)) {
    return 1;
  }
  GameConfigCreateDefault(config);
  config->tickLength =
      ConfigHandleGetValueFloat(&h, CONF_KEY_TICK_DURATION, config->tickLength);
  config->musicVol =
      ConfigHandleGetValueFloat(&h, CONF_KEY_MUSIC_VOL, config->musicVol);
  config->voiceVol =
      ConfigHandleGetValueFloat(&h, CONF_KEY_VOICE_VOL, config->voiceVol);
  config->soundVol =
      ConfigHandleGetValueFloat(&h, CONF_KEY_SOUND_VOL, config->soundVol);
  config->moveInAutomap = ConfigHandleGetValueFloat(&h, CONF_KEY_AUTOMAP_MOVE,
                                                    config->moveInAutomap);

  ConfigHandleRelease(&h);
  return 1;
}
int GameConfigCreateDefault(GameConfig *config) {
  memset(config, 0, sizeof(GameConfig));
  config->tickLength = 20;
  config->musicVol = config->soundVol = config->voiceVol = 8;
  return 1;
}

int GameConfigWriteFile(const GameConfig *config, const char *filepath) {
  ConfigHandle h = {0};
  ConfigHandleSetValueInt(&h, CONF_KEY_TICK_DURATION, config->tickLength);
  ConfigHandleSetValueInt(&h, CONF_KEY_MUSIC_VOL, config->musicVol);
  ConfigHandleSetValueInt(&h, CONF_KEY_VOICE_VOL, config->voiceVol);
  ConfigHandleSetValueInt(&h, CONF_KEY_SOUND_VOL, config->soundVol);
  if (config->moveInAutomap) {
    ConfigHandleSetValueInt(&h, CONF_KEY_AUTOMAP_MOVE, 1);
  }
  int ret = ConfigHandleWriteFile(&h, filepath);
  ConfigHandleRelease(&h);
  return ret;
}
