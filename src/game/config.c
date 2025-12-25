#include "config.h"
#include <assert.h>

int GameConfigCreateDefault(ConfigHandle *handle) {
  assert(handle);

  assert(ConfigHandleAddValue(handle, CONF_KEY_SOUND_VOL, "8"));
  assert(ConfigHandleAddValue(handle, CONF_KEY_VOICE_VOL, "8"));
  assert(ConfigHandleAddValue(handle, CONF_KEY_MUSIC_VOL, "8"));
  return 1;
}
